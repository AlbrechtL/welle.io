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

// gui.ui.navigation: the navigation drawer behind the ☰ button. It opens
// with the button, closes with Esc, a click on the dimmed window and after
// a selection, highlights the current destination and leads to the
// station lists, settings, about and exit.

#include "gui_test_main.h"
#include "ui_test_case.h"

static const QString clipId = "odr_dabmod";

class UiNavigationTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void defaultAccentIsBlue();
    void opensAndClosesDrawer();
    void drawerFitsSmallWindows();
    void switchesStationList();
    void opensPages_data();
    void opensPages();
    void aboutTabs_data();
    void aboutTabs();
    void exits();

private:
    bool drawerOpen() const { return app.find("navDrawer")->property("position").toReal() >= 1.0; }
    bool drawerClosed() const { return !app.find("navDrawer")->property("visible").toBool(); }
    bool openDrawer()
    {
        const bool open = app.click("navButton") && UiApp::waitFor([&] { return drawerOpen(); });
        UiApp::settle(100);
        return open;
    }
};

void UiNavigationTests::initTestCase()
{
    UI_START_WITH_CLIP(app, clipId);
}

void UiNavigationTests::init()
{
    // Back to the home page after a failed test
    if (!drawerClosed()) {
        app.key(Qt::Key_Escape);
        QTRY_VERIFY(drawerClosed());
    }
    while (app.isVisible("backButton")) {
        app.key(Qt::Key_Escape);
        QVERIFY(waitForPage());
    }
    app.resize(960, 640);
}

void UiNavigationTests::defaultAccentIsBlue()
{
    // Fresh settings: blue accent color
    QCOMPARE(app.find("appearanceSettings")->property("accentColor").toInt(), 4);
    QCOMPARE(app.property("setting.accentColor", "displayText").toString(), QString("Blue"));
}

void UiNavigationTests::opensAndClosesDrawer()
{
    QVERIFY(drawerClosed());

    // ☰ opens, Esc closes
    QVERIFY(openDrawer());
    QVERIFY(app.isVisible("navItem.all"));
    UI_VERIFY_EMPTY(app.controlsWithoutAccessibleName(), "Controls without accessible name");
    app.screenshot("navigation-drawer");
    app.key(Qt::Key_Escape);
    QTRY_VERIFY(drawerClosed());

    // A click on the dimmed window right of the drawer closes it
    QVERIFY(openDrawer());
    QVERIFY(app.clickAt(QPointF(app.window()->width() - 20, app.window()->height() / 2)));
    QTRY_VERIFY(drawerClosed());

    qInfo() << "The drawer opens with the menu button and closes with Esc and a click outside";
}

void UiNavigationTests::drawerFitsSmallWindows()
{
    app.resize(360, 640);
    QVERIFY(openDrawer());

    // Part of the window stays visible, to close the drawer
    const qreal width = app.find("navDrawer")->property("width").toReal();
    QVERIFY2(width <= 360 * 0.85 + 1, qPrintable(QString("Drawer width %1").arg(width)));
    UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
    app.screenshot("navigation-drawer-compact");

    app.key(Qt::Key_Escape);
    QTRY_VERIFY(drawerClosed());
}

void UiNavigationTests::switchesStationList()
{
    QObject* drawer = app.find("navDrawer");
    QCOMPARE(drawer->property("current").toString(), QString("all"));
    QCOMPARE(app.text("stationListTitle"), QString("All stations"));

    // Back and forth several times: the drawer is the only place to choose
    // the list and must keep working
    for (int i = 0; i < 3; i++) {
        QVERIFY(showList("favorites"));
        QTRY_COMPARE(app.text("stationListTitle"), QString("Favorites"));
        QCOMPARE(drawer->property("current").toString(), QString("favorites"));
        QCOMPARE(app.item("stationListView")->property("model").value<QObject*>(), app.find("favoritesList"));
        QVERIFY(app.item("navItem.favorites")->property("highlighted").toBool());
        QVERIFY(!app.item("navItem.all")->property("highlighted").toBool());

        QVERIFY(showList("all"));
        QTRY_COMPARE(app.text("stationListTitle"), QString("All stations"));
        QCOMPARE(drawer->property("current").toString(), QString("all"));
        QCOMPARE(app.item("stationListView")->property("model").value<QObject*>(), app.find("stationList"));
    }

    qInfo() << "The drawer switches between all stations and favorites, repeatedly";
}

void UiNavigationTests::opensPages_data()
{
    QTest::addColumn<QString>("destination");
    QTest::addColumn<QString>("page");

    QTest::newRow("settings") << "settings" << "settingsPage";
    QTest::newRow("about") << "about" << "aboutPage";
}

void UiNavigationTests::opensPages()
{
    QFETCH(QString, destination);
    QFETCH(QString, page);

    QVERIFY(openDrawer());
    QVERIFY(app.click("navItem." + destination));
    QTRY_VERIFY(drawerClosed());
    QTRY_VERIFY(app.isVisible(page));
    QVERIFY(waitForPage());
    QCOMPARE(app.find("navDrawer")->property("current").toString(), destination);
    QCOMPARE(app.text("appTitle"), app.find(page)->property("title").toString());
    QVERIFY(app.isVisible("backButton"));
    UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
    UI_VERIFY_EMPTY(app.controlsWithoutAccessibleName(), "Controls without accessible name");
    app.screenshot("navigation-" + destination);

    // Esc and the back button go back to the station list
    app.key(Qt::Key_Escape);
    QTRY_VERIFY(!app.isVisible(page));
    QVERIFY(waitForPage());
    QVERIFY(app.isVisible("stationListPane"));

    QVERIFY(openDrawer());
    QVERIFY(app.click("navItem." + destination));
    QTRY_VERIFY(drawerClosed());
    QTRY_VERIFY(app.isVisible(page));
    QVERIFY(waitForPage());
    QVERIFY(app.click("backButton"));
    QTRY_VERIFY(!app.isVisible(page));
    QVERIFY(waitForPage());
    QVERIFY(app.isVisible("navButton"));

    qInfo().noquote() << "The drawer opens" << page;
}

void UiNavigationTests::aboutTabs_data()
{
    QTest::addColumn<int>("width");
    QTest::newRow("360") << 360;
    QTest::newRow("960") << 960;
}

void UiNavigationTests::aboutTabs()
{
    QFETCH(int, width);
    app.resize(width, 640);

    QVERIFY(openDrawer());
    QVERIFY(app.click("navItem.about"));
    QTRY_VERIFY(drawerClosed());
    QTRY_VERIFY(app.isVisible("aboutPage"));
    QVERIFY(waitForPage());

    // Every tab shows its own text, only the licenses tab the license chips
    const QList<std::pair<QString, QString>> tabs = {
        {"versions", "welle.io version"},
        {"authors", ""},
        {"thanks", ""},
        {"licenses", ""},
    };
    QStringList texts;
    for (const auto& [tab, expected] : tabs) {
        QVERIFY(app.click("aboutTab." + tab));
        QTRY_VERIFY(app.item("aboutTab." + tab)->property("checked").toBool());
        const QString text = app.text("aboutContent");
        QVERIFY2(!text.isEmpty(), qPrintable("No text on the tab " + tab));
        QVERIFY2(text.contains(expected), qPrintable(tab + ": " + text.left(80)));
        QVERIFY(!texts.contains(text));
        texts << text;
        QCOMPARE(app.isVisible("licenseChip.0"), tab == "licenses");
        UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
        UI_VERIFY_EMPTY(app.controlsWithoutAccessibleName(), "Controls without accessible name");
    }
    UiApp::settle(); // Tab indicator animation
    app.screenshot(QString("about-licenses-%1").arg(width));

    // The chips choose the license
    QVERIFY(app.click("licenseChip.0"));
    QTRY_VERIFY(app.text("aboutContent").contains("GNU GENERAL PUBLIC LICENSE"));
    QVERIFY(app.click("licenseChip.1"));
    QTRY_VERIFY(app.text("aboutContent").contains("LESSER GENERAL PUBLIC LICENSE"));
    QVERIFY(app.item("licenseChip.1")->property("checked").toBool());
    QVERIFY(!app.item("licenseChip.0")->property("checked").toBool());
    QVERIFY(app.click("licenseChip.2"));
    QTRY_VERIFY(app.text("aboutContent").contains("Redistribution"));

    QVERIFY(app.click("aboutTab.versions"));
    QTRY_VERIFY(app.text("aboutContent").contains("welle.io version"));
    // The versions of the libraries follow the Qt version
    QVERIFY2(app.text("aboutContent").contains("mpg123 version: "), qPrintable(app.text("aboutContent")));
    QVERIFY(!app.isVisible("licenseChip.0"));
    UiApp::settle();
    app.screenshot(QString("about-versions-%1").arg(width));

    app.key(Qt::Key_Escape);
    QTRY_VERIFY(!app.isVisible("aboutPage"));
    QVERIFY(waitForPage());

    qInfo() << "The about tabs show versions, authors, thanks and the three licenses";
}

void UiNavigationTests::exits()
{
    // MainView calls guiHelper.close() for "exit", which calls
    // QApplication::quit(). That does nothing without the event loop of
    // exec(), which the tests don't run.
    QSignalSpy navigate(app.find("navDrawer"), SIGNAL(navigate(QString)));
    QVERIFY(openDrawer());
    QVERIFY(app.click("navItem.exit"));
    QTRY_COMPARE(navigate.count(), 1);
    QCOMPARE(navigate.first().first().toString(), QString("exit"));

    qInfo() << "Exit asks MainView to quit";
}

WELLE_GUI_TEST_MAIN(UiNavigationTests)

#include "tst_ui_navigation.moc"
