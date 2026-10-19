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

// gui.ui.settings: the settings page. Settings of an existing installation
// (same QSettings keys as before the merged page) are shown and applied,
// switches and combo boxes are changed by clicking them, the input device
// stays open, the section index scrolls on wide windows and changes
// survive a restart.

#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_test_case.h"

static const QString clipId = "odr_dabmod";

class UiSettingsTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void showsExistingSettings();
    void opensFromDrawerAndList();
    void switchesByClickingTheRow();
    void choosesFromComboBox();
    void enablesExpertMode();
    void keepsInputDeviceOpen();
    void indexScrollsToSection();
    void layout_data();
    void layout();
    void switchesLanguage();
    void keepsChangesAfterRestart();

private:
    QObject* settingsPage() const { return app.find("settingsPage"); }
    bool openSettings()
    {
        if (app.isVisible("settingsPage"))
            return true;
        return app.click("navButton")
               && UiApp::waitFor([&] { return app.find("navDrawer")->property("position").toReal() >= 1.0; })
               && app.click("navItem.settings")
               && app.waitVisible("settingsPage")
               && waitForPage();
    }
    // Scrolls a setting into view
    QQuickItem* setting(const QString& key)
    {
        QQuickItem* item = app.item("setting." + key);
        QQuickItem* flickable = app.item("settingsFlickable");
        if (!item || !flickable)
            return nullptr;
        // Position in the content = position in the view + scroll position
        const qreal contentY = flickable->property("contentY").toReal();
        const qreal y = item->mapToItem(flickable, QPointF(0, 0)).y() + contentY;
        const qreal maxY = qMax(0.0, flickable->property("contentHeight").toReal() - flickable->height());
        flickable->setProperty("contentY", qBound(0.0, y - flickable->height() / 3, maxY));
        UiApp::settle(100);
        return item;
    }
    bool chooseComboItem(const QString& key, int index)
    {
        if (!app.click(setting(key)))
            return false;
        UiApp::settle(); // Popup animation
        if (!app.waitVisible("comboItem." + QString::number(index)))
            return false;
        const bool clicked = app.click("comboItem." + QString::number(index));
        UiApp::settle();
        return clicked;
    }
};

void UiSettingsTests::initTestCase()
{
    // Settings of an existing installation
    UiApp::writeSettings({
        {"addStationNameToWindowTitleState", true},
        {"qQStyleTheme", 1},     // dark
        {"accentColor", 2},      // deep orange
        {"enableAGCState", false},
        {"manualGainState", 30},
        {"enableExpertModeState", false},
    });
    UI_START_WITH_CLIP(app, clipId);
}

void UiSettingsTests::init()
{
    app.resize(960, 640);
}

void UiSettingsTests::showsExistingSettings()
{
    QVERIFY(openSettings());

    QVERIFY(app.property("setting.addStationNameToWindowTitleState", "checked").toBool());
    QVERIFY(!app.property("setting.enableAGCState", "checked").toBool());
    QCOMPARE(app.find("receiverSettings")->property("manualGainState").toInt(), 30);
    QCOMPARE(app.property("setting.qQStyleTheme", "currentIndex").toInt(), 1);
    QCOMPARE(app.property("setting.accentColor", "currentIndex").toInt(), 2);
    QCOMPARE(app.find("receiverSettings")->property("device").toInt(), 5); // RAW file
    QCOMPARE(app.property("setting.device", "displayText").toString(), QString("RAW file"));

    // Receiver section: the input device comes first, then the gain
    const QStringList receiverOrder = {"setting.enableAutoSdr", "setting.device",
                                       "setting.enableAGCState", "setting.manualGainState"};
    for (int i = 1; i < receiverOrder.size(); ++i) {
        QQuickItem *above = app.item(receiverOrder[i - 1]);
        QQuickItem *below = app.item(receiverOrder[i]);
        QVERIFY(above && below);
        QVERIFY2(above->mapToScene(QPointF()).y() < below->mapToScene(QPointF()).y(),
                 qPrintable(receiverOrder[i - 1] + " is not above " + receiverOrder[i]));
    }

    // ... and applied
    QVERIFY(app.root()->property("darkTheme").toBool());
    QVERIFY(app.root()->property("isStationNameInWindowTitle").toBool());
    QVERIFY(!app.radio()->property("agc").toBool());

    // A RAW file has no gain to set (like an RTL-SDR stick with the Linux
    // kernel driver): the manual gain stays disabled although the
    // automatic gain is off, and the settings say why
    QCOMPARE(app.radio()->property("gainCount").toInt(), 0);
    QVERIFY(!app.item("setting.manualGainState")->isEnabled());
    QCOMPARE(app.property("setting.manualGainRow", "description").toString(),
             QString("The input device has no adjustable gain"));
    QCOMPARE(app.property("setting.enableAGCState", "description").toString(),
             QString("No effect with this input device"));

    qInfo() << "The settings of an existing installation are shown and applied";
}

void UiSettingsTests::opensFromDrawerAndList()
{
    QVERIFY(openSettings());
    QCOMPARE(app.text("appTitle"), settingsPage()->property("title").toString());
    app.key(Qt::Key_Escape);
    QTRY_VERIFY(!app.isVisible("settingsPage"));
    QVERIFY(waitForPage());

    // "Station settings" of the station list menu shows the stations section
    QVERIFY(app.click("stationListMenuButton"));
    QVERIFY(app.waitVisible("stationSettingsItem"));
    QVERIFY(app.click("stationSettingsItem"));
    QTRY_VERIFY(app.isVisible("settingsPage"));
    QTRY_COMPARE(settingsPage()->property("currentSection").toString(), QString("stations"));
    app.key(Qt::Key_Escape);
    QTRY_VERIFY(!app.isVisible("settingsPage"));

    qInfo() << "The settings open from the drawer and from the station list menu";
}

void UiSettingsTests::switchesByClickingTheRow()
{
    QVERIFY(openSettings());
    const QString station = app.radio()->property("title").toString().trimmed();

    QVERIFY(app.click(setting("addStationNameToWindowTitleState")));
    QTRY_VERIFY(!app.root()->property("isStationNameInWindowTitle").toBool());
    QCOMPARE(app.window()->title(), QString("welle.io"));

    QVERIFY(app.click(setting("addStationNameToWindowTitleState")));
    QTRY_VERIFY(app.root()->property("isStationNameInWindowTitle").toBool());
    QCOMPARE(app.window()->title(), station + " - welle.io");

    // Automatic RF gain: switched on, the manual gain is disabled
    QVERIFY(app.click(setting("enableAGCState")));
    QTRY_VERIFY(app.radio()->property("agc").toBool());
    QVERIFY(!app.item("setting.manualGainState")->isEnabled());

    qInfo() << "Clicking a switch row changes the setting";
}

void UiSettingsTests::choosesFromComboBox()
{
    QVERIFY(openSettings());

    QVERIFY(chooseComboItem("qQStyleTheme", 0)); // light
    QTRY_VERIFY(!app.root()->property("darkTheme").toBool());
    app.screenshot("settings-light");

    QVERIFY(chooseComboItem("qQStyleTheme", 1)); // dark
    QTRY_VERIFY(app.root()->property("darkTheme").toBool());
    app.screenshot("settings-dark");

    QVERIFY(chooseComboItem("accentColor", 4)); // blue
    QTRY_COMPARE(app.property("setting.accentColor", "currentIndex").toInt(), 4);

    qInfo() << "Theme and accent color are chosen from the combo boxes";
}

void UiSettingsTests::enablesExpertMode()
{
    QVERIFY(openSettings());
    QVERIFY(!app.item("setting.disableCoarseState")->isEnabled());

    QVERIFY(app.click(setting("enableExpertModeState")));
    QTRY_VERIFY(app.root()->property("isExpertView").toBool());
    QVERIFY(app.item("setting.disableCoarseState")->isEnabled());

    // The home page shows the expert views instead of "now playing"
    app.key(Qt::Key_Escape);
    QTRY_VERIFY(app.isVisible("homePage"));
    QVERIFY(waitForPage());
    QVERIFY(app.isVisible("addViewButton"));
    QVERIFY(!app.isVisible("nowPlayingPane"));
    QVERIFY(app.isVisible("manualChannelBox"));
    // Band III only, 5A to 13F (no L-Band)
    QCOMPARE(app.item("manualChannelBox")->property("count").toInt(), 38);

    QVERIFY(openSettings());
    QVERIFY(app.click(setting("enableExpertModeState")));
    QTRY_VERIFY(!app.root()->property("isExpertView").toBool());
    app.key(Qt::Key_Escape);
    QTRY_VERIFY(app.isVisible("nowPlayingPane"));

    qInfo() << "Expert mode switches the home page to the expert views";
}

void UiSettingsTests::keepsInputDeviceOpen()
{
    for (int i = 0; i < 2; i++) {
        QVERIFY(openSettings());
        QVERIFY(app.isVisible("rawFileSettings"));
        app.key(Qt::Key_Escape);
        QTRY_VERIFY(!app.isVisible("settingsPage"));
        QVERIFY(waitForPage());
    }
    QCOMPARE(app.radio()->property("deviceId").value<CDeviceID>(), CDeviceID::RAWFILE);
    QVERIFY(app.radio()->property("deviceName").toString().contains(QFileInfo(app.clipFile()).fileName()));

    qInfo() << "Opening and closing the settings keeps the RAW file open";
}

void UiSettingsTests::indexScrollsToSection()
{
    app.resize(1200, 700);
    QVERIFY(openSettings());
    QVERIFY(app.isVisible("settingsIndex.expert"));

    QVERIFY(app.click("settingsIndex.expert"));
    QTRY_COMPARE(settingsPage()->property("currentSection").toString(), QString("expert"));
    QVERIFY(app.item("settingsIndex.expert")->property("highlighted").toBool());

    QVERIFY(app.click("settingsIndex.general"));
    QTRY_COMPARE(settingsPage()->property("currentSection").toString(), QString("general"));

    // No index on narrow windows
    app.resize(700, 640);
    QVERIFY(!app.isVisible("settingsIndex.expert"));

    qInfo() << "The section index scrolls to the sections";
}

void UiSettingsTests::layout_data()
{
    QTest::addColumn<int>("width");
    QTest::newRow("360") << 360;
    QTest::newRow("700") << 700;
    QTest::newRow("1200") << 1200;
}

void UiSettingsTests::layout()
{
    QFETCH(int, width);
    app.resize(width, 700);
    QVERIFY(openSettings());

    QQuickItem* flickable = app.item("settingsFlickable");
    const qreal maxY = qMax(0.0, flickable->property("contentHeight").toReal() - flickable->height());
    for (qreal y = 0;; y = qMin(y + flickable->height(), maxY)) {
        flickable->setProperty("contentY", y);
        UiApp::settle(100);
        UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
        UI_VERIFY_EMPTY(app.controlsWithoutAccessibleName(), "Controls without accessible name");
        if (y == 0)
            app.screenshot(QString("settings-%1").arg(width));
        if (y >= maxY)
            break;
    }
}

void UiSettingsTests::switchesLanguage()
{
    QVERIFY(openSettings());
    const int theme = app.property("setting.qQStyleTheme", "currentIndex").toInt();
    QCOMPARE(app.property("setting.qQStyleTheme", "displayText").toString(), QString(theme == 1 ? "Dark" : "Light"));

    // Deutsch: page title and the entries of the combo boxes are translated
    QVERIFY(chooseComboItem("languageValue", 1));
    QTRY_COMPARE(app.text("appTitle"), QString("Einstellungen"));
    QCOMPARE(app.property("setting.qQStyleTheme", "displayText").toString(), QString(theme == 1 ? "Dunkel" : "Hell"));
    app.screenshot("settings-german");

    // Back to the language of the system (English in the tests)
    QVERIFY(chooseComboItem("languageValue", 0));
    QTRY_COMPARE(app.text("appTitle"), QString("Settings"));
    QCOMPARE(app.property("setting.qQStyleTheme", "displayText").toString(), QString(theme == 1 ? "Dark" : "Light"));

    qInfo() << "The language changes to German and back";
}

void UiSettingsTests::keepsChangesAfterRestart()
{
    // Changed above: accent blue, AGC on, dark theme
    QVERIFY(app.restart());
    QVERIFY(openSettings());
    QCOMPARE(app.property("setting.accentColor", "currentIndex").toInt(), 4);
    QVERIFY(app.property("setting.enableAGCState", "checked").toBool());
    QVERIFY(app.root()->property("darkTheme").toBool());
    QTRY_COMPARE(app.radio()->property("deviceId").value<CDeviceID>(), CDeviceID::RAWFILE);

    qInfo() << "Changed settings survive a restart";
}

WELLE_GUI_TEST_MAIN(UiSettingsTests)

#include "tst_ui_settings.moc"
