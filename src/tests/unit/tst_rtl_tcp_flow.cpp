/*
 * RTL-TCP byte conservation, reset and bounded clock tracking tests.
 * This file is part of welle.io and was generated with the help of an AI
 * assistant (OpenAi Codex 6 Astra).
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
#include <QTcpServer>
#include <QTcpSocket>
#include <atomic>
#include <chrono>
#include <future>
#include <limits>
#include <thread>
#include "input/rtl_tcp.h"
#include "input/iq_transfer.h"
#include "input/iq_buffer_pacer.h"
#include "null_radio_controller.h"
#include "expectations.h"

namespace {
using Clock = std::chrono::steady_clock;
uint8_t sampleByte(uint64_t index) {
    uint32_t x = uint32_t(index / 2) * 2654435761u;
    x ^= x >> 16;
    return uint8_t(x >> ((index & 1) * 8));
}
// Qt sockets stay on their owning thread. Bounded waits allow cleanup even
// after a failed assertion or a client that stops reading.
class Source {
public:
    std::atomic<bool> running{true};
    std::atomic<bool> failed{false};
    std::atomic<bool> commandsSeen{false};
    std::thread thread;
    uint16_t port = 0;
    ~Source() { running = false; if (thread.joinable()) thread.join(); }
    void start(uint64_t byteLimit = 0, bool withHeader = true, double ppm = 0) {
        std::promise<uint16_t> ready;
        auto result = ready.get_future();
        thread = std::thread([this, byteLimit, withHeader, ppm, ready = std::move(ready)]() mutable {
            QTcpServer listener;
            if (!listener.listen(QHostAddress::LocalHost, 0)) {
                ready.set_value(0); return;
            }
            ready.set_value(listener.serverPort());
            while (running && !listener.hasPendingConnections())
                listener.waitForNewConnection(20);
            if (!running) return;
            std::unique_ptr<QTcpSocket> socket(listener.nextPendingConnection());
            if (!socket) { failed = true; return; }
            // Deliberately split RTL0 metadata across writes.
            const char header[12] = {'R','T','L','0',0,0,0,5,0,0,0,29};
            if (withHeader) {
                socket->write(header, 5); socket->waitForBytesWritten(100);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                socket->write(header + 5, 7);
            }
            uint64_t sent = 0;
            const auto start = Clock::now();
            const double rate = 2.0 * INPUT_RATE * (1 + ppm / 1e6);
            QByteArray bytes(8192, 0);
            while (running && socket->state() == QAbstractSocket::ConnectedState) {
                if (byteLimit && sent >= byteLimit) {
                    socket->waitForReadyRead(20);
                    if (!socket->readAll().isEmpty()) commandsSeen = true;
                    continue;
                }
                for (int i = 0; i < bytes.size(); ++i) bytes[i] = char(sampleByte(sent + i));
                if (socket->write(bytes) != bytes.size()) { failed = true; break; }
                while (running && socket->bytesToWrite() > 0) {
                    socket->waitForBytesWritten(20);
                    if (socket->state() != QAbstractSocket::ConnectedState) break;
                }
                if (!socket->readAll().isEmpty()) commandsSeen = true;
                socket->waitForReadyRead(0);
                sent += bytes.size();
                std::this_thread::sleep_until(start + std::chrono::duration_cast<Clock::duration>(
                    std::chrono::duration<double>(sent / rate)));
            }
            socket->abort();
        });
        port = result.get();
    }
};
}

class RtlTcpFlowTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { hideDebugOutput(); }

    void fullDecoderPreservesBytes() {
        RingBuffer<uint8_t> source(128), destination(16);
        uint8_t bytes[128], scratch[32], read[16];
        for (int i = 0; i < 128; ++i) bytes[i] = uint8_t(i);
        QCOMPARE(source.putDataIntoBuffer(bytes, 128), 128);
        QCOMPARE(transferIQ(source, destination, scratch, 32), 16);
        for (int i = 0; i < 100; ++i)
            QCOMPARE(transferIQ(source, destination, scratch, 32), 0);
        QCOMPARE(source.GetRingBufferReadAvailable(), 112);
        int consumed = 0;
        while (consumed < 128) {
            const int count = destination.getDataFromBuffer(read, 16);
            QVERIFY(count > 0);
            for (int i = 0; i < count; ++i) QCOMPARE(read[i], bytes[consumed + i]);
            consumed += count;
            transferIQ(source, destination, scratch, 32);
        }
        QCOMPARE(source.GetRingBufferReadAvailable(), 0);
    }

    void completePairsOnly() {
        RingBuffer<uint8_t> source(8), destination(8);
        uint8_t data[8] = {}, scratch[8];
        source.putDataIntoBuffer(data, 8);
        destination.putDataIntoBuffer(data, 7);
        QCOMPARE(transferIQ(source, destination, scratch, 8), 0);
        QCOMPARE(source.GetRingBufferReadAvailable(), 8);
        destination.FlushRingBuffer();
        QCOMPARE(transferIQ(source, destination, scratch, 3), 2);
        QCOMPARE(source.GetRingBufferReadAvailable(), 6);
    }

    void pendingReceiveCanBeCancelled_data() {
        QTest::addColumn<bool>("reset");
        QTest::newRow("reset") << true;
        QTest::newRow("stop") << false;
    }

    void pendingReceiveCanBeCancelled() {
        QFETCH(bool, reset);
        Source server;
        server.start(8192);
        QVERIFY(server.port != 0);
        std::atomic<bool> finished{false}, failed{false};
        NullRadioController controller;
        CRTL_TCP_Client input(controller);
        input.setAgc(false);
        input.connected = input.sock.connect("127.0.0.1", server.port, 2);
        QVERIFY(input.connected);
        input.rtlsdrRunning = true;

        // Do not start the copier: the receive reservoir must stay full.
        std::vector<uint8_t> full(input.sampleNetworkBuffer.GetBufferSize(), 0x7e);
        QCOMPARE(input.sampleNetworkBuffer.putDataIntoBuffer(full.data(), full.size()),
                 int(full.size()));
        input.receiveThread = std::thread([&] {
            try { input.receiveData(); }
            catch (...) { failed = true; }
            finished = true;
        });

        // Header-triggered commands prove that receiveData captured its
        // generation and read a complete block. It cannot publish that block
        // into the full reservoir. This avoids a timing-only sleep assertion.
        QTRY_VERIFY_WITH_TIMEOUT(server.commandsSeen.load(), 2000);
        QVERIFY(!finished);
        if (reset) {
            input.reset();
            QTRY_VERIFY_WITH_TIMEOUT(finished.load(), 2000);
            std::lock_guard<std::mutex> lock(input.bufferTransferMutex);
            QCOMPARE(input.sampleNetworkBuffer.GetRingBufferReadAvailable(), 0);
            QCOMPARE(input.sampleBuffer.GetRingBufferReadAvailable(), 0);
        }

        const auto start = Clock::now();
        input.stop();
        QVERIFY(Clock::now() - start < std::chrono::seconds(2));
        QVERIFY(finished);
        QVERIFY(!failed);
        QVERIFY(!server.failed);
    }

    void incompleteHeaderDisconnect_data() {
        QTest::addColumn<int>("length");
        for (int length : {0, 1, 3, 5, 11})
            QTest::newRow(qPrintable(QString::number(length))) << length;
    }
    void incompleteHeaderDisconnect() {
        QFETCH(int, length);
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        NullRadioController controller;
        CRTL_TCP_Client input(controller);
        input.connected = input.sock.connect("127.0.0.1", server.serverPort(), 2);
        QVERIFY(input.connected);
        QVERIFY(server.waitForNewConnection(2000));
        std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
        QVERIFY(peer != nullptr);
        const QByteArray header = QByteArray::fromHex("52544c30000000050000001d");
        if (length) {
            QCOMPARE(peer->write(header.constData(), length), qint64(length));
            QVERIFY(peer->waitForBytesWritten(2000));
        }
        peer->disconnectFromHost();
        input.rtlsdrRunning = true;
        std::atomic<bool> finished{false}, failed{false};
        input.receiveThread = std::thread([&] {
            try { input.receiveData(); } catch (...) { failed = true; }
            finished = true;
        });
        QTRY_VERIFY_WITH_TIMEOUT(finished.load(), 2000);
        input.stop();
        QVERIFY(!failed);
        QVERIFY(!input.connected);
        QCOMPARE(input.sampleNetworkBuffer.GetRingBufferReadAvailable(), 0);
        QCOMPARE(input.sampleBuffer.GetRingBufferReadAvailable(), 0);
    }

    void pausedDecoderPreservesBytes_data() {
        QTest::addColumn<bool>("withHeader");
        QTest::newRow("rtl0-header") << true;
        QTest::newRow("headerless") << false;
    }
    void pausedDecoderPreservesBytes() {
        QFETCH(bool, withHeader);
        Source server; server.start(0, withHeader);
        QVERIFY(server.port != 0);
        NullRadioController controller;
        CRTL_TCP_Client input(controller);
        input.setServerAddress("127.0.0.1"); input.setPort(server.port);
        input.setAgc(false);
        QVERIFY(input.restart());
        // A decoder pause fills both client buffers and exercises receive-side
        // backpressure as well as transfer-side capacity checks.
        std::this_thread::sleep_for(std::chrono::seconds(4));
        uint64_t consumed = 0;
        std::vector<DSPCOMPLEX> samples(32768);
        const auto until = Clock::now() + std::chrono::seconds(5);
        while (Clock::now() < until) {
            const int count = input.getSamples(samples.data(), samples.size());
            for (int i = 0; i < count; ++i) {
                const float re = (float(sampleByte(consumed)) - 128) / 128;
                const float im = (float(sampleByte(consumed + 1)) - 128) / 128;
                if (samples[i].real() != re || samples[i].imag() != im)
                    QFAIL(qPrintable(QString("IQ mismatch at byte %1").arg(consumed)));
                consumed += 2;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        QVERIFY(consumed > 2 * INPUT_RATE);
        QVERIFY(!server.failed);
    }
    void clockSimulation_data() {
        QTest::addColumn<double>("ppm");
        for (double ppm : {-5000., -500., -47.569, 0., 47.569, 500., 5000.})
            QTest::newRow(qPrintable(QString::number(ppm))) << ppm;
    }
    void clockSimulation() {
        QFETCH(double, ppm);
        const double capacity = 8388608, nominal = 4096000;
        IQBufferPacer pacer(nominal, capacity);
        double queue = capacity / 2, elapsed = 0, low = queue, high = queue;
        for (uint64_t step = 0; elapsed < 86400; ++step) {
            queue -= 65536;
            double dt = pacer.secondsFor(65536, size_t(std::clamp(queue, 0.0, capacity)));
            // A 100 ms scheduling delay followed by catch-up over ten ticks.
            if (step % 1000 == 0) dt += .1;
            if (step % 1000 >= 1 && step % 1000 <= 10) dt -= .01;
            queue += nominal * (1 + ppm / 1e6) * dt;
            low = std::min(low, queue); high = std::max(high, queue);
            elapsed += dt;
        }
        QVERIFY2(low > 3000000 && high < 5500000,
                 qPrintable(QString("reservoir %1 .. %2").arg(low).arg(high)));
    }

    void correctionIsBounded() {
        IQBufferPacer pacer(4096000, 8388608);
        QCOMPARE(pacer.bytesPerSecond(0), 4096000 * .99);
        QCOMPARE(pacer.bytesPerSecond(8388608), 4096000 * 1.01);
        QCOMPARE(pacer.bytesPerSecond(4194304), 4096000.0);
        QVERIFY(pacer.secondsFor(2, 4194304) > 0);
        QCOMPARE(pacer.bytesPerSecond(std::numeric_limits<size_t>::max()), 4096000 * 1.01);
        QVERIFY(std::isfinite(pacer.secondsFor(65536, std::numeric_limits<size_t>::max())));
    }

    void outsideControlRange_data() {
        QTest::addColumn<double>("ppm");
        for (double ppm : {-20000., -10100., 10100., 20000.})
            QTest::newRow(qPrintable(QString::number(ppm))) << ppm;
    }
    void outsideControlRange() {
        QFETCH(double, ppm);
        const double capacity = 8388608, nominal = 4096000;
        IQBufferPacer pacer(nominal, capacity);
        double queue = capacity / 2, elapsed = 0;
        // No sender backpressure in this model. Stop at the first physical
        // boundary: pacing cannot repair a sustained source beyond +/-1%.
        while (queue >= 65536 && queue <= capacity && elapsed < 36000) {
            queue -= 65536;
            const size_t fill = size_t(std::clamp(queue, 0.0, capacity));
            const double rate = pacer.bytesPerSecond(fill);
            QVERIFY(rate >= nominal * .99 && rate <= nominal * 1.01);
            const double dt = pacer.secondsFor(65536, fill);
            QVERIFY(std::isfinite(dt) && dt > 0);
            queue += nominal * (1 + ppm / 1e6) * dt;
            elapsed += dt;
        }
        QVERIFY(elapsed < 36000);
        QVERIFY(ppm > 0 ? queue > capacity : queue < 65536);
    }

    void clockDrift_data() {
        QTest::addColumn<double>("ppm");
        QTest::newRow("slow-source") << -5000.0;
        QTest::newRow("fast-source") << 5000.0;
    }
    void clockDrift() {
        QFETCH(double, ppm);
        Source server; server.start(0, true, ppm);
        QVERIFY(server.port != 0);
        NullRadioController controller;
        CRTL_TCP_Client input(controller);
        input.setServerAddress("127.0.0.1"); input.setPort(server.port);
        input.setAgc(false);
        QVERIFY(input.restart());
        uint64_t consumed = 0;
        std::vector<DSPCOMPLEX> samples(32768);
        const auto until = Clock::now() + std::chrono::seconds(40);
        int minFill = 8388608, maxFill = 0;
        double totalFill = 0;
        int observations = 0;
        const auto measuring = Clock::now() + std::chrono::seconds(30);
        while (Clock::now() < until) {
            const int count = input.getSamples(samples.data(), samples.size());
            for (int i = 0; i < count; ++i) {
                const float re = (float(sampleByte(consumed)) - 128) / 128;
                const float im = (float(sampleByte(consumed + 1)) - 128) / 128;
                if (samples[i].real() != re || samples[i].imag() != im)
                    QFAIL(qPrintable(QString("IQ mismatch at byte %1").arg(consumed)));
                consumed += 2;
            }
            if (Clock::now() >= measuring) {
                std::lock_guard<std::mutex> lock(input.bufferTransferMutex);
                const int fill = input.sampleNetworkBuffer.GetRingBufferReadAvailable();
                minFill = std::min(minFill, fill); maxFill = std::max(maxFill, fill);
                totalFill += fill; ++observations;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        QVERIFY(consumed > 2 * INPUT_RATE);
        QVERIFY(!server.failed);
        {
            QVERIFY(observations > 0);
            const double mean = totalFill / observations;
            // Equilibrium: target + (sourceRate - nominalRate) * 10 seconds.
            // At +/-5000 ppm that is 4194304 +/- 204800 bytes. These margins
            // depend on the 10 s feedback constant and the +/-1% rate limit.
            // TCP delivery and thread scheduling are bursty. Check clock
            // tracking over the final ten seconds, with separate bounds for
            // transient fill rather than treating each burst as clock drift.
            QVERIFY2(mean > 3800000 && mean < 4600000 &&
                     minFill > 3000000 && maxFill < 5500000,
                     qPrintable(QString("reservoir %1 .. %2, mean %3")
                                .arg(minFill).arg(maxFill).arg(mean)));
            qInfo("Reservoir: min=%d max=%d mean=%.0f; checked %llu bytes",
                  minFill, maxFill, mean, static_cast<unsigned long long>(consumed));
        }
    }
    void initialTuneDiscardsSettlingSamples() {
        Source server; server.start();
        QVERIFY(server.port != 0);
        NullRadioController controller;
        CRTL_TCP_Client input(controller);
        input.setServerAddress("127.0.0.1"); input.setPort(server.port);
        input.setAgc(false); input.setFrequency(174928000);
        const size_t discarded = size_t(input.settleMs) * INPUT_RATE * 2 / 1000;
        QVERIFY(input.restart());
        QTRY_VERIFY_WITH_TIMEOUT(input.getSamplesToRead() >= 1024, 4000);
        std::vector<DSPCOMPLEX> samples(1024);
        QCOMPARE(input.getSamples(samples.data(), samples.size()), int(samples.size()));
        for (size_t i=0; i<samples.size(); ++i) {
            QCOMPARE(samples[i].real(), (float(sampleByte(discarded+2*i))-128)/128);
            QCOMPARE(samples[i].imag(), (float(sampleByte(discarded+2*i+1))-128)/128);
        }
        QVERIFY(!server.failed);
    }
    void connectedRetune_data() {
        QTest::addColumn<bool>("blocked");
        QTest::newRow("active") << false;
        QTest::newRow("decoder-blocked") << true;
    }
    void connectedRetune() {
        QFETCH(bool, blocked);
        Source server; server.start();
        QVERIFY(server.port != 0);
        std::atomic<bool> finished{false}, failed{false};
        NullRadioController controller;
        CRTL_TCP_Client input(controller);
        input.setAgc(false);
        input.connected = input.sock.connect("127.0.0.1", server.port, 2);
        QVERIFY(input.connected);
        input.rtlsdrRunning = true;
        if (blocked) {
            std::vector<uint8_t> full(input.sampleNetworkBuffer.GetBufferSize(), 0x7e);
            QCOMPARE(input.sampleNetworkBuffer.putDataIntoBuffer(full.data(), full.size()), int(full.size()));
        }
        input.receiveThread = std::thread([&] {
            try { input.receiveData(); } catch (...) { failed = true; }
            finished = true;
        });
        QTRY_VERIFY_WITH_TIMEOUT(server.commandsSeen.load(), 2000);
        if (blocked) QVERIFY(!finished);
        else {
            QTRY_VERIFY_WITH_TIMEOUT(finished.load(), 2000);
            QCOMPARE(input.sampleNetworkBuffer.GetRingBufferReadAvailable(), 8192-12);
        }
        input.setFrequency(229072000); input.reset();
        QTRY_VERIFY_WITH_TIMEOUT(finished.load(), 2000);
        input.receiveThread.join();
        QVERIFY(!failed);
        QCOMPARE(input.sampleNetworkBuffer.GetRingBufferReadAvailable(), 0);
        const size_t discarded = size_t(input.settleMs) * INPUT_RATE * 2 / 1000;
        QCOMPARE(input.discardBytes.load(), discarded);
        finished = false;
        input.receiveThread = std::thread([&] {
            try {
                while (input.rtlsdrRunning) {
                    input.receiveData();
                    std::lock_guard<std::mutex> lock(input.bufferTransferMutex);
                    if (input.sampleNetworkBuffer.GetRingBufferReadAvailable() > 0) break;
                }
            } catch (...) { failed = true; }
            finished = true;
        });
        QTRY_VERIFY_WITH_TIMEOUT(finished.load(), 3000);
        input.receiveThread.join();
        QVERIFY(!failed);
        QCOMPARE(input.discardBytes.load(), size_t(0));
        uint8_t scratch[8192];
        const int bytes = transferIQ(input.sampleNetworkBuffer, input.sampleBuffer, scratch, sizeof(scratch));
        QVERIFY(bytes > 0);
        std::vector<DSPCOMPLEX> samples(bytes/2);
        QCOMPARE(input.getSamples(samples.data(), samples.size()), int(samples.size()));
        // One block was consumed before the reset, including its 12-byte header.
        const size_t first = 8192-12+discarded;
        for (size_t i=0; i<samples.size(); ++i) {
            QCOMPARE(samples[i].real(), (float(sampleByte(first+2*i))-128)/128);
            QCOMPARE(samples[i].imag(), (float(sampleByte(first+2*i+1))-128)/128);
        }
        QVERIFY(!server.failed);

    }
};
QTEST_GUILESS_MAIN(RtlTcpFlowTests)
#include "tst_rtl_tcp_flow.moc"
