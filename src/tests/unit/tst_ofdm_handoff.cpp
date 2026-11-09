/*
 * OFDM frame handoff and lifecycle regression tests.
 * This file was generated with the help of an AI assistant
 * (OpenAi Codex 6 Astra).
 *
 * welle.io is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * welle.io is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with welle.io; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <QtTest>
#include <QProcess>
#include <QProcessEnvironment>
#include <chrono>
#include <future>
#include <functional>
#include <mutex>
#include <thread>
#include <system_error>
#include "ofdm-decoder.h"
#include "null_radio_controller.h"

using namespace std::chrono_literals;

// Only control scheduling/state at the handoff boundary. The library's actual
// worker, lifecycle methods, FFT and symbol decoding run unchanged in every test.
class OfdmDecoderTestAccess {
public:
    static void stop(OfdmDecoder& d) { d.stopWorker(); }
    static void prepareDelayedWorker(OfdmDecoder& d) {
        d.stopWorker();
        std::lock_guard<std::mutex> lock(d.mutex);
        d.running = true;
    }
    static void start(OfdmDecoder& d) {
        d.thread = std::thread(&OfdmDecoder::workerthread, &d);
    }
    static void startAfter(OfdmDecoder& d, std::shared_future<void> gate) {
        d.thread = std::thread([&d, gate] {
            gate.wait();
            d.workerthread();
        });
    }
    static void failResetThreadCreation(OfdmDecoder& d) {
        d.resetWorker([](OfdmDecoder&) -> std::thread {
            throw std::system_error(
                    std::make_error_code(std::errc::resource_unavailable_try_again));
        });
    }
    static void publishNewGeneration(OfdmDecoder& d) {
        // Model the boundary where reset has already restarted the receiver
        // before an old producer gets its mutex back. Do not start a consumer.
        std::lock_guard<std::mutex> lock(d.mutex);
        ++d.generation;
        d.pending_symbols.clear();
        d.num_pending_symbols = 0;
        d.pending_symbols_cv.notify_all();
    }
    static void nextPrsReportsSnr(OfdmDecoder& d) { d.snrCount = 10; }
    static void notify(OfdmDecoder& d) { d.pending_symbols_cv.notify_all(); }
    static int pending(OfdmDecoder& d) {
        std::lock_guard<std::mutex> lock(d.mutex);
        return d.num_pending_symbols;
    }
    static bool running(OfdmDecoder& d) { return d.running; }
};

class FrameObserver : public NullRadioController {
public:
    std::mutex mutex;
    std::vector<std::vector<DSPCOMPLEX>> frames;
    std::function<void()> snrCallback;
    void onSNR(float) override { if (snrCallback) snrCallback(); }
    void onConstellationPoints(std::vector<DSPCOMPLEX>&& points) override {
        std::lock_guard<std::mutex> lock(mutex);
        frames.push_back(std::move(points));
    }
    size_t count() {
        std::lock_guard<std::mutex> lock(mutex);
        return frames.size();
    }
};

struct DecoderFixture {
    DABParams params;
    FrameObserver observer;
    FicHandler fic{observer};
    MscHandler msc{params, false};
    OfdmDecoder decoder{params, observer, fic, msc};
    explicit DecoderFixture(int mode = 2) : params(mode) {
        // Synthetic frames also exercise modes II/IV. FicHandler defaults to
        // mode I, so its per-symbol soft-bit count must match the selected K.
        fic.setBitsperBlock(2 * params.K);
    }

    // Impulses have a flat FFT. Consecutive data symbols rotate by phase,
    // making every differential constellation point identify its input frame.
    std::vector<std::vector<DSPCOMPLEX>> frame(float phase) {
        std::vector<std::vector<DSPCOMPLEX>> symbols(params.L);
        symbols[0].resize(params.T_u);
        symbols[0][0] = 1;
        for (int i = 1; i < params.L; ++i) {
            symbols[i].resize(params.T_s);
            symbols[i][params.T_s - params.T_u] = std::polar(1.0f, phase * i);
        }
        return symbols;
    }
};

class TestOfdmHandoff : public QObject {
    Q_OBJECT
private slots:
    void notificationBeforeWorkerStarts() {
        DecoderFixture f;
        OfdmDecoderTestAccess::prepareDelayedWorker(f.decoder);
        f.decoder.pushAllSymbols(f.frame(0.1f));
        OfdmDecoderTestAccess::start(f.decoder);
        QTRY_COMPARE_WITH_TIMEOUT(f.observer.count(), size_t(1), 3000);
    }

    void fullSlotPreservesFrameOrder_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("mode-I") << 1;
        QTest::newRow("mode-II") << 2;
        QTest::newRow("mode-IV") << 4;
    }

    void fullSlotPreservesFrameOrder() {
        QFETCH(int, mode);
        DecoderFixture f(mode);
        OfdmDecoderTestAccess::prepareDelayedWorker(f.decoder);
        f.decoder.pushAllSymbols(f.frame(0.1f));
        auto second = std::async(std::launch::async, [&] {
            f.decoder.pushAllSymbols(f.frame(0.4f));
        });
        const auto waiting = second.wait_for(50ms);
        // Release the worker before assertions, so even failure cannot strand
        // the async future while unwinding the fixture.
        OfdmDecoderTestAccess::start(f.decoder);
        QCOMPARE(waiting, std::future_status::timeout);
        QVERIFY(second.wait_for(3s) == std::future_status::ready);
        second.get();
        QTRY_COMPARE_WITH_TIMEOUT(f.observer.count(), size_t(2), 3000);
        OfdmDecoderTestAccess::stop(f.decoder);
        QCOMPARE(f.observer.frames[0].size(), size_t((f.params.L - 1) * f.params.K / OfdmDecoder::constellationDecimation));
        for (int i = 0; i < 2; ++i) {
            const auto expected = std::polar(1.0f, i == 0 ? 0.1f : 0.4f);
            for (const auto point : f.observer.frames[i])
                QVERIFY(std::abs(point - expected) < 0.001f);
        }
    }

    void spuriousWakeupDoesNotDecode() {
        DecoderFixture f;
        for (int i = 0; i < 100; ++i)
            OfdmDecoderTestAccess::notify(f.decoder);
        QTest::qWait(20);
        OfdmDecoderTestAccess::stop(f.decoder);
        QCOMPARE(f.observer.count(), size_t(0));
    }

    void stopBeforeWorkerEntryStaysStopped() {
        if (!qEnvironmentVariableIsSet("WELLE_TEST_STOP_CHILD")) {
            // A future's destructor can itself block after a failed assertion.
            // Isolate this deliberate hang regression so the parent can report
            // the named case and terminate the child, not wait for CTest's limit.
            QProcess child;
            auto env = QProcessEnvironment::systemEnvironment();
            env.insert("WELLE_TEST_STOP_CHILD", "1");
            child.setProcessEnvironment(env);
            child.start(QCoreApplication::applicationFilePath(),
                        {QStringLiteral("stopBeforeWorkerEntryStaysStopped")});
            QVERIFY(child.waitForStarted(2000));
            const bool finished = child.waitForFinished(3000);
            if (!finished) {
                child.kill();
                child.waitForFinished(2000);
            }
            QVERIFY2(finished, "Worker failed to stop after entering with running=false");
            QCOMPARE(child.exitStatus(), QProcess::NormalExit);
            const auto output = child.readAllStandardOutput() + child.readAllStandardError();
            QVERIFY2(child.exitCode() == 0, output.constData());
            return;
        }

        DecoderFixture f;
        OfdmDecoderTestAccess::prepareDelayedWorker(f.decoder);
        std::promise<void> enter;
        OfdmDecoderTestAccess::startAfter(f.decoder, enter.get_future().share());
        auto stopped = std::async(std::launch::async, [&] {
            OfdmDecoderTestAccess::stop(f.decoder);
        });
        QTRY_VERIFY_WITH_TIMEOUT(!OfdmDecoderTestAccess::running(f.decoder), 1000);
        // The real worker now enters strictly after the stop flag was set.
        enter.set_value();
        stopped.get(); // The parent process bounds a regression here.
        QVERIFY(!OfdmDecoderTestAccess::running(f.decoder));
    }

    void stopRejectsWaitingSubmission() {
        DecoderFixture f;
        OfdmDecoderTestAccess::prepareDelayedWorker(f.decoder);
        f.decoder.pushAllSymbols(f.frame(0.1f));
        auto second = std::async(std::launch::async, [&] {
            f.decoder.pushAllSymbols(f.frame(0.4f));
        });
        const auto waiting = second.wait_for(50ms);
        OfdmDecoderTestAccess::stop(f.decoder);
        QCOMPARE(waiting, std::future_status::timeout);
        QVERIFY(second.wait_for(3s) == std::future_status::ready);
        second.get();
        QCOMPARE(f.observer.count(), size_t(0));
        // Rejected submission has not moved out of the caller's frame.
        auto rejected = f.frame(0.7f);
        f.decoder.pushAllSymbols(std::move(rejected));
        QCOMPARE(rejected.size(), size_t(f.params.L));
    }

    void oldSubmissionCannotCrossGeneration() {
        DecoderFixture f;
        OfdmDecoderTestAccess::prepareDelayedWorker(f.decoder);
        f.decoder.pushAllSymbols(f.frame(0.1f));
        auto oldFrame = f.frame(0.4f);
        std::promise<void> started;
        auto second = std::async(std::launch::async, [&] {
            started.set_value();
            f.decoder.pushAllSymbols(std::move(oldFrame));
        });
        started.get_future().wait();
        const auto waiting = second.wait_for(50ms);
        OfdmDecoderTestAccess::publishNewGeneration(f.decoder);
        QCOMPARE(waiting, std::future_status::timeout);
        QVERIFY(second.wait_for(3s) == std::future_status::ready);
        second.get();
        QCOMPARE(oldFrame.size(), size_t(f.params.L));
        QCOMPARE(OfdmDecoderTestAccess::pending(f.decoder), 0);
    }

    void stopDuringPrsSkipsRemainingSymbols() {
        DecoderFixture f;
        OfdmDecoderTestAccess::prepareDelayedWorker(f.decoder);
        OfdmDecoderTestAccess::nextPrsReportsSnr(f.decoder);
        std::promise<void> entered, resume;
        auto enteredFuture = entered.get_future();
        auto resumeFuture = resume.get_future();
        f.observer.snrCallback = [&] {
            entered.set_value();
            resumeFuture.wait(); // Real PRS callback still owns the frame mutex.
        };
        f.decoder.pushAllSymbols(f.frame(0.1f));
        OfdmDecoderTestAccess::start(f.decoder);
        const auto callbackReady = enteredFuture.wait_for(2s);
        auto stopped = std::async(std::launch::async, [&] {
            OfdmDecoderTestAccess::stop(f.decoder);
        });
        const auto deadline = std::chrono::steady_clock::now() + 2s;
        while (OfdmDecoderTestAccess::running(f.decoder) &&
               std::chrono::steady_clock::now() < deadline)
            std::this_thread::yield();
        const bool stopRequested = !OfdmDecoderTestAccess::running(f.decoder);
        const auto waiting = stopped.wait_for(20ms);
        resume.set_value(); // Always release the callback before assertions.
        stopped.get();
        QVERIFY(callbackReady == std::future_status::ready);
        QVERIFY(stopRequested);
        QVERIFY(waiting == std::future_status::timeout);
        QCOMPARE(f.observer.count(), size_t(0));
        QCOMPARE(OfdmDecoderTestAccess::pending(f.decoder), f.params.L - 1);
    }

    void resetRejectsBlockedProducer() {
        DecoderFixture f;
        OfdmDecoderTestAccess::prepareDelayedWorker(f.decoder);
        f.decoder.pushAllSymbols(f.frame(0.1f));
        auto oldFrame = f.frame(0.4f);
        std::promise<void> started;
        auto producer = std::async(std::launch::async, [&] {
            started.set_value();
            f.decoder.pushAllSymbols(std::move(oldFrame));
        });
        started.get_future().wait();
        const auto waiting = producer.wait_for(50ms);
        f.decoder.reset(); // Exercise the real stop/clear/restart path.
        const auto finished = producer.wait_for(3s);
        if (finished != std::future_status::ready)
            OfdmDecoderTestAccess::stop(f.decoder); // Release before assertions.
        producer.get();
        QVERIFY(waiting == std::future_status::timeout);
        QVERIFY(finished == std::future_status::ready);
        QCOMPARE(oldFrame.size(), size_t(f.params.L));
        f.decoder.pushAllSymbols(f.frame(0.7f));
        QTRY_COMPARE_WITH_TIMEOUT(f.observer.count(), size_t(1), 3000);
        OfdmDecoderTestAccess::stop(f.decoder);
        for (const auto point : f.observer.frames[0])
            QVERIFY(std::abs(point - std::polar(1.0f, 0.7f)) < 0.001f);
    }

    void failedResetStaysStoppedAndCanRecover() {
        DecoderFixture f;
        bool threw = false;
        try {
            OfdmDecoderTestAccess::failResetThreadCreation(f.decoder);
        }
        catch (const std::system_error& error) {
            threw = true;
            QVERIFY(error.code() == std::errc::resource_unavailable_try_again);
        }
        QVERIFY(threw);
        QVERIFY(!OfdmDecoderTestAccess::running(f.decoder));
        auto rejected = f.frame(0.1f);
        f.decoder.pushAllSymbols(std::move(rejected));
        QCOMPARE(rejected.size(), size_t(f.params.L));
        QCOMPARE(OfdmDecoderTestAccess::pending(f.decoder), 0);
        // A failed attempt does not poison a later successful reset.
        f.decoder.reset();
        f.decoder.pushAllSymbols(f.frame(0.7f));
        QTRY_COMPARE_WITH_TIMEOUT(f.observer.count(), size_t(1), 3000);
        OfdmDecoderTestAccess::stop(f.decoder);
    }

    void resetDiscardsPendingFrame() {
        DecoderFixture f;
        OfdmDecoderTestAccess::prepareDelayedWorker(f.decoder);
        f.decoder.pushAllSymbols(f.frame(0.1f));
        f.decoder.reset();
        f.decoder.pushAllSymbols(f.frame(0.7f));
        QTRY_COMPARE_WITH_TIMEOUT(f.observer.count(), size_t(1), 3000);
        OfdmDecoderTestAccess::stop(f.decoder);
        for (const auto point : f.observer.frames[0])
            QVERIFY(std::abs(point - std::polar(1.0f, 0.7f)) < 0.001f);
    }

    void repeatedResetAndOrderedFrames() {
        DecoderFixture f;
        for (int i = 0; i < 30; ++i) {
            f.decoder.pushAllSymbols(f.frame(0.02f * i));
            QTRY_COMPARE_WITH_TIMEOUT(f.observer.count(), size_t(i + 1), 3000);
            f.decoder.reset();
        }
        OfdmDecoderTestAccess::stop(f.decoder);
        QCOMPARE(OfdmDecoderTestAccess::pending(f.decoder), 0);
        for (int i = 0; i < 30; ++i)
            for (const auto point : f.observer.frames[i])
                QVERIFY(std::abs(point - std::polar(1.0f, 0.02f * i)) < 0.001f);
    }
};

QTEST_GUILESS_MAIN(TestOfdmHandoff)
#include "tst_ofdm_handoff.moc"
