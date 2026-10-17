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

// gui.ui.devices: the input device box lists only the devices of this build
// (CMake options AIRSPY, RTLSDR, SOAPYSDR, V4L2_SDR). A saved device the
// build doesn't support is shown as "None" and kept until another device is
// chosen. The Linux kernel SDR device has its own device list. Needs no
// recording and no receiver.

#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_test_case.h"
#ifdef HAVE_V4L2_SDR
#include "v4l2_sdr.h"
#endif

class UiDevicesTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();

    void listsCompiledInDevices();
    void showsUnavailableDeviceAsNone();
    void choosesLinuxSdrDevice();

private:
    // Labels of the device box in this build
    static QStringList compiledInDevices()
    {
        QStringList devices = {"None"};
#ifdef HAVE_AIRSPY
        devices << "Airspy";
#endif
#ifdef HAVE_RTLSDR
        devices << "rtl-sdr";
#endif
#ifdef HAVE_SOAPYSDR
        devices << "SoapySDR";
#endif
        devices << "rtl-tcp" << "RAW file";
#ifdef HAVE_V4L2_SDR
        devices << "Linux SDR (V4L2)";
#endif
        return devices;
    }
    bool openSettings()
    {
        return app.click("navButton")
               && UiApp::waitFor([&] { return app.find("navDrawer")->property("position").toReal() >= 1.0; })
               && app.click("navItem.settings")
               && app.waitVisible("settingsPage")
               && waitForPage();
    }
    bool closeSettings()
    {
        app.key(Qt::Key_Escape);
        return app.waitHidden("settingsPage") && waitForPage();
    }
    bool openDeviceBox()
    {
        if (!app.click("setting.device"))
            return false;
        UiApp::settle(); // Popup animation
        return app.waitVisible("comboItem.0");
    }
    int savedDevice() const { return app.find("receiverSettings")->property("device").toInt(); }
    QString deviceSettingsPage() const { return app.property("deviceSettings", "source").toUrl().fileName(); }
    CDeviceID deviceId() const { return app.radio()->property("deviceId").value<CDeviceID>(); }
};

void UiDevicesTests::initTestCase()
{
    hideDebugOutput();
    UiApp::writeSettings({
        {"device", 3}, // SoapySDR, which the default build doesn't support
        {"enableAutoSdr", false},
        {"rtlTcpHostName", "127.0.0.1"},
        {"ipPort", "1234"},
    });
    QVERIFY2(app.start(), "MainView.qml could not be loaded");
}

void UiDevicesTests::listsCompiledInDevices()
{
    QVERIFY(openSettings());
    QVERIFY(openDeviceBox());

    QStringList shown;
    for (int i = 0; app.isVisible("comboItem." + QString::number(i)); i++)
        shown << app.text("comboItem." + QString::number(i));
    QCOMPARE(shown, compiledInDevices());
    app.screenshot("device-box");

    app.key(Qt::Key_Escape); // Closes the popup
    QVERIFY(app.waitHidden("comboItem.0"));
    QVERIFY(closeSettings());

    qInfo() << "Device box:" << shown.join(", ");
}

void UiDevicesTests::showsUnavailableDeviceAsNone()
{
#ifdef HAVE_SOAPYSDR
    QSKIP("Needs a build without SoapySDR support");
#endif
    QVERIFY(openSettings());

    // Shown as "None", which opens no device, but still saved
    QCOMPARE(app.property("setting.device", "displayText").toString(), QString("None"));
    QCOMPARE(deviceSettingsPage(), QString("NullSettings.qml"));
    QCOMPARE(savedDevice(), 3);
    QTRY_COMPARE(deviceId(), CDeviceID::NULLDEVICE);

    // Another device is saved with its position in the full list
    QVERIFY(openDeviceBox());
    const QString rtlTcpItem = "comboItem." + QString::number(compiledInDevices().indexOf("rtl-tcp"));
    QCOMPARE(app.text(rtlTcpItem), QString("rtl-tcp"));
    QVERIFY(app.click(rtlTcpItem));
    UiApp::settle();
    QCOMPARE(savedDevice(), 4);
    QCOMPARE(app.property("setting.device", "displayText").toString(), QString("rtl-tcp"));
    QVERIFY(app.waitVisible("rtlTcpSettings"));
    QTRY_COMPARE(deviceId(), CDeviceID::RTL_TCP);
    QVERIFY(closeSettings());

    // Kept after a restart
    QVERIFY(app.restart());
    QTRY_COMPARE(deviceId(), CDeviceID::RTL_TCP);
    QVERIFY(openSettings());
    QCOMPARE(app.property("setting.device", "displayText").toString(), QString("rtl-tcp"));
    QCOMPARE(savedDevice(), 4);
    QVERIFY(closeSettings());

    qInfo() << "Unavailable device shown as None, rtl-tcp chosen and kept";
}

void UiDevicesTests::choosesLinuxSdrDevice()
{
#ifndef HAVE_V4L2_SDR
    QSKIP("Built without Linux kernel SDR support (CMake option V4L2_SDR)");
#else
    // Without a device (e.g. CI) the Null device is used
    const auto devices = CV4L2Sdr::listDevices();
    const CDeviceID expectedId = devices.empty() ? CDeviceID::NULLDEVICE : CDeviceID::V4L2;

    QVERIFY(openSettings());
    QVERIFY(openDeviceBox());
    const QString item = "comboItem." + QString::number(compiledInDevices().indexOf("Linux SDR (V4L2)"));
    QCOMPARE(app.text(item), QString("Linux SDR (V4L2)"));
    QVERIFY(app.click(item));
    UiApp::settle();
    QCOMPARE(savedDevice(), 6);
    QCOMPARE(deviceSettingsPage(), QString("V4L2Settings.qml"));
    QVERIFY(app.waitVisible("v4l2Settings"));
    QTRY_COMPARE(deviceId(), expectedId);

    // "First found" and the devices of the system
    QCOMPARE(app.property("setting.v4l2Device", "displayText").toString(), QString("First found"));
    QVERIFY(app.click("setting.v4l2Device"));
    UiApp::settle(); // Popup animation
    QVERIFY(app.waitVisible("comboItem.0"));
    QStringList shown;
    for (int i = 0; app.isVisible("comboItem." + QString::number(i)); i++)
        shown << app.text("comboItem." + QString::number(i));
    QCOMPARE(shown.size(), int(devices.size()) + 1);
    QCOMPARE(shown.first(), QString("First found"));
    app.screenshot("v4l2-device-box");
    app.key(Qt::Key_Escape);
    QVERIFY(app.waitHidden("comboItem.0"));
    QVERIFY(closeSettings());

    // Kept after a restart
    QVERIFY(app.restart());
    QTRY_COMPARE(deviceId(), expectedId);
    QVERIFY(openSettings());
    QCOMPARE(app.property("setting.device", "displayText").toString(), QString("Linux SDR (V4L2)"));
    QVERIFY(app.waitVisible("v4l2Settings"));
    QCOMPARE(savedDevice(), 6);
    QVERIFY(closeSettings());

    qInfo() << "Linux SDR devices:" << shown.join(", ");
#endif
}

WELLE_GUI_TEST_MAIN(UiDevicesTests)

#include "tst_ui_devices.moc"
