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

// gui.ui.noreceiver: welle.io without a receiver (input device "None").
// A station scan must not start: it would wait for a signal at the first
// channel forever. The empty station list explains why and leads to the
// receiver settings. Needs no recording and no hardware.

#include <QSignalSpy>

#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_test_case.h"

class UiNoReceiverTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();

    void disablesScan();
    void opensReceiverSettings();
    void scanEndsAtOnce();

private:
    bool isEnabled(const QString& objectName) const
    {
        return app.property(objectName, "enabled").toBool();
    }
};

void UiNoReceiverTests::initTestCase()
{
    hideDebugOutput();
    UiApp::writeSettings({
        {"device", 0}, // None
        {"enableAutoSdr", false},
    });
    QVERIFY2(app.start(), "MainView.qml could not be loaded");
    QTRY_COMPARE(app.radio()->property("deviceId").value<CDeviceID>(), CDeviceID::NULLDEVICE);
}

void UiNoReceiverTests::disablesScan()
{
    QVERIFY(app.isVisible("stationListEmpty"));
    QVERIFY(app.isVisible("scanButton"));
    QVERIFY(!isEnabled("scanButton"));
    QVERIFY(app.isVisible("receiverSettingsButton"));
    QVERIFY(app.text("stationListEmptyText").contains("No receiver found"));
    UI_VERIFY_EMPTY(app.controlsWithoutAccessibleName(), "Controls without accessible name");
    app.screenshot("noreceiver-empty");

    app.resize(360, 640);
    QVERIFY(app.isVisible("receiverSettingsButton"));
    UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
    app.screenshot("noreceiver-empty-360x640");
    app.resize(1200, 800);

    // The menu entry is disabled too
    QVERIFY(app.click("stationListMenuButton"));
    QVERIFY(app.waitVisible("startScanItem"));
    QVERIFY(!isEnabled("startScanItem"));
    QVERIFY(!isEnabled("stopScanItem"));
    app.key(Qt::Key_Escape);
    QVERIFY(app.waitHidden("stationListMenu"));
    QVERIFY(!app.radio()->property("isChannelScan").toBool());

    // The favorites show their own explanation, no receiver hint
    QVERIFY(showList("favorites"));
    QTRY_VERIFY(!app.isVisible("receiverSettingsButton"));
    QVERIFY(!app.text("stationListEmptyText").contains("No receiver found"));
    QVERIFY(showList("all"));
    QTRY_VERIFY(app.isVisible("receiverSettingsButton"));

    qInfo() << "Without a receiver the scan is disabled and the list explains why";
}

void UiNoReceiverTests::opensReceiverSettings()
{
    QVERIFY(app.click("receiverSettingsButton"));
    QVERIFY(app.waitVisible("settingsPage"));
    QVERIFY(waitForPage());
    QVERIFY(app.isVisible("receiverSettings"));
    QTRY_VERIFY(app.isVisible("setting.device"));
    app.screenshot("noreceiver-settings");

    app.key(Qt::Key_Escape);
    QVERIFY(app.waitHidden("settingsPage"));
    QVERIFY(waitForPage());

    qInfo() << "The button of the empty list opens the receiver settings";
}

void UiNoReceiverTests::scanEndsAtOnce()
{
    QSignalSpy scanStopped(app.radio(), &CRadioController::scanStopped);
    QSignalSpy scanChanged(app.radio(), &CRadioController::isChannelScanChanged);

    QVERIFY(QMetaObject::invokeMethod(app.radio(), "startScan"));

    QCOMPARE(scanStopped.count(), 1);
    QCOMPARE(scanChanged.count(), 0);
    QVERIFY(!app.radio()->property("isChannelScan").toBool());
    QCOMPARE(app.radio()->property("title").toString(), QString("No receiver"));
    UiApp::settle();
    QVERIFY(!app.isVisible("scanProgress"));

    qInfo() << "A scan without a receiver ends at once";
}

WELLE_GUI_TEST_MAIN(UiNoReceiverTests)

#include "tst_ui_noreceiver.moc"
