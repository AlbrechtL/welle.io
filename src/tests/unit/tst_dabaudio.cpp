/*
 * DabAudio fragment handoff and shutdown regression tests.
 * This file is part of welle.io and was generated with the help of an AI
 * assistant (OpenAI Codex).
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
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "dab-audio.h"
#include "dab-processor.h"

using namespace std::chrono_literals;

// Control scheduling and queue boundaries only. The real worker, interleaver,
// channel decoder, publication and shutdown methods are not replaced.
class DabAudioTestAccess {
public:
    static void stop(DabAudio& d) { d.stopWorker(); }
    static void prepareDelayedWorker(DabAudio& d) {
        d.stopWorker();
        std::lock_guard<std::mutex> lock(d.ourMutex);
        d.running = true;
    }
    static void start(DabAudio& d) {
        d.ourThread = std::thread(&DabAudio::run, &d);
    }
    static std::mutex& mutex(DabAudio& d) { return d.ourMutex; }
    static bool running(DabAudio& d) { return d.running; }
    static int pending(DabAudio& d) { return d.mscBuffer.GetRingBufferReadAvailable(); }
    static int leaveSpace(DabAudio& d, int space) {
        // Called only while the worker is stopped and before a producer starts.
        std::lock_guard<std::mutex> lock(d.ourMutex);
        const int capacity = d.mscBuffer.GetRingBufferWriteAvailable();
        std::vector<softbit_t> fill(size_t(capacity - space), 32);
        d.mscBuffer.putDataIntoBuffer(fill.data(), int32_t(fill.size()));
        return capacity;
    }
    static void wakeAll(DabAudio& d) {
        d.mscDataAvailable.notify_all();
        d.mscSpaceAvailable.notify_all();
    }
};

class CountFrames : public DabProcessor {
public:
    explicit CountFrames(std::atomic<int>& count) : count(count) {}
    void addtoFrame(uint8_t*) override { ++count; }
private:
    std::atomic<int>& count;
};

struct Fixture {
    static constexpr int fragment = 384; // 8 kbit/s, EEP-A level 3, six CUs
    std::atomic<int> frames{0};
    DabAudio audio{fragment, 8, ProtectionSettings{}, std::make_unique<CountFrames>(frames)};
    std::vector<softbit_t> bits = std::vector<softbit_t>(fragment, 32);
    void push(int n) {
        for (int i = 0; i < n; ++i) audio.process(bits.data(), fragment);
    }
};

class TestDabAudio : public QObject {
    Q_OBJECT
private slots:
    void invalidFragment_data() {
        QTest::addColumn<int>("fragment");
        QTest::newRow("zero") << 0;
        QTest::newRow("negative") << -1;
        QTest::newRow("beyond-CIF") << (864 * 64 + 1);
    }
    void invalidFragment() {
        QFETCH(int, fragment);
        std::atomic<int> frames{0};
        QVERIFY_EXCEPTION_THROWN(DabAudio(fragment, 8, ProtectionSettings{},
            std::make_unique<CountFrames>(frames)), std::invalid_argument);
    }
    void largeLegalFragment() {
        // 576 CUs / EEP-A level 1 is 384 kbit/s, within a CIF (864 CUs).
        // 576 * 64 cannot be represented by the old int16_t fragment size.
        constexpr int fragment = 576 * 64;
        ProtectionSettings settings;
        settings.eepLevel = EEPProtectionLevel::EEP_1;
        std::atomic<int> frames{0};
        DabAudio audio(fragment, 384, settings, std::make_unique<CountFrames>(frames));
        const std::vector<softbit_t> bits(fragment, 32);
        for (int i = 0; i < 17; ++i) QCOMPARE(audio.process(bits.data(), fragment), fragment);
        QTRY_COMPARE_WITH_TIMEOUT(frames.load(), 1, 5000);
        DabAudioTestAccess::stop(audio);
        QCOMPARE(DabAudioTestAccess::pending(audio), 0);
    }
    void invalidSubmission_data() {
        QTest::addColumn<int>("count");
        QTest::addColumn<bool>("nullPointer");
        QTest::newRow("oversized-count") << (864 * 64 + 1) << false;
        QTest::newRow("negative-count") << -1 << false;
        QTest::newRow("minimum-count") << -32768 << false;
        QTest::newRow("null-data") << 1 << true;
        QTest::newRow("empty-null-data") << 0 << true;
    }
    void invalidSubmission() {
        QFETCH(int, count);
        QFETCH(bool, nullPointer);
        Fixture f;
        DabAudioTestAccess::prepareDelayedWorker(f.audio);
        QCOMPARE(f.audio.process(nullPointer ? nullptr : f.bits.data(), int32_t(count)), 0);
        QCOMPARE(DabAudioTestAccess::pending(f.audio), 0);
        // A rejected submission must not corrupt subsequent valid input.
        QCOMPARE(f.audio.process(f.bits.data(), 1), 1);
        QCOMPARE(DabAudioTestAccess::pending(f.audio), 1);
    }

    void returnsAcceptedCount() {
        Fixture f;
        DabAudioTestAccess::prepareDelayedWorker(f.audio);
        QCOMPARE(f.audio.process(f.bits.data(), 1), 1);
        QCOMPARE(f.audio.process(f.bits.data(), 0), 0);
        QCOMPARE(DabAudioTestAccess::pending(f.audio), 1);
    }

    void exactFragmentIsConsumed() {
        Fixture f;
        f.push(17); // Existing interleaver warm-up is 16 fragments.
        QTRY_COMPARE_WITH_TIMEOUT(f.frames.load(), 1, 2000);
        DabAudioTestAccess::stop(f.audio);
        QCOMPARE(DabAudioTestAccess::pending(f.audio), 0);
    }

    void partialFragmentWaitsForRemainder() {
        Fixture f;
        f.push(16);
        f.audio.process(f.bits.data(), Fixture::fragment - 1);
        QTest::qWait(20);
        QCOMPARE(f.frames.load(), 0);
        f.audio.process(f.bits.data() + Fixture::fragment - 1, 1);
        QTRY_COMPARE_WITH_TIMEOUT(f.frames.load(), 1, 2000);
        DabAudioTestAccess::stop(f.audio);
        QCOMPARE(DabAudioTestAccess::pending(f.audio), 0);
    }

    void publicationUsesWaitMutex() {
        Fixture f;
        std::unique_lock<std::mutex> lock(DabAudioTestAccess::mutex(f.audio));
        std::promise<void> entered;
        auto producer = std::async(std::launch::async, [&] {
            entered.set_value();
            return f.audio.process(f.bits.data(), 1);
        });
        entered.get_future().wait();
        const auto waiting = producer.wait_for(50ms);
        const int pending = DabAudioTestAccess::pending(f.audio);
        lock.unlock();
        const auto finished = producer.wait_for(2s);
        DabAudioTestAccess::stop(f.audio);
        QVERIFY(finished == std::future_status::ready);
        QVERIFY(producer.get() > 0);
        QVERIFY(waiting == std::future_status::timeout);
        QCOMPARE(pending, 0);
    }

    void exactFreeSpaceIsUsable() {
        Fixture f;
        DabAudioTestAccess::prepareDelayedWorker(f.audio);
        const int capacity = DabAudioTestAccess::leaveSpace(f.audio, Fixture::fragment);
        auto producer = std::async(std::launch::async, [&] {
            return f.audio.process(f.bits.data(), Fixture::fragment);
        });
        const auto finished = producer.wait_for(1s);
        DabAudioTestAccess::stop(f.audio); // Also releases a faulty producer.
        DabAudioTestAccess::wakeAll(f.audio);
        const int result = producer.get();
        QVERIFY(finished == std::future_status::ready);
        QCOMPARE(result, Fixture::fragment);
        QCOMPARE(DabAudioTestAccess::pending(f.audio), capacity);
    }

    void fullQueueWakesWhenWorkerMakesSpace() {
        Fixture f;
        DabAudioTestAccess::prepareDelayedWorker(f.audio);
        DabAudioTestAccess::leaveSpace(f.audio, 0);
        auto producer = std::async(std::launch::async, [&] {
            return f.audio.process(f.bits.data(), Fixture::fragment);
        });
        const auto waiting = producer.wait_for(50ms);
        DabAudioTestAccess::start(f.audio);
        const auto finished = producer.wait_for(2s);
        DabAudioTestAccess::stop(f.audio);
        DabAudioTestAccess::wakeAll(f.audio);
        const int result = producer.get();
        QVERIFY(waiting == std::future_status::timeout);
        QVERIFY(finished == std::future_status::ready);
        QCOMPARE(result, Fixture::fragment);
    }

    void stopReleasesProducerAndRejectsNewData() {
        Fixture f;
        DabAudioTestAccess::prepareDelayedWorker(f.audio);
        const int capacity = DabAudioTestAccess::leaveSpace(f.audio, 0);
        auto producer = std::async(std::launch::async, [&] {
            return f.audio.process(f.bits.data(), Fixture::fragment);
        });
        const auto waiting = producer.wait_for(50ms);
        DabAudioTestAccess::stop(f.audio);
        const auto finished = producer.wait_for(1s);
        DabAudioTestAccess::wakeAll(f.audio); // Cleanup after a failed assertion.
        const int result = producer.get();
        QVERIFY(waiting == std::future_status::timeout);
        QVERIFY(finished == std::future_status::ready);
        QCOMPARE(result, 0);
        QCOMPARE(f.audio.process(f.bits.data(), 1), 0);
        QCOMPARE(DabAudioTestAccess::pending(f.audio), capacity);
    }

    void stoppedEmptyQueueRejectsData() {
        Fixture f;
        DabAudioTestAccess::stop(f.audio);
        QCOMPARE(f.audio.process(f.bits.data(), Fixture::fragment), 0);
        QCOMPARE(DabAudioTestAccess::pending(f.audio), 0);
        QCOMPARE(f.frames.load(), 0);
    }

    void dataBeforeWorkerStarts() {
        Fixture f;
        DabAudioTestAccess::prepareDelayedWorker(f.audio);
        f.push(17);
        DabAudioTestAccess::start(f.audio);
        QTRY_COMPARE_WITH_TIMEOUT(f.frames.load(), 1, 2000);
    }

    void spuriousWakeupsDoNotDecode() {
        Fixture f;
        for (int i = 0; i < 100; ++i) DabAudioTestAccess::wakeAll(f.audio);
        QTest::qWait(20);
        DabAudioTestAccess::stop(f.audio);
        QCOMPARE(f.frames.load(), 0);
        QCOMPARE(DabAudioTestAccess::pending(f.audio), 0);
    }

    void stopUsesWaitMutex() {
        Fixture f;
        std::unique_lock<std::mutex> lock(DabAudioTestAccess::mutex(f.audio));
        std::promise<void> entered;
        auto stop = std::async(std::launch::async, [&] {
            entered.set_value();
            DabAudioTestAccess::stop(f.audio);
        });
        entered.get_future().wait();
        const auto waiting = stop.wait_for(50ms);
        const bool stillRunning = DabAudioTestAccess::running(f.audio);
        lock.unlock();
        const auto finished = stop.wait_for(2s);
        DabAudioTestAccess::wakeAll(f.audio);
        stop.get();
        QVERIFY(waiting == std::future_status::timeout);
        QVERIFY(stillRunning);
        QVERIFY(finished == std::future_status::ready);
    }

    void repeatedIdleShutdown() {
        for (int i = 0; i < 100; ++i) {
            Fixture f;
            DabAudioTestAccess::stop(f.audio);
        }
    }
};

QTEST_GUILESS_MAIN(TestDabAudio)
#include "tst_dabaudio.moc"
