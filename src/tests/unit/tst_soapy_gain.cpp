/*
 * SoapySDR software AGC gain direction regression tests.
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
#include <QTemporaryDir>
#include <memory>
#include <stdexcept>
#include <limits>
#include <SoapySDR/Errors.hpp>
#include "input/soapy_sdr.h"
#include "null_radio_controller.h"

namespace {
SoapySDR::Range nextRange{0, 30, 10};
bool failRangeQuery = false;
int liveDevices = 0;
std::string driverFault;
std::atomic<int> streamActivations{0};

class MessageController : public NullRadioController {
public:
    int errors = 0;
    std::string message;
    void onMessage(message_level_t level, const std::string& text,
                   const std::string& = std::string()) override {
        if (level == message_level_t::Error) ++errors;
        message = text;
    }
};

// A test device records the commands from the actual CSoapySdr methods.
// Stream startup tests use a timeout-only stream; no physical device is opened.
class GainDevice : public SoapySDR::Device {
public:
    GainDevice() { ++liveDevices; }
    ~GainDevice() override { --liveDevices; }
    double gain = 0;
    int writes = 0;
    SoapySDR::Range range = nextRange;
    std::atomic<bool> activated{false};
    double gainAtActivation = -1;

    void setMasterClockRate(double) override {}
    void setSampleRate(int, size_t, double) override {}
    double getSampleRate(int, size_t) const override { return 2048000; }
    void setGainMode(int, size_t, bool) override {
        if (driverFault == "mode") throw std::runtime_error("Test gain-mode failure");
    }
    SoapySDR::Stream* setupStream(int, const std::string&,
        const std::vector<size_t>&, const SoapySDR::Kwargs&) override {
        return reinterpret_cast<SoapySDR::Stream*>(this);
    }
    size_t getStreamMTU(SoapySDR::Stream*) const override { return 16; }
    int activateStream(SoapySDR::Stream*, int, long long, size_t) override {
        ++streamActivations;
        gainAtActivation = gain;
        activated = true;
        return 0;
    }
    int readStream(SoapySDR::Stream*, void* const*, size_t, int&,
        long long&, long) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return SOAPY_SDR_TIMEOUT;
    }
    void closeStream(SoapySDR::Stream*) override {}

    SoapySDR::Range getGainRange(const int, const size_t) const override {
        if (failRangeQuery) throw std::runtime_error("Test driver range failure");
        return range;
    }

    double getGain(const int direction, const size_t channel) const override {
        if (direction != SOAPY_SDR_RX || channel != 0)
            throw std::runtime_error("Unexpected gain channel");
        if (driverFault == "read") throw std::runtime_error("Test gain read failure");
        return gain;
    }

    void setGain(const int direction, const size_t channel, const double value) override {
        if (direction != SOAPY_SDR_RX || channel != 0)
            throw std::runtime_error("Unexpected gain channel");
        if (driverFault == "write") throw std::runtime_error("Test gain write failure");
        if (driverFault == "write-out-of-range") throw std::out_of_range("Test driver range failure");
        gain = value;
        ++writes;
    }
};

SoapySDR::KwargsList findDevice(const SoapySDR::Kwargs&) {
    return {{{"driver", "welle_test_gain"}, {"label", "Gain test device"}}};
}

SoapySDR::Device* makeDevice(const SoapySDR::Kwargs&) {
    return new GainDevice;
}
}

class TestSoapyGain : public QObject {
    Q_OBJECT
private:
    QTemporaryDir emptyRoot;
    std::unique_ptr<SoapySDR::Registry> registry;

private slots:
    void initTestCase() {
        QVERIFY(emptyRoot.isValid());
        // Prevent the input constructor's enumeration from loading installed
        // hardware plugins. Only the in-process test driver is registered.
        QVERIFY(qputenv("SOAPY_SDR_ROOT", emptyRoot.path().toUtf8()));
        QVERIFY(qputenv("SOAPY_SDR_PLUGIN_PATH", ""));
        QVERIFY(SoapySDR::listModules().empty());
        registry.reset(new SoapySDR::Registry("welle_test_gain", findDevice,
                                            makeDevice, SOAPY_SDR_ABI_VERSION));
    }

    void cleanup() {
        nextRange = SoapySDR::Range(0, 30, 10);
        failRangeQuery = false;
        driverFault.clear();
        streamActivations = 0;
        QCOMPARE(liveDevices, 0);
    }

    void gainStep_data() {
        QTest::addColumn<std::vector<double>>("gains");
        QTest::addColumn<double>("current");
        QTest::addColumn<bool>("decrease");
        QTest::addColumn<double>("expected");
        QTest::addColumn<int>("writes");
        const std::vector<double> gains = {0, 10, 20, 30};
        QTest::newRow("decrease-middle") << gains << 20.0 << true << 10.0 << 1;
        QTest::newRow("decrease-maximum") << gains << 30.0 << true << 20.0 << 1;
        QTest::newRow("decrease-minimum") << gains << 0.0 << true << 0.0 << 0;
        QTest::newRow("decrease-between-steps") << gains << 25.0 << true << 20.0 << 1;
        QTest::newRow("decrease-above-table") << gains << 35.0 << true << 30.0 << 1;
        QTest::newRow("decrease-below-table") << gains << -5.0 << true << -5.0 << 0;
        QTest::newRow("increase-middle") << gains << 10.0 << false << 20.0 << 1;
        QTest::newRow("increase-minimum") << gains << 0.0 << false << 10.0 << 1;
        QTest::newRow("increase-maximum") << gains << 30.0 << false << 30.0 << 0;
        QTest::newRow("increase-between-steps") << gains << 15.0 << false << 20.0 << 1;
        const std::vector<double> fractional = {-4.0, -1.5, 0.0, 2.5};
        QTest::newRow("decrease-fractional-negative") << fractional << -1.5 << true << -4.0 << 1;
        QTest::newRow("increase-fractional") << fractional << 0.0 << false << 2.5 << 1;
        QTest::newRow("increase-decimal-maximum") << std::vector<double>{0.0, 1.3} << 1.3 << false << 1.3 << 0;
        QTest::newRow("decrease-decimal-maximum") << std::vector<double>{0.0, 1.3} << 1.3 << true << 0.0 << 1;
        QTest::newRow("increase-decimal-endpoint") << std::vector<double>{0.0, 1.3} << 0.0 << false << 1.3 << 1;
        QTest::newRow("decrease-empty") << std::vector<double>{} << 10.0 << true << 10.0 << 0;
        QTest::newRow("increase-empty") << std::vector<double>{} << 10.0 << false << 10.0 << 0;
        QTest::newRow("decrease-single") << std::vector<double>{10} << 10.0 << true << 10.0 << 0;
        QTest::newRow("increase-single") << std::vector<double>{10} << 10.0 << false << 10.0 << 0;
    }

    void gainStep() {
        QFETCH(std::vector<double>, gains);
        QFETCH(double, current);
        QFETCH(bool, decrease);
        QFETCH(double, expected);
        QFETCH(int, writes);
        NullRadioController controller;
        CSoapySdr input(controller);
        input.m_device = SoapySDR::Device::make("driver=welle_test_gain");
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        QVERIFY(device != nullptr);
        device->gain = current;
        input.m_gains = gains;
        if (decrease) input.decreaseGain();
        else input.increaseGain();
        QCOMPARE(device->gain, expected);
        QCOMPARE(device->writes, writes);
        // input's destructor returns the fake device through Device::unmake.
    }

    void noDevice() {
        NullRadioController controller;
        CSoapySdr input(controller);
        QVERIFY(input.m_device == nullptr);
        input.decreaseGain();
        input.increaseGain();
        QVERIFY(input.m_device == nullptr);
    }

    void gainBeforeOpen_data() {
        QTest::addColumn<bool>("explicitArgs");
        QTest::addColumn<int>("index");
        QTest::addColumn<bool>("automatic");
        QTest::addColumn<double>("expected");
        for (bool args : {false, true}) {
            const QByteArray prefix = args ? "explicit-" : "default-";
            QTest::newRow((prefix+"middle").constData()) << args << 2 << false << 20.0;
            QTest::newRow((prefix+"maximum").constData()) << args << 3 << false << 30.0;
            QTest::newRow((prefix+"minimum").constData()) << args << 0 << false << 0.0;
            QTest::newRow((prefix+"invalid").constData()) << args << 99 << false << 0.0;
            QTest::newRow((prefix+"automatic").constData()) << args << 2 << true << 0.0;
        }
    }

    void gainBeforeOpen() {
        QFETCH(bool, explicitArgs);
        QFETCH(int, index);
        QFETCH(bool, automatic);
        QFETCH(double, expected);
        NullRadioController controller;
        CSoapySdr input(controller);
        input.setGain(index); // Same ordering as the real CLI.
        input.setAgc(automatic);
        if (explicitArgs)
            input.setDeviceParam(DeviceParam::SoapySDRDriverArgs, "driver=welle_test_gain");
        else
            QVERIFY(input.restart());
        for (int run = 0; run < 2; ++run) {
            auto* device = dynamic_cast<GainDevice*>(input.m_device);
            QVERIFY(device != nullptr);
            QTRY_VERIFY_WITH_TIMEOUT(device->activated.load(), 2000);
            QCOMPARE(device->gainAtActivation, expected);
            input.stop();
            if (run == 0) QVERIFY(input.restart());
        }
    }

    void range_data() {
        QTest::addColumn<double>("minimum");
        QTest::addColumn<double>("maximum");
        QTest::addColumn<double>("step");
        QTest::addColumn<std::vector<double>>("expected");
        QTest::newRow("integer") << 0.0 << 6.0 << 2.0 << std::vector<double>{0, 2, 4, 6};
        QTest::newRow("fractional-negative") << -1.5 << 1.5 << 0.5 << std::vector<double>{-1.5, -1, -.5, 0, .5, 1, 1.5};
        QTest::newRow("unspecified-step") << 0.0 << 2.5 << 0.0 << std::vector<double>{0, 1, 2, 2.5};
        QTest::newRow("unaligned-maximum") << 1.0 << 6.0 << 2.0 << std::vector<double>{1, 3, 5, 6};
        QTest::newRow("large-step") << 1.0 << 2.0 << 10.0 << std::vector<double>{1, 2};
        QTest::newRow("single") << 2.5 << 2.5 << 0.0 << std::vector<double>{2.5};
        QTest::newRow("decimal-endpoint") << 0.0 << 0.9 << 0.3 << std::vector<double>{0, .3, .6, .9};
        QTest::newRow("fractional-endpoint") << 0.0 << 1.3 << 0.5 << std::vector<double>{0, .5, 1, 1.3};
        std::vector<double> miriGains;
        for (int gain = 0; gain <= 102; ++gain) miriGains.push_back(gain);
        QTest::newRow("miri-vhf-range") << 0.0 << 102.0 << 1.0 << miriGains;
        std::vector<double> limitGains;
        for (int gain = 0; gain < 4096; ++gain) limitGains.push_back(gain);
        QTest::newRow("entry-limit") << 0.0 << 4095.0 << 1.0 << limitGains;
        limitGains.back() = 4094.5;
        QTest::newRow("entry-limit-off-grid") << 0.0 << 4094.5 << 1.0 << limitGains;
    }

    void range() {
        QFETCH(double, minimum);
        QFETCH(double, maximum);
        QFETCH(double, step);
        QFETCH(std::vector<double>, expected);
        NullRadioController controller;
        CSoapySdr input(controller);
        input.m_device = SoapySDR::Device::make("driver=welle_test_gain");
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        QVERIFY(device != nullptr);
        device->range = SoapySDR::Range(minimum, maximum, step);
        input.m_gains = {-100, 100}; // Previous device/restart must not leak gains.
        input.updateGains();
        QCOMPARE(input.m_gains.size(), expected.size());
        for (size_t i = 0; i < expected.size(); ++i) {
            QVERIFY(std::abs(input.m_gains[i] - expected[i]) < 1e-12);
            QVERIFY(input.m_gains[i] >= minimum && input.m_gains[i] <= maximum);
        }
        QCOMPARE(input.m_gains.back(), maximum);
        const auto first = input.m_gains;
        input.updateGains();
        QVERIFY(input.m_gains == first);

        device->gain = minimum;
        for (size_t i = 1; i < first.size(); ++i) {
            input.increaseGain();
            QCOMPARE(device->gain, first[i]);
        }
        const int writesAtMaximum = device->writes;
        input.increaseGain();
        QCOMPARE(device->gain, maximum);
        QCOMPARE(device->writes, writesAtMaximum);
        for (size_t i = first.size() - 1; i > 0; --i) {
            input.decreaseGain(); // Overload still reduces gain from the maximum.
            QCOMPARE(device->gain, first[i-1]);
        }
        const int writesAtMinimum = device->writes;
        input.decreaseGain();
        QCOMPARE(device->gain, minimum);
        QCOMPARE(device->writes, writesAtMinimum);
    }

    void invalidRange_data() {
        QTest::addColumn<double>("minimum");
        QTest::addColumn<double>("maximum");
        QTest::addColumn<double>("step");
        QTest::newRow("reversed") << 2.0 << 1.0 << 1.0;
        QTest::newRow("negative-step") << 0.0 << 1.0 << -1.0;
        QTest::newRow("nan-minimum") << std::numeric_limits<double>::quiet_NaN() << 1.0 << 1.0;
        QTest::newRow("infinite-maximum") << 0.0 << std::numeric_limits<double>::infinity() << 1.0;
        QTest::newRow("infinite-step") << 0.0 << 1.0 << std::numeric_limits<double>::infinity();
        QTest::newRow("nan-step") << 0.0 << 1.0 << std::numeric_limits<double>::quiet_NaN();
        QTest::newRow("tiny-step") << 0.0 << 70.0 << 1e-6;
        QTest::newRow("tinier-step") << 0.0 << 70.0 << 1e-9;
        QTest::newRow("over-entry-limit") << 0.0 << 4096.0 << 1.0;
        QTest::newRow("off-grid-over-limit") << 0.0 << 4095.5 << 1.0;
        QTest::newRow("overflowing-span") << -1e308 << 1e308 << 1e300;
    }

    void invalidRange() {
        QFETCH(double, minimum);
        QFETCH(double, maximum);
        QFETCH(double, step);
        NullRadioController controller;
        CSoapySdr input(controller);
        input.m_device = SoapySDR::Device::make("driver=welle_test_gain");
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        device->range = SoapySDR::Range(minimum, maximum, step);
        input.m_gains = {42};
        QVERIFY_EXCEPTION_THROWN(input.updateGains(), std::runtime_error);
        QVERIFY(input.m_gains.empty());
        QCOMPARE(device->writes, 0);
    }

    void restartRejectsRange_data() { invalidRange_data(); }

    void restartRejectsRange() {
        QFETCH(double, minimum);
        QFETCH(double, maximum);
        QFETCH(double, step);
        nextRange = SoapySDR::Range(minimum, maximum, step);
        MessageController controller;
        CSoapySdr input(controller);
        input.m_driver_args = "driver=welle_test_gain";
        // Exercise the real restart boundary, including a second failed attempt.
        for (int attempt = 1; attempt <= 2; ++attempt) {
            input.m_gains = {42};
            QVERIFY(!input.restart());
            QVERIFY(!input.is_ok());
            QVERIFY(input.m_device == nullptr);
            QVERIFY(!input.m_thread.joinable());
            QVERIFY(input.m_gains.empty());
            QCOMPARE(liveDevices, 0);
            QCOMPARE(controller.errors, attempt);
            QCOMPARE(controller.message,
                     std::string("The SoapySDR device reported an invalid gain range."));
        }
    }

    void restartRejectsDriverException() {
        failRangeQuery = true;
        MessageController controller;
        CSoapySdr input(controller);
        input.m_driver_args = "driver=welle_test_gain";
        input.m_gains = {42};
        QVERIFY(!input.restart());
        QVERIFY(!input.is_ok());
        QVERIFY(input.m_device == nullptr);
        QVERIFY(!input.m_thread.joinable());
        QVERIFY(input.m_gains.empty());
        QCOMPARE(liveDevices, 0);
        QCOMPARE(controller.errors, 1);
    }

    void gainAfterOpen() {
        NullRadioController controller;
        CSoapySdr input(controller);
        input.setGain(1); // An older request must not overwrite the runtime change.
        QVERIFY(input.restart());
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        QVERIFY(device != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(device->activated.load(), 2000);
        QCOMPARE(device->gainAtActivation, 10.0);
        QCOMPARE(input.setGain(2), 20.0f);
        for (int run = 0; run < 2; ++run) {
            input.stop();
            QVERIFY(input.restart());
            // unmake() destroyed the old object; reacquire the new device.
            device = dynamic_cast<GainDevice*>(input.m_device);
            QVERIFY(device != nullptr);
            QTRY_VERIFY_WITH_TIMEOUT(device->activated.load(), 2000);
            QCOMPARE(device->gainAtActivation, 20.0);
        }
    }

    void startupGainFailure_data() {
        QTest::addColumn<QString>("fault");
        for (const char* fault : {"mode", "write", "write-out-of-range", "read"})
            QTest::newRow(fault) << QString(fault);
    }
    void startupGainFailure() {
        QFETCH(QString, fault);
        MessageController controller;
        CSoapySdr input(controller);
        input.setGain(2);
        driverFault = fault.toStdString();
        QVERIFY(!input.restart());
        QVERIFY(!input.is_ok());
        QVERIFY(input.m_device == nullptr);
        QVERIFY(!input.m_thread.joinable());
        QVERIFY(input.m_gains.empty());
        QCOMPARE(liveDevices, 0);
        QCOMPARE(streamActivations.load(), 0);
        QCOMPARE(controller.errors, 1);
        QCOMPARE(controller.message,
                 std::string("The SoapySDR device could not apply its gain settings."));
        // A transient device error must not prevent a later successful retry.
        driverFault.clear();
        QVERIFY(input.restart());
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        QVERIFY(device != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(device->activated.load(), 2000);
        QCOMPARE(device->gainAtActivation, 20.0);
        QCOMPARE(streamActivations.load(), 1);
        input.stop();
        QCOMPARE(liveDevices, 0);
    }

    void shorterGainTableClearsRequest() {
        NullRadioController controller;
        CSoapySdr input(controller);
        input.setGain(3);
        QVERIFY(input.restart());
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        QVERIFY(device != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(device->activated.load(), 2000);
        QCOMPARE(device->gainAtActivation, 30.0);
        input.stop();
        nextRange = SoapySDR::Range(0, 10, 10);
        QVERIFY(input.restart());
        device = dynamic_cast<GainDevice*>(input.m_device);
        QVERIFY(device != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(device->activated.load(), 2000);
        QCOMPARE(device->writes, 0);
        QCOMPARE(input.m_requestedGain, -1);
        input.stop();
        nextRange = SoapySDR::Range(0, 30, 10);
        QVERIFY(input.restart());
        device = dynamic_cast<GainDevice*>(input.m_device);
        QVERIFY(device != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(device->activated.load(), 2000);
        QCOMPARE(device->gainAtActivation, 0.0);
        QCOMPARE(device->writes, 0);
    }

};

QTEST_GUILESS_MAIN(TestSoapyGain)
#include "tst_soapy_gain.moc"
