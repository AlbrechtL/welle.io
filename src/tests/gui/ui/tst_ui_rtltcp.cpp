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

// gui.ui.rtltcp: rtl-tcp as input device. Opening and closing the settings
// keeps the device (and the playback) as it is; a changed server is
// connected when the settings are closed. Needs no recording and no
// server: the client only connects when a station is played.

#include <QSignalSpy>

#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_test_case.h"

class UiRtlTcpTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();

    void keepsDeviceWhenSettingsAreClosed();
    void opensChangedServer();

private:
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
};

void UiRtlTcpTests::initTestCase()
{
    hideDebugOutput();
    UiApp::writeSettings({
        {"device", 4}, // rtl-tcp
        {"enableAutoSdr", false},
        {"rtlTcpHostName", "127.0.0.1"},
        {"ipPort", "1234"},
    });
    QVERIFY2(app.start(), "MainView.qml could not be loaded");
    QTRY_COMPARE(app.radio()->property("deviceId").value<CDeviceID>(), CDeviceID::RTL_TCP);
}

void UiRtlTcpTests::keepsDeviceWhenSettingsAreClosed()
{
    QSignalSpy deviceReady(app.radio(), &CRadioController::deviceReady);

    for (int i = 0; i < 2; i++) {
        QVERIFY(openSettings());
        QVERIFY(app.isVisible("rtlTcpSettings"));
        QVERIFY(closeSettings());
    }
    QCOMPARE(deviceReady.count(), 0);
    QVERIFY(app.radio()->property("deviceName").toString().contains("127.0.0.1:1234"));

    qInfo() << "Opening and closing the settings doesn't open rtl-tcp again";
}

void UiRtlTcpTests::opensChangedServer()
{
    QSignalSpy deviceReady(app.radio(), &CRadioController::deviceReady);

    QVERIFY(openSettings());
    QVERIFY(app.click("setting.rtlTcpHostName"));
    app.keySequence(QKeySequence::SelectAll);
    for (char c : QByteArray("localhost"))
        QTest::keyClick(app.window(), c);
    QCOMPARE(app.text("setting.rtlTcpHostName"), QString("localhost"));
    QCOMPARE(deviceReady.count(), 0); // Not while typing
    QVERIFY(closeSettings());

    QCOMPARE(deviceReady.count(), 1);
    QVERIFY(app.radio()->property("deviceName").toString().contains("localhost:1234"));

    // Unchanged again
    QVERIFY(openSettings());
    QVERIFY(closeSettings());
    QCOMPARE(deviceReady.count(), 1);

    qInfo() << "A changed server is opened when the settings are closed";
}

WELLE_GUI_TEST_MAIN(UiRtlTcpTests)

#include "tst_ui_rtltcp.moc"
