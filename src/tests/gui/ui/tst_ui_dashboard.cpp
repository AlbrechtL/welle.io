/*
 *    Copyright (C) 2026
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    This file was generated with an AI assistant (Claude Code).
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

// gui.ui.dashboard: the expert views. The grid of welle.io 2.7 is taken
// over, every view can be added and gets data while a station plays,
// cards are maximized, moved and removed, the layout survives a restart
// and leaving the expert mode keeps the views of the normal mode. The
// service details tell whether the ensemble provides station logos (SPI).

#include <QAbstractSeries>

#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_test_case.h"

static const QString clipId = "odr_dabmod";
// Views by name, and their files in the saved grid of welle.io 2.7
static const QString radioView = "serviceOverview";
static const QString motView = "slideShow";
static const QString spectrumView = "spectrum";
static const QString radioViewFile = "qrc:/QML/RadioView.qml";
static const QString motViewFile = "qrc:/QML/MotView.qml";
static const QString spectrumViewFile = "qrc:/QML/expertviews/SpectrumGraph.qml";

class UiDashboardTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void takesOverOldGrid();
    void addsEveryView();
    void showsSpiStatus();
    void graphsGetData_data();
    void graphsGetData();
    void showsImpulseResponseDelay();
    void maximizesCard();
    void movesCard();
    void removesCard();
    void keepsLayoutAfterRestart();
    void layout_data();
    void layout();
    void leavingExpertModeKeepsNormalViews();

private:
    QStringList views() const { return app.find("dashboard")->property("views").toStringList(); }

    QObject* child(QObject* parent, const QString& objectName) const
    {
        for (QObject* object : parent->findChildren<QObject*>(objectName))
            return object;
        return nullptr;
    }

    // Clicks an entry of the menu of a card
    bool cardMenu(const QString& card, const QString& entry)
    {
        QObject* cardItem = app.item("card." + card);
        if (!cardItem)
            return false;
        auto* button = qobject_cast<QQuickItem*>(child(cardItem, "cardMenuButton"));
        if (!app.click(button))
            return false;
        UiApp::settle();
        const bool clicked = app.click(entry);
        UiApp::settle();
        return clicked;
    }
};

void UiDashboardTests::initTestCase()
{
    // Expert mode with the grid of welle.io 2.7: spectrum in the second
    // column, service overview and slide show below each other in the first
    UiApp::writeSettings({
        {"enableExpertModeState", true},
        {"viewComponents", QString("[[\"%1\",\"%2\",\"%3\"],[0,0,1],[1,0,0]]").arg(spectrumViewFile, radioViewFile, motViewFile)},
    });
    UI_START_WITH_CLIP(app, clipId);
    QVERIFY(app.isVisible("dashboard"));

    // A station with slide show plays
    QVERIFY(scanStations());
    for (const QJsonObject& service : app.services()) {
        if (service["type"].toString() == "DAB+")
            QVERIFY(app.click(stationRow(service["sid"].toInt())));
    }
    QTRY_VERIFY_WITH_TIMEOUT(app.radio()->property("isPlaying").toBool(), 10000);
}

void UiDashboardTests::init()
{
    app.resize(1400, 800);
}

void UiDashboardTests::takesOverOldGrid()
{
    QCOMPARE(views(), QStringList({radioView, motView, spectrumView}));
    QVERIFY(app.isVisible("card.serviceOverview"));
    QVERIFY(app.isVisible("card.slideShow"));
    QVERIFY(app.isVisible("card.spectrum"));

    qInfo() << "The grid of welle.io 2.7 is taken over column by column";
}

void UiDashboardTests::addsEveryView()
{
    QObject* dashboard = app.find("dashboard");
    const QVariantList types = dashboard->property("viewTypes").toList();

    for (const QVariant& value : types) {
        const QVariantMap type = value.toMap();
        const QString name = type["name"].toString();
        const bool present = views().contains(name);

        QVERIFY(app.click("addViewButton"));
        QVERIFY(app.waitVisible("addView." + name));
        UiApp::settle();
        QCOMPARE(app.item("addView." + name)->isEnabled(), !present);
        if (present) {
            app.key(Qt::Key_Escape);
            UiApp::settle();
            continue;
        }
        QVERIFY(app.click("addView." + name));
        QTRY_VERIFY2(app.find("card." + name), qPrintable("No card for " + name));
        UiApp::settle();
    }

    QCOMPARE(views().size(), types.size());
    app.screenshot("dashboard-all-views");

    qInfo().noquote() << "All" << types.size() << "views added";
}

void UiDashboardTests::showsSpiStatus()
{
    // The recording has no SPI service: no station logos from this ensemble
    QObject* card = app.find("card.serviceDetails");
    QVERIFY(card);
    QObject* spi = child(card, "spiDetail");
    QVERIFY(spi);
    QTRY_COMPARE_WITH_TIMEOUT(app.radio()->property("spiState").toInt(), int(CRadioController::SpiNotAvailable), 30000);
    QCOMPARE(spi->property("value").toString(), QString("Not provided by this ensemble"));
    app.scrollIntoView(qobject_cast<QQuickItem*>(card));
    app.screenshot("dashboard-spi");

    qInfo().noquote() << "Service details, station logos (SPI):" << spi->property("value").toString();
}

void UiDashboardTests::graphsGetData_data()
{
    QTest::addColumn<QString>("card");
    QTest::newRow("spectrum") << "spectrum";
    QTest::newRow("impulseResponse") << "impulseResponse";
    QTest::newRow("constellation") << "constellation";
    QTest::newRow("nullSymbol") << "nullSymbol";
}

void UiDashboardTests::graphsGetData()
{
    QFETCH(QString, card);

    QObject* cardItem = app.find("card." + card);
    QVERIFY(cardItem);
    QObject* chart = nullptr;
    for (QObject* object : cardItem->findChildren<QObject*>()) {
        if (object->inherits("DeclarativeChart"))
            chart = object;
    }
    QVERIFY2(chart, "No chart in the card");

    QAbstractSeries* series = nullptr;
    QVERIFY(QMetaObject::invokeMethod(chart, "series", Q_RETURN_ARG(QAbstractSeries*, series), Q_ARG(int, 0)));
    QVERIFY(series);
    QTRY_VERIFY_WITH_TIMEOUT(series->property("count").toInt() > 0, 10000);

    qInfo().noquote() << card << "shows" << series->property("count").toInt() << "points";
}

void UiDashboardTests::showsImpulseResponseDelay()
{
    // Delay and distance of the impulse response at the mouse position
    QQuickItem* card = app.item("card.impulseResponse");
    QVERIFY(card);
    app.scrollIntoView(card);
    const QPoint center = card->mapToScene(QPointF(card->width() / 2, card->height() / 2)).toPoint();
    QTest::mouseMove(app.window(), center);
    QTest::mouseMove(app.window(), center + QPoint(10, 5));
    QTRY_VERIFY(app.isVisible("delayInfo"));
    // "sample: 971", "474.1 µs", "142.2 km"
    const QStringList lines = app.text("delayInfo").split('\n');
    QCOMPARE(lines.size(), 3);
    const double sample = lines[0].section(' ', 1, 1).toDouble();
    const double us = lines[1].section(' ', 0, 0).toDouble();
    const double km = lines[2].section(' ', 0, 0).toDouble();
    QVERIFY(qAbs(us - sample / 2.048) < 0.3); // The sample is rounded
    QVERIFY2(qAbs(km - us * 0.3) < 0.1, qPrintable(lines.join(", ")));

    qInfo().noquote() << "Impulse response at the mouse:" << app.text("delayInfo").replace('\n', ", ");
}

void UiDashboardTests::maximizesCard()
{
    QVERIFY(cardMenu("spectrum", "cardMaximize"));
    QTRY_COMPARE(app.find("dashboard")->property("maximizedView").toString(), spectrumView);
    QVERIFY(app.isVisible("card.spectrum"));
    QVERIFY(!app.isVisible("card.serviceOverview"));
    QQuickItem* dashboard = app.item("dashboard");
    QVERIFY(app.item("card.spectrum")->height() > dashboard->height() * 0.8);
    app.screenshot("dashboard-maximized");

    QVERIFY(cardMenu("spectrum", "cardMaximize"));
    QTRY_COMPARE(app.find("dashboard")->property("maximizedView").toString(), QString());
    QVERIFY(app.isVisible("card.serviceOverview"));

    qInfo() << "A card is maximized and restored";
}

void UiDashboardTests::movesCard()
{
    QStringList before = views();
    const int index = before.indexOf(spectrumView);
    QVERIFY(index > 0);

    QVERIFY(cardMenu("spectrum", "cardMoveEarlier"));
    QStringList expected = before;
    expected.move(index, index - 1);
    QTRY_COMPARE(views(), expected);

    // The first card can't move up
    QObject* first = app.find("card.serviceOverview");
    QCOMPARE(first->property("index").toInt(), 0);
    QVERIFY(cardMenu("serviceOverview", "cardMoveLater"));
    QTRY_COMPARE(views().indexOf(radioView), 1);
    QVERIFY(cardMenu("serviceOverview", "cardMoveEarlier"));
    QTRY_COMPARE(views().indexOf(radioView), 0);

    qInfo() << "Cards are moved with the card menu";
}

void UiDashboardTests::removesCard()
{
    const int count = views().size();
    QVERIFY(cardMenu("console", "cardRemove"));
    QTRY_COMPARE(views().size(), count - 1);
    QVERIFY(!views().contains("console"));

    qInfo() << "A card is removed";
}

void UiDashboardTests::keepsLayoutAfterRestart()
{
    const QStringList before = views();
    QVERIFY(app.restart());
    QCOMPARE(views(), before);

    qInfo() << "The dashboard survives a restart";
}

void UiDashboardTests::layout_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("columns");
    QTest::newRow("compact-360") << 360 << 1;
    QTest::newRow("800") << 800 << 1;
    QTest::newRow("1400") << 1400 << 2;
    QTest::newRow("1920") << 1920 << 3;
}

void UiDashboardTests::layout()
{
    QFETCH(int, width);
    QFETCH(int, columns);

    app.resize(width, 800);
    if (app.property("homePage", "layoutMode").toString() == "compact") {
        QVERIFY(app.click("miniPlayer"));
        QTRY_VERIFY(app.isVisible("dashboard"));
    }
    QVERIFY(app.isVisible("dashboard"));
    QCOMPARE(app.property("dashboard", "columns").toInt(), columns);

    UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
    UI_VERIFY_EMPTY(app.controlsWithoutAccessibleName(), "Controls without accessible name");
    app.screenshot(QString("dashboard-%1").arg(QTest::currentDataTag()));

    if (app.property("homePage", "detailOpen").toBool())
        app.key(Qt::Key_Escape);
}

void UiDashboardTests::leavingExpertModeKeepsNormalViews()
{
    app.find("expertSettings")->setProperty("enableExpertModeState", false);
    QTRY_VERIFY(!app.root()->property("isExpertView").toBool());
    QCOMPARE(views(), QStringList({radioView, motView}));
    QVERIFY(app.isVisible("nowPlayingPane"));

    qInfo() << "Without expert mode only service overview and slide show stay";
}

WELLE_GUI_TEST_MAIN(UiDashboardTests)

#include "tst_ui_dashboard.moc"
