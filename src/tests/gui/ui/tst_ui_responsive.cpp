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

// gui.ui.responsive: the layout at window widths from a phone to a wide
// desktop window, in the light and the dark theme, while a station with
// slide show plays. At every size: which panes are shown, nothing sticks
// out of the window and every control has an accessible name.

#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_test_case.h"

static const QString clipId = "odr_dabmod";

class UiResponsiveTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();

    void layout_data();
    void layout();
    void compactPlayerPage();

private:
    void setTheme(int theme)
    {
        // Settings: 0 light, 1 dark, 2 system
        QObject* settings = app.find("appearanceSettings");
        QVERIFY(settings);
        settings->setProperty("qQStyleTheme", theme);
        UiApp::settle();
    }
};

void UiResponsiveTests::initTestCase()
{
    UI_START_WITH_CLIP(app, clipId);
    QVERIFY(scanStations());

    // A station with a slide show and DLS, so that every part is filled
    QJsonObject withSlides;
    for (const QJsonObject& service : app.services()) {
        if (service["type"].toString() == "DAB+")
            withSlides = service;
    }
    QVERIFY(!withSlides.isEmpty());
    QVERIFY(app.click(stationRow(withSlides["sid"].toInt())));
    QTRY_VERIFY_WITH_TIMEOUT(!app.text("nowPlaying.dls").isEmpty(), 20000);
    QTRY_VERIFY_WITH_TIMEOUT(!app.property("nowPlaying.slide", "source").toString().isEmpty(), 40000);
}

void UiResponsiveTests::layout_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::addColumn<QString>("layoutMode");
    QTest::addColumn<int>("theme");

    const QList<std::tuple<int, int, QString>> sizes = {
        {360, 640, "compact"},
        {360, 740, "compact"},
        {599, 640, "compact"},
        {600, 640, "medium"},
        {800, 600, "medium"},
        {1000, 700, "expanded"},
        {1600, 900, "expanded"},
    };
    for (int theme : {0, 1}) {
        for (const auto& [width, height, mode] : sizes) {
            const QString name = QString("%1x%2-%3").arg(width).arg(height).arg(theme ? "dark" : "light");
            QTest::newRow(qPrintable(name)) << width << height << mode << theme;
        }
    }
}

void UiResponsiveTests::layout()
{
    QFETCH(int, width);
    QFETCH(int, height);
    QFETCH(QString, layoutMode);
    QFETCH(int, theme);

    setTheme(theme);
    app.resize(width, height);

    QCOMPARE(app.property("homePage", "layoutMode").toString(), layoutMode);

    QQuickItem* list = app.item("stationListPane");
    QVERIFY(list && list->isVisible());
    if (layoutMode == "compact") {
        QVERIFY(app.isVisible("miniPlayer"));
        QVERIFY(!app.isVisible("detailPane"));
        QCOMPARE(list->width(), qreal(width));
        QCOMPARE(app.text("miniPlayer.station"), app.radio()->property("title").toString().trimmed());
    } else {
        QVERIFY(!app.isVisible("miniPlayer"));
        QVERIFY(app.isVisible("detailPane"));
        QVERIFY(app.isVisible("nowPlayingPane"));
        QVERIFY(list->width() <= width * 0.45 + 1);
        QVERIFY(app.isVisible("nowPlaying.station"));
        QVERIFY(app.isVisible("nowPlaying.slide"));
    }

    UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
    UI_VERIFY_EMPTY(app.controlsWithoutAccessibleName(), "Controls without accessible name");

    app.screenshot(QString("responsive-%1").arg(QTest::currentDataTag()));
}

void UiResponsiveTests::compactPlayerPage()
{
    setTheme(0);
    app.resize(360, 640);

    // Mini player -> player page
    QVERIFY(app.click("miniPlayer"));
    QTRY_VERIFY(app.isVisible("detailPane"));
    QVERIFY(!app.isVisible("miniPlayer"));
    QVERIFY(!app.isVisible("stationListPane"));
    QVERIFY(app.isVisible("backButton"));
    QVERIFY(!app.isVisible("navButton"));
    QVERIFY(app.isVisible("nowPlaying.station"));
    UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
    UI_VERIFY_EMPTY(app.controlsWithoutAccessibleName(), "Controls without accessible name");
    app.screenshot("responsive-player-page");

    // Back with the button
    QVERIFY(app.click("backButton"));
    QTRY_VERIFY(app.isVisible("stationListPane"));
    QVERIFY(app.isVisible("miniPlayer"));
    QVERIFY(app.isVisible("navButton"));

    // Back with Esc
    QVERIFY(app.click("miniPlayer"));
    QTRY_VERIFY(app.isVisible("detailPane"));
    app.key(Qt::Key_Escape);
    QTRY_VERIFY(app.isVisible("stationListPane"));

    // A wider window closes the player page, both panes are shown
    QVERIFY(app.click("miniPlayer"));
    QTRY_VERIFY(app.isVisible("detailPane"));
    app.resize(900, 640);
    QVERIFY(app.isVisible("stationListPane"));
    QVERIFY(app.isVisible("detailPane"));
    QVERIFY(!app.isVisible("backButton"));

    qInfo() << "The mini player opens the player page, back and Esc close it";
}

WELLE_GUI_TEST_MAIN(UiResponsiveTests)

#include "tst_ui_responsive.moc"
