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
#include <functional>
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
    size_t mtu = 4096;
    std::function<int(void*, size_t)> feed;
    std::vector<double> commands;
    bool failGain = false;
    int failedWrites = 0;
    std::function<void(double)> onGain;
    double frequency = 0;
    void setFrequency(int, size_t, double value, const SoapySDR::Kwargs&) override { frequency = value; }
    double getFrequency(int, size_t) const override { return frequency; }

    void setMasterClockRate(double) override {
        if (driverFault == "master-clock") throw std::runtime_error("Test master clock failure");
    }
    void setSampleRate(int, size_t, double) override {
        if (driverFault == "sample-rate") throw std::runtime_error("Test sample rate failure");
    }
    bool hasDCOffsetMode(int, size_t) const override { return driverFault == "dc-offset"; }
    void setDCOffsetMode(int, size_t, bool) override { throw std::runtime_error("Test DC offset failure"); }
    double getSampleRate(int, size_t) const override { return 2048000; }
    void setGainMode(int, size_t, bool) override {
        if (driverFault == "mode") throw std::runtime_error("Test gain-mode failure");
    }
    SoapySDR::Stream* setupStream(int, const std::string&,
        const std::vector<size_t>&, const SoapySDR::Kwargs&) override {
        if (driverFault == "setup-stream") throw std::runtime_error("Test setup failure");
        if (driverFault == "null-stream") return nullptr;
        return reinterpret_cast<SoapySDR::Stream*>(this);
    }
    size_t getStreamMTU(SoapySDR::Stream*) const override {
        if (driverFault == "zero-mtu") return 0;
        if (driverFault == "oversized-mtu") return std::numeric_limits<size_t>::max();
        return mtu;
    }
    int activateStream(SoapySDR::Stream*, int, long long, size_t) override {
        if (driverFault == "activate-throw") throw std::runtime_error("Test activate failure");
        if (driverFault == "activate-error") return SOAPY_SDR_STREAM_ERROR;
        ++streamActivations;
        gainAtActivation = gain;
        activated = true;
        return 0;
    }
    int readStream(SoapySDR::Stream*, void* const* buffers, size_t count, int&,
        long long&, long) override {
        if (driverFault == "stream-error" || driverFault == "close-stream") return SOAPY_SDR_STREAM_ERROR;
        if (driverFault == "read-throw") throw std::runtime_error("Test read failure");
        if (feed) return feed(buffers[0], count);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return SOAPY_SDR_TIMEOUT;
    }
    void closeStream(SoapySDR::Stream*) override {
        if (driverFault == "close-stream") throw std::runtime_error("Test close failure");
    }

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
        if (failGain) { ++failedWrites; throw std::runtime_error("Test gain failure"); }
        if (driverFault == "write") throw std::runtime_error("Test gain write failure");
        if (driverFault == "write-out-of-range") throw std::out_of_range("Test driver range failure");
        if (onGain) onGain(value);
        gain = value;
        ++writes;
        commands.push_back(value);
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

    void deviceSetupFailure_data() {
        QTest::addColumn<QString>("fault");
        for (const char* fault : {"master-clock", "sample-rate", "dc-offset"})
            QTest::newRow(fault) << QString(fault);
    }
    void deviceSetupFailure() {
        QFETCH(QString, fault);
        MessageController controller;
        CSoapySdr input(controller);
        driverFault = fault.toStdString();
        QVERIFY(!input.restart());
        QVERIFY(input.m_device == nullptr);
        QVERIFY(!input.m_thread.joinable());
        QVERIFY(!input.is_ok());
        QCOMPARE(liveDevices, 0);
        QCOMPARE(controller.errors, 1);
        driverFault.clear();
        QVERIFY(input.restart());
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        QTRY_VERIFY_WITH_TIMEOUT(device->activated.load(), 2000);
        input.stop();
        QCOMPARE(liveDevices, 0);
    }

    void restartAfterStreamFailure_data() {
        QTest::addColumn<QString>("fault");
        for (const char* fault : {"stream-error", "setup-stream", "null-stream", "activate-throw", "activate-error",
                                  "read-throw", "zero-mtu", "oversized-mtu", "close-stream"})
            QTest::newRow(fault) << QString(fault);
    }
    void restartAfterStreamFailure() {
        QFETCH(QString, fault);
        NullRadioController controller;
        CSoapySdr input(controller);
        driverFault = fault.toStdString();
        QVERIFY(input.restart());
        QTRY_VERIFY_WITH_TIMEOUT(!input.is_ok(), 2000);
        // The worker has ended, but its std::thread still needs joining.
        QVERIFY(input.m_thread.joinable());
        driverFault.clear();
        QVERIFY(input.restart());
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        QVERIFY(device != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(device->activated.load(), 2000);
        QCOMPARE(liveDevices, 1);
        input.stop();
        QCOMPARE(liveDevices, 0);
    }
    void resetRejectsPendingRead() {
        NullRadioController controller;
        CSoapySdr input(controller);
        input.m_device = SoapySDR::Device::make("driver=welle_test_gain");
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        input.updateGains();
        int reads = 0;
        device->feed = [&](void* buffer, size_t count) {
            ++reads;
            if (reads == 3) { input.m_running = false; return SOAPY_SDR_TIMEOUT; }
            auto* samples = static_cast<DSPCOMPLEX*>(buffer);
            std::fill(samples, samples+count, DSPCOMPLEX(float(reads), 0));
            if (reads == 1) input.reset(); // retune completed while driver read was pending
            return int(count);
        };
        input.m_running = true;
        input.process(reinterpret_cast<SoapySDR::Stream*>(device));
        QCOMPARE(input.getSamplesToRead(), int(device->mtu));
        std::vector<DSPCOMPLEX> samples(device->mtu);
        QCOMPARE(input.getSamples(samples.data(), samples.size()), int(samples.size()));
        for (auto value : samples) QCOMPARE(value, DSPCOMPLEX(2,0));
        const auto spectrum = input.getSpectrumSamples(8192);
        QCOMPARE(spectrum.size(), device->mtu);
        for (auto value : spectrum) QCOMPARE(value, DSPCOMPLEX(2,0));
        // A subsequent reset also clears already queued spectrum samples.
        const DSPCOMPLEX queued(.3f, .2f);
        input.m_sampleBuffer.putDataIntoBuffer(&queued, 1);
        input.m_spectrumSampleBuffer.putDataIntoBuffer(&queued, 1);
        input.reset();
        QCOMPARE(input.getSamplesToRead(), 0);
        QVERIFY(input.getSpectrumSamples(8192).empty());
    }
    void stoppedReadIsNotPublished() {
        NullRadioController controller;
        CSoapySdr input(controller);
        input.m_device = SoapySDR::Device::make("driver=welle_test_gain");
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        input.updateGains();
        device->feed = [&](void* buffer, size_t count) {
            std::fill_n(static_cast<DSPCOMPLEX*>(buffer), count, DSPCOMPLEX(.1f, .1f));
            input.m_running = false;
            return int(count);
        };
        input.m_running = true;
        input.process(reinterpret_cast<SoapySDR::Stream*>(device));
        QCOMPARE(input.getSamplesToRead(), 0);
        QVERIFY(input.getSpectrumSamples(8192).empty());
    }
    void largeMtuUsesBoundedRead() {
        NullRadioController controller;
        CSoapySdr input(controller);
        input.m_device = SoapySDR::Device::make("driver=welle_test_gain");
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        input.updateGains();
        const auto capacity = size_t(input.m_sampleBuffer.GetBufferSize());
        device->mtu = capacity * 2;
        size_t requested = 0;
        device->feed = [&](void*, size_t count) {
            requested = count;
            input.m_running = false;
            return SOAPY_SDR_TIMEOUT;
        };
        input.m_running = true;
        input.process(reinterpret_cast<SoapySDR::Stream*>(device));
        QCOMPARE(requested, capacity);
        QCOMPARE(input.getSamplesToRead(), 0);
    }

    void oversizedReadIsRejected() {
        NullRadioController controller;
        CSoapySdr input(controller);
        input.m_device = SoapySDR::Device::make("driver=welle_test_gain");
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        input.updateGains();
        int reads = 0;
        device->feed = [&](void*, size_t count) {
            if (++reads == 2) { input.m_running = false; return SOAPY_SDR_TIMEOUT; }
            return int(count)+1;
        };
        input.m_running = true;
        // The driver reports more samples than it was asked to write.
        // No zero-padded synthetic samples may enter the decoder or recorder.
        QVERIFY_EXCEPTION_THROWN(input.process(reinterpret_cast<SoapySDR::Stream*>(device)), std::runtime_error);
        QCOMPARE(input.getSamplesToRead(), 0);
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

    void streamingAgc_data() {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<double>("start");
        QTest::addColumn<double>("maximum");
        QTest::addColumn<int>("mtu");
        QTest::addColumn<double>("step");
        for (const char* mode : {"weak", "overload", "impulses", "manual",
                                 "zero", "nan", "infinity", "gain-error"})
            QTest::newRow(mode) << QString(mode) << (QString(mode)=="overload" ? 102.0 : 59.0) << 102.0 << 4096 << 1.0;
        QTest::newRow("large-block") << QString("weak") << 59.0 << 102.0 << 18432 << 1.0;
        QTest::newRow("rtl-range") << QString("weak") << 0.0 << 49.6 << 8192 << 1.0;
        QTest::newRow("ceiling") << QString("ceiling") << 59.0 << 102.0 << 4096 << 1.0;
        QTest::newRow("fractional-step") << QString("weak") << 0.0 << 49.6 << 4096 << 0.4;
        QTest::newRow("fractional-endpoint") << QString("ceiling") << 0.0 << 1.3 << 4096 << 0.5;
    }

    void streamingAgc() {
        QFETCH(QString, mode);
        QFETCH(double, start);
        QFETCH(double, maximum);
        QFETCH(int, mtu);
        QFETCH(double, step);
        NullRadioController controller;
        CSoapySdr input(controller);
        input.m_device = SoapySDR::Device::make("driver=welle_test_gain");
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        device->range = SoapySDR::Range(0, maximum, step);
        device->mtu = mtu;
        device->gain = start;
        device->failGain = mode == "gain-error";
        input.updateGains();
        input.setAgc(mode != "manual");
        uint64_t remaining = uint64_t(INPUT_RATE) * 8;
        uint32_t noise = 123456789;
        device->feed = [&](void* buffer, size_t capacity) {
            if (!remaining) { input.m_running = false; return 0; }
            const size_t count = std::min<uint64_t>(capacity, remaining);
            auto* samples = static_cast<DSPCOMPLEX*>(buffer);
            const double reference = maximum > 50 ? 59 : 0;
            const float amplitude = (mode == "ceiling" ? 1e-7 : .02) *
                std::pow(10.0, (device->gain - reference) / 20.0);
            auto component = [&]() {
                noise = noise * 1664525u + 1013904223u;
                const float value = (int(noise >> 16) - 32768) / 32768.0f;
                return std::clamp(value * 1.73205f * amplitude, -1.0f, 1.0f);
            };
            for (size_t i = 0; i < count; ++i) samples[i] = {component(), component()};
            if (mode == "zero") std::fill(samples, samples + count, DSPCOMPLEX{});
            if (mode == "nan") samples[0] = {std::numeric_limits<float>::quiet_NaN(), 0};
            if (mode == "infinity") samples[0] = {std::numeric_limits<float>::infinity(), 0};
            if (mode == "impulses") samples[0] = {1, 1};
            remaining -= count;
            return int(count);
        };
        input.m_running = true;
        // Exercise the actual streaming loop and its gain commands, not a
        // second implementation of the controller in the test.
        input.process(reinterpret_cast<SoapySDR::Stream*>(device));
        QCOMPARE(remaining, uint64_t(0));
        for (double command : device->commands) {
            QVERIFY(command >= 0 && command <= maximum);
            QVERIFY(std::find(input.m_gains.begin(), input.m_gains.end(), command) != input.m_gains.end());
        }
        if (mode == "weak" || mode == "impulses") {
            QVERIFY(device->gain > start);
            QVERIFY(input.m_agc.lastLevelDb() > -18 && input.m_agc.lastLevelDb() <= -11);
            QVERIFY(device->writes < 20);
        }
        else if (mode == "overload") QVERIFY(device->gain < start);
        else if (mode == "ceiling") QCOMPARE(device->gain, maximum);
        else {
            QCOMPARE(device->gain, start);
            QCOMPARE(device->writes, 0);
        }
        if (mode == "gain-error") QCOMPARE(device->failedWrites, 1);
    }

    void delayedGainSamples_data() {
        QTest::addColumn<bool>("overload");
        QTest::addColumn<bool>("retune");
        QTest::newRow("weak") << false << false;
        QTest::newRow("overload") << true << false;
        QTest::newRow("retune") << false << true;
    }

    void delayedGainSamples() {
        QFETCH(bool, overload);
        QFETCH(bool, retune);
        NullRadioController controller;
        CSoapySdr input(controller);
        input.m_device = SoapySDR::Device::make("driver=welle_test_gain");
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        QVERIFY(device != nullptr);
        device->range = SoapySDR::Range(0, 102, 1);
        device->gain = overload ? 102 : 0;
        input.updateGains();
        input.setAgc(true);
        const uint64_t settleSamples = uint64_t(INPUT_RATE) * 250 / 1000;
        const uint64_t minimumSpacing = settleSamples +
            uint64_t(INPUT_RATE) * SoftwareAgc::windowMs / 1000;
        uint64_t delivered = 0, applyAt = 0, retunedAt = 0;
        double effectiveGain = device->gain, pendingGain = effectiveGain;
        std::vector<uint64_t> commandsAt;
        device->onGain = [&](double gain) {
            commandsAt.push_back(delivered);
            // Emulate a tuner/driver queue: the next 250 ms still use the old gain.
            pendingGain = gain;
            applyAt = delivered + settleSamples;
        };
        device->feed = [&](void* buffer, size_t capacity) {
            if (delivered >= uint64_t(INPUT_RATE) * 4) {
                input.m_running = false;
                return 0;
            }
            if (retune && !retunedAt && delivered >= uint64_t(INPUT_RATE) * 2) {
                input.setFrequency(229072000);
                retunedAt = delivered;
            }
            auto* samples = static_cast<DSPCOMPLEX*>(buffer);
            for (size_t i = 0; i < capacity; ++i) {
                if (applyAt && delivered + i >= applyAt) effectiveGain = pendingGain;
                const float amplitude = std::min(1.0, (retunedAt ? .01 : .0001) * std::pow(10.0, effectiveGain / 20));
                samples[i] = (i & 1) ? DSPCOMPLEX(amplitude, -amplitude)
                                     : DSPCOMPLEX(-amplitude, amplitude);
            }
            delivered += capacity;
            return int(capacity);
        };
        input.m_running = true;
        input.process(reinterpret_cast<SoapySDR::Stream*>(device));
        QVERIFY(!commandsAt.empty());
        for (size_t i = 1; i < commandsAt.size(); ++i) {
            QVERIFY2(commandsAt[i] - commandsAt[i-1] >= minimumSpacing,
                     "AGC acted on pre-change samples before settling plus a new window");
        }
        if (retune) {
            const auto next = std::find_if(commandsAt.begin(), commandsAt.end(),
                                          [&](uint64_t at) { return at > retunedAt; });
            QVERIFY(next != commandsAt.end());
            QVERIFY2(*next - retunedAt >= minimumSpacing,
                     "Retune must discard the old AGC measurement and settle first");
        }
        QVERIFY(overload ? device->gain < 102 : device->gain > 0);
    }

    void quickToggleRetriesGainFailure() {
        NullRadioController controller;
        CSoapySdr input(controller);
        input.m_device = SoapySDR::Device::make("driver=welle_test_gain");
        auto* device = dynamic_cast<GainDevice*>(input.m_device);
        QVERIFY(device != nullptr);
        input.updateGains();
        input.setAgc(true);
        device->failGain = true;
        uint64_t delivered = 0;
        bool toggled = false;
        device->feed = [&](void* buffer, size_t capacity) {
            if (delivered >= uint64_t(INPUT_RATE) * 3) { input.m_running = false; return 0; }
            if (device->failedWrites && !toggled) {
                device->failGain = false;
                // Both GUI changes can happen before the sample thread reads the flag.
                input.setAgc(false);
                input.setAgc(true);
                toggled = true;
            }
            auto* samples = static_cast<DSPCOMPLEX*>(buffer);
            for (size_t i = 0; i < capacity; ++i)
                samples[i] = (i & 1) ? DSPCOMPLEX(.01f, -.01f) : DSPCOMPLEX(-.01f, .01f);
            delivered += capacity;
            return int(capacity);
        };
        input.m_running = true;
        input.process(reinterpret_cast<SoapySDR::Stream*>(device));
        QVERIFY(toggled);
        QCOMPARE(device->failedWrites, 1);
        QVERIFY2(device->writes > 0, "Off/on must retry even between two input blocks");
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
