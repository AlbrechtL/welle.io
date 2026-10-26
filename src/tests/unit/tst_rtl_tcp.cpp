/*
 *    This file is part of the welle.io.
 *    This file was generated with the help of an AI assistant (Claude Code).
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// unit.rtl_tcp: the rtl_tcp client against a minimal rtl_tcp server on
// localhost. Like a real server (buffers of tuner, server and network), the
// server sends samples of the old frequency for another 100 ms after a
// retune. The client must not pass them on: a much stronger previous channel
// made the channel scan miss a weak one (10A after 9D).

#include <QtTest>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

#include "expectations.h"
#include "null_radio_controller.h"
#include "rtl_tcp.h"
#include "various/Socket.h"

namespace {

using Clock = std::chrono::steady_clock;

constexpr int oldFrequency = 208064000;  // 9D
constexpr int newFrequency = 209936000;  // 10A
constexpr uint8_t oldValue = 200;
constexpr uint8_t newValue = 60;
constexpr auto staleTime = std::chrono::milliseconds(100);

class FakeRtlTcpServer {
public:
    bool start()
    {
        for (port = 41234; port < 41334; port++) {
            if (listener.bind(port) && listener.listen()) {
                thread = std::thread(&FakeRtlTcpServer::run, this);
                return true;
            }
            listener.close();
        }
        return false;
    }

    ~FakeRtlTcpServer()
    {
        running = false;
        listener.shutdown();
        client.shutdown();
        if (thread.joinable())
            thread.join();
        if (commandThread.joinable())
            commandThread.join();
    }

    int port = 0;

private:
    void run()
    {
        client = listener.accept();
        if (!client.valid())
            return;
        commandThread = std::thread(&FakeRtlTcpServer::readCommands, this);

        // Dongle info: magic, tuner type R820T, 29 gains (network byte order)
        const uint8_t header[12] = { 'R', 'T', 'L', '0', 0, 0, 0, 5, 0, 0, 0, 29 };
        client.send(header, sizeof(header), MSG_NOSIGNAL);

        // 8 bit I/Q in real time
        std::vector<uint8_t> chunk(8192);
        const auto start = Clock::now();
        uint64_t sent = 0;
        while (running) {
            const bool retuned = retunedAt.load() != Clock::time_point() &&
                Clock::now() - retunedAt.load() > staleTime;
            std::fill(chunk.begin(), chunk.end(), retuned ? newValue : oldValue);
            if (client.send(chunk.data(), chunk.size(), MSG_NOSIGNAL) <= 0)
                break;
            sent += chunk.size();
            std::this_thread::sleep_until(start + std::chrono::microseconds(
                    sent * 1000000 / (2 * INPUT_RATE)));
        }
    }

    void readCommands()
    {
        uint8_t command[5];
        size_t have = 0;
        while (running) {
            const ssize_t ret = client.recv(command + have, sizeof(command) - have, 0);
            if (ret <= 0)
                break;
            have += ret;
            if (have < sizeof(command))
                continue;
            have = 0;
            const uint32_t param = (uint32_t(command[1]) << 24) | (command[2] << 16) |
                                   (command[3] << 8) | command[4];
            if (command[0] == 0x01 && param == uint32_t(newFrequency) &&
                    retunedAt.load() == Clock::time_point())
                retunedAt = Clock::now();
        }
    }

    Socket listener;
    Socket client;
    std::thread thread;
    std::thread commandThread;
    std::atomic<bool> running{true};
    std::atomic<Clock::time_point> retunedAt{Clock::time_point()};
};

} // namespace

class RtlTcpTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { hideDebugOutput(); }
    void dropsSamplesOfOldFrequency();
};

void RtlTcpTests::dropsSamplesOfOldFrequency()
{
    FakeRtlTcpServer server;
    QVERIFY2(server.start(), "No free port for the rtl_tcp server");

    NullRadioController rc;
    CRTL_TCP_Client input(rc);
    input.setServerAddress("127.0.0.1");
    input.setPort(server.port);
    input.setFrequency(oldFrequency);
    QVERIFY2(input.restart(), "No connection to the rtl_tcp server");

    // Samples of the old frequency arrive (after the network buffer is
    // filled)
    std::vector<DSPCOMPLEX> buffer(8192);
    const float oldSample = (float(oldValue) - 128) / 128;
    const float newSample = (float(newValue) - 128) / 128;
    bool oldSeen = false;
    for (auto t = Clock::now(); !oldSeen && Clock::now() - t < std::chrono::seconds(5); ) {
        const int32_t n = input.getSamples(buffer.data(), buffer.size());
        for (int32_t i = 0; i < n; i++)
            oldSeen |= buffer[i].real() == oldSample;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    QVERIFY2(oldSeen, "No samples received");

    // Retune like CRadioController::setChannel()
    input.setFrequency(newFrequency);
    input.reset();

    int64_t oldSamples = 0;
    int64_t newSamples = 0;
    for (auto t = Clock::now(); newSamples < INPUT_RATE / 2 &&
            Clock::now() - t < std::chrono::seconds(5); ) {
        const int32_t n = input.getSamples(buffer.data(), buffer.size());
        for (int32_t i = 0; i < n; i++) {
            oldSamples += buffer[i].real() == oldSample;
            newSamples += buffer[i].real() == newSample;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    QVERIFY2(newSamples >= INPUT_RATE / 2, "No samples of the new frequency");
    QVERIFY2(oldSamples == 0,
             qPrintable(QString("%1 ms of the old frequency after the retune")
                        .arg(1000.0 * oldSamples / INPUT_RATE)));
}

QTEST_GUILESS_MAIN(RtlTcpTests)
#include "tst_rtl_tcp.moc"
