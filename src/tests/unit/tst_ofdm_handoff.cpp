/*
 * OFDM frame handoff and lifecycle regression tests.
 * This file is part of welle.io and was developed with an AI assistant.
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
#include <chrono>
#include <future>
#include <mutex>
#include <thread>
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
    static void publishNewGeneration(OfdmDecoder& d) {
        // Model the boundary where reset has already restarted the receiver
        // before an old producer gets its mutex back. Do not start a consumer.
        std::lock_guard<std::mutex> lock(d.mutex);
        ++d.generation;
        d.pending_symbols.clear();
        d.num_pending_symbols = 0;
        d.pending_symbols_cv.notify_all();
    }
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
    DABParams params{2};
    FrameObserver observer;
    FicHandler fic{observer};
    MscHandler msc{params, false};
    OfdmDecoder decoder{params, observer, fic, msc};
    explicit DecoderFixture(int mode = 2) : params(mode) { fic.setBitsperBlock(2 * params.K); }

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
        DecoderFixture f;
        OfdmDecoderTestAccess::stop(f.decoder);
        // Schedule the real worker after the stop flag is already set. It must
        // not restore running=true and wait forever (CTest also has a timeout).
        OfdmDecoderTestAccess::start(f.decoder);
        OfdmDecoderTestAccess::stop(f.decoder);
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
