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

// hw.v4l2: the Linux kernel SDR input with the real devices (/dev/swradioN).
// Runs only with WELLE_TEST_V4L2=1, because it needs the hardware and the
// devices must not be used by another program. Needs no antenna: it checks
// the sample rate, retuning, gain changes (in the noise level) and
// restarting, not the reception.

#include <QtTest>

#include <chrono>
#include <cmath>
#include <mutex>
#include <thread>
#include <vector>

#include "null_radio_controller.h"
#include "v4l2_sdr.h"

namespace {

class RecordingController : public NullRadioController {
public:
    void onMessage(message_level_t level, const std::string& text, const std::string& = std::string()) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (level == message_level_t::Error)
            errors.push_back(text);
    }
    std::vector<std::string> errorList()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return errors;
    }

private:
    std::mutex mutex;
    std::vector<std::string> errors;
};

struct Capture {
    double rate = 0;     // samples per second
    double levelDb = 0;  // mean power, dB relative to full scale
};

// Reads the samples of the device for the given time
Capture capture(CV4L2Sdr& device, double seconds)
{
    std::vector<DSPCOMPLEX> buffer(65536);
    uint64_t count = 0;
    double power = 0;

    // Empty the buffer, then count from now
    while (device.getSamples(buffer.data(), int32_t(buffer.size())) > 0) {}
    const auto start = std::chrono::steady_clock::now();
    double elapsed = 0;
    while (elapsed < seconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        int32_t n;
        while ((n = device.getSamples(buffer.data(), int32_t(buffer.size()))) > 0) {
            count += n;
            for (int32_t i = 0; i < n; i++)
                power += std::norm(buffer[i]);
        }
        elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    }

    Capture c;
    c.rate = count / elapsed;
    c.levelDb = count > 0 ? 10 * std::log10(power / count + 1e-20) : -200;
    return c;
}

} // namespace

class V4L2DeviceTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void listsDevices();
    void streams_data();
    void streams();
    void rejectsUnknownDevice();
};

void V4L2DeviceTests::initTestCase()
{
    if (qEnvironmentVariableIntValue("WELLE_TEST_V4L2") != 1)
        QSKIP("Set WELLE_TEST_V4L2=1 to test the Linux kernel SDR devices");
    if (CV4L2Sdr::listDevices().empty())
        QSKIP("No Linux kernel SDR device (/dev/swradioN)");
}

void V4L2DeviceTests::listsDevices()
{
    for (const auto& info : CV4L2Sdr::listDevices()) {
        qInfo() << "Device" << info.path.c_str() << info.card.c_str() << info.driver.c_str();
        QVERIFY(QString::fromStdString(info.path).startsWith("/dev/swradio"));
        QVERIFY(!info.driver.empty());
    }
}

void V4L2DeviceTests::streams_data()
{
    QTest::addColumn<QString>("path");
    for (const auto& info : CV4L2Sdr::listDevices())
        QTest::newRow(qPrintable(QString::fromStdString(info.driver + " " + info.path)))
            << QString::fromStdString(info.path);
}

void V4L2DeviceTests::streams()
{
    QFETCH(QString, path);
    RecordingController controller;
    CV4L2Sdr device(controller, path.toStdString());
    qInfo() << "Description" << device.getDescription().c_str()
            << "gain steps" << device.getGainCount();

    // 5C, the airspy driver measures its rate in restart()
    device.setFrequency(178352000);
    QVERIFY(device.restart());
    QVERIFY(device.is_ok());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    const Capture first = capture(device, 2.0);
    qInfo() << "Rate" << first.rate << "level" << first.levelDb << "dB";
    QVERIFY2(std::abs(first.rate / INPUT_RATE - 1) < 0.02, qPrintable(QString("rate %1").arg(first.rate)));

    // Retuning while streaming (11D)
    device.setFrequency(222064000);
    QCOMPARE(device.getFrequency(), 222064000);
    const Capture retuned = capture(device, 1.0);
    QVERIFY2(std::abs(retuned.rate / INPUT_RATE - 1) < 0.02, qPrintable(QString("rate %1").arg(retuned.rate)));

    // A higher gain gives a higher noise level, also without antenna
    if (device.getGainCount() > 0) {
        device.setAgc(false);
        device.setGain(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        const Capture low = capture(device, 0.5);
        device.setGain(device.getGainCount());
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        const Capture high = capture(device, 0.5);
        qInfo() << "Level at gain 0" << low.levelDb << "dB, at gain" << device.getGainCount()
                << high.levelDb << "dB";
        QVERIFY2(high.levelDb > low.levelDb + 6,
                 qPrintable(QString("%1 dB -> %2 dB").arg(low.levelDb).arg(high.levelDb)));
    }

    // Stop and start again
    device.stop();
    QVERIFY(device.restart());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    const Capture restarted = capture(device, 1.0);
    QVERIFY2(std::abs(restarted.rate / INPUT_RATE - 1) < 0.02, qPrintable(QString("rate %1").arg(restarted.rate)));
    device.stop();

    QVERIFY(device.is_ok());
    const auto errors = controller.errorList();
    QVERIFY2(errors.empty(), errors.empty() ? "" : errors.front().c_str());
}

void V4L2DeviceTests::rejectsUnknownDevice()
{
    RecordingController controller;
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, CV4L2Sdr(controller, "/dev/swradio-does-not-exist"));
}

QTEST_GUILESS_MAIN(V4L2DeviceTests)
#include "tst_v4l2_device.moc"
