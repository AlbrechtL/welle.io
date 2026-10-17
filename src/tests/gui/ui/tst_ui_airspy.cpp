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

// gui.ui.airspy: Airspy as input device. The gain mode (linearity,
// sensitivity, hybrid) is chosen in the Airspy settings and kept after a
// restart. Needs no Airspy (nor Airspy support in the build); with one
// connected, the gain range of the device follows the mode (0..21, hybrid:
// VGA 0..15).

#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_test_case.h"

class UiAirspyTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();

    void choosesGainMode();

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
    bool choose(const QString& combo, int index)
    {
        if (!app.click(combo))
            return false;
        UiApp::settle(); // Popup animation
        if (!app.waitVisible("comboItem." + QString::number(index)))
            return false;
        const bool clicked = app.click("comboItem." + QString::number(index));
        UiApp::settle();
        return clicked;
    }
    bool chooseGainMode(int index)
    {
        return choose("setting.airspyGainMode", index);
    }
    // Without an Airspy (or without Airspy support) opening it fails, the
    // device box shows "None" and after a restart the auto detection is on:
    // choose "Airspy" to see its settings
    bool showAirspySettings()
    {
        if (app.isVisible("airspySettings"))
            return true;
        // The error message that no device was found covers the settings
        QObject* snackbar = app.find("snackbar");
        if (snackbar && snackbar->property("visible").toBool()) {
            if (!app.click("snackbar.close")
                    || !UiApp::waitFor([&] { return !snackbar->property("visible").toBool(); }))
                return false;
        }
        if (app.property("setting.enableAutoSdr", "checked").toBool()) {
            if (!app.click("setting.enableAutoSdr"))
                return false;
            UiApp::settle();
        }
        return choose("setting.device", 1) && app.waitVisible("airspySettings");
    }
    QString shownGainMode()
    {
        return QString("%1 %2").arg(app.property("setting.airspyGainMode", "currentIndex").toInt())
                               .arg(app.property("setting.airspyGainMode", "displayText").toString());
    }
    bool airspyConnected()
    {
        return app.radio()->property("deviceId").value<CDeviceID>() == CDeviceID::AIRSPY;
    }
};

void UiAirspyTests::initTestCase()
{
    hideDebugOutput();
    UiApp::writeSettings({
        {"device", 1}, // Airspy
        {"enableAutoSdr", false},
    });
    QVERIFY2(app.start(), "MainView.qml could not be loaded");
}

void UiAirspyTests::choosesGainMode()
{
    QVERIFY(openSettings());
    QVERIFY(showAirspySettings());
    QCOMPARE(shownGainMode(), QString("0 Linearity"));
    if (airspyConnected())
        QCOMPARE(app.radio()->property("gainCount").toInt(), 21);

    QVERIFY(chooseGainMode(2));
    QCOMPARE(shownGainMode(), QString("2 Hybrid (tuner AGC)"));
    if (airspyConnected())
        QTRY_COMPARE(app.radio()->property("gainCount").toInt(), 15);

    QVERIFY(chooseGainMode(1));
    QCOMPARE(shownGainMode(), QString("1 Sensitivity"));
    if (airspyConnected())
        QTRY_COMPARE(app.radio()->property("gainCount").toInt(), 21);
    QVERIFY(closeSettings());

    // Kept after a restart
    QVERIFY(app.restart());
    QVERIFY(openSettings());
    QVERIFY(showAirspySettings());
    QCOMPARE(shownGainMode(), QString("1 Sensitivity"));
    QVERIFY(closeSettings());

    qInfo() << "Gain mode chosen and kept"
            << (airspyConnected() ? "(with an Airspy)" : "(without an Airspy)");
}

WELLE_GUI_TEST_MAIN(UiAirspyTests)

#include "tst_ui_airspy.moc"
