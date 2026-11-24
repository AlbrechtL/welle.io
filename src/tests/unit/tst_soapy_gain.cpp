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
#include "input/soapy_sdr.h"
#include "null_radio_controller.h"

namespace {
// A test device records the commands from the actual CSoapySdr methods.
// No streaming thread or physical device is started.
class GainDevice : public SoapySDR::Device {
public:
    double gain = 0;
    int writes = 0;

    double getGain(const int direction, const size_t channel) const override {
        if (direction != SOAPY_SDR_RX || channel != 0)
            throw std::runtime_error("Unexpected gain channel");
        return gain;
    }

    void setGain(const int direction, const size_t channel, const double value) override {
        if (direction != SOAPY_SDR_RX || channel != 0)
            throw std::runtime_error("Unexpected gain channel");
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
};

QTEST_GUILESS_MAIN(TestSoapyGain)
#include "tst_soapy_gain.moc"
