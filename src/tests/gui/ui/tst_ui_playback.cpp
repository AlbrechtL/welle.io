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

// gui.ui.playback: scans the synthetic ODR recording with the button of
// the empty station list, plays every service by clicking its row and
// checks what the user sees: station name, ensemble, DAB/DAB+, DLS and the
// slide show. Then the play/stop button and the keyboard shortcuts.

#include <QBuffer>
#include <QPainter>

#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_test_case.h"

static const QString clipId = "odr_dabmod";

// The recording plays in real time, these timeouts leave enough margin
static const int timeoutPlay = 10000;
static const int timeoutDls = 15000;
static const int timeoutSlide = 40000;

class UiPlaybackTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();

    void scansStations();
    void playsEachService();
    void showsSlides();
    void showsStationLogo();
    void fitsWithoutScrolling_data();
    void fitsWithoutScrolling();
    void stopsAndPlays();
    void keyboardShortcuts();
    void volumeShortcuts();
    void showsGenreOnTheRight();

private:
    // Clicks the station and waits until it plays and is shown
    bool play(const QJsonObject& service);

    quint32 currentService() const { return app.radio()->property("service").toUInt(); }
    bool isPlaying() const { return app.radio()->property("isPlaying").toBool(); }
    bool showsSlide() const
    {
        const QString source = app.property("nowPlaying.slide", "source").toString();
        return !source.isEmpty() && !source.endsWith("/empty");
    }
};

void UiPlaybackTests::initTestCase()
{
    UI_START_WITH_CLIP(app, clipId);
}

void UiPlaybackTests::scansStations()
{
    QVERIFY2(app.isVisible("scanButton"), "The empty station list shows no scan button");
    QVERIFY(scanStations());

    QMap<quint32, QString> expected;
    for (const QJsonObject& service : app.services())
        expected[service["sid"].toInt()] = service["label"].toString();
    QCOMPARE(shownStations(), expected);
    QTRY_VERIFY(!app.isVisible("scanButton"));

    for (const QJsonObject& service : app.services()) {
        QQuickItem* row = stationRow(service["sid"].toInt());
        QVERIFY2(row, qPrintable("No row for " + service["label"].toString()));
        QCOMPARE(row->property("label").toString().trimmed(), service["label"].toString());
    }
    app.screenshot("playback-stations");

    qInfo().noquote() << "Station list after the scan:" << QStringList(expected.values()).join(", ");
}

bool UiPlaybackTests::play(const QJsonObject& service)
{
    const quint32 sid = service["sid"].toInt();
    const QString label = service["label"].toString();
    if (!app.click(stationRow(sid))) {
        qWarning().noquote() << "Cannot click the row of" << label;
        return false;
    }
    return UiApp::waitFor([&] {
        return currentService() == sid && isPlaying() && app.text("nowPlaying.station") == label;
    }, timeoutPlay);
}

void UiPlaybackTests::playsEachService()
{
    const QJsonObject expect = app.clip()["expect"].toObject();
    const QString ensemble = expect["ensemble_label"].toString();
    int servicesWithDls = 0;

    for (const QJsonObject& service : app.services()) {
        const QString label = service["label"].toString();
        QVERIFY2(play(service), qPrintable("Clicking " + label + " doesn't play it"));
        QTRY_COMPARE_WITH_TIMEOUT(app.text("nowPlaying.ensemble"), ensemble, timeoutPlay);

        // "DAB+" and "AAC-LC, 48 kHz Stereo @ 96 kbit/s". The previous
        // station's stay until the first audio frame of the new one is
        // decoded.
        QTRY_COMPARE_WITH_TIMEOUT(app.text("nowPlaying.dabType"), service["type"].toString(), timeoutPlay);
        const QString bitrate = QString("@ %1 kbit/s").arg(service["bitrate"].toInt());
        QTRY_VERIFY2_WITH_TIMEOUT(app.text("nowPlaying.audioMode").endsWith(bitrate),
                                  qPrintable(app.text("nowPlaying.audioMode") + " doesn't end with " + bitrate), timeoutPlay);
        const QString audioMode = app.text("nowPlaying.dabType") + " " + app.text("nowPlaying.audioMode");

        const bool hasDls = UiApp::waitFor([&] { return !app.text("nowPlaying.dls").isEmpty(); }, timeoutDls);
        servicesWithDls += hasDls;

        qInfo().noquote() << QString("Playing \"%1\": %2, DLS \"%3\"")
                                 .arg(label, audioMode, hasDls ? app.text("nowPlaying.dls") : "-");
    }

    QVERIFY2(servicesWithDls >= expect["min_dls_services"].toInt(),
             qPrintable(QString("%1 services showed a DLS, expected at least %2")
                            .arg(servicesWithDls).arg(expect["min_dls_services"].toInt())));
    app.screenshot("playback-playing");
}

void UiPlaybackTests::showsSlides()
{
    if (app.clip()["expect"].toObject()["min_mot"].toInt() == 0)
        QSKIP("The clip has no slides");

    // The service that is playing now (the last one) first, then the others
    QList<QJsonObject> services = app.services();
    std::reverse(services.begin(), services.end());
    bool shown = false;
    for (int i = 0; i < services.size() && !shown; i++) {
        if (i > 0)
            QVERIFY(play(services[i]));
        shown = UiApp::waitFor([&] { return showsSlide(); }, timeoutSlide);
        if (shown)
            qInfo().noquote() << "Slide of" << services[i]["label"].toString() << "shown:"
                              << app.property("nowPlaying.slide", "source").toString();
    }
    QVERIFY2(shown, "No service showed a slide");
    app.screenshot("playback-slide");
}

static QRectF sceneRect(QQuickItem* item);

void UiPlaybackTests::showsStationLogo()
{
    QVERIFY(!app.isVisible("nowPlaying.logo"));

    // A logo for the station that plays, as the backend delivers it by SPI
    QImage image(128, 128, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor("#e6007e"));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(8, 28, 112, 72), 12, 12);
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(36);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRectF(8, 28, 112, 72), Qt::AlignCenter, "DAB+");
    painter.end();
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    QVERIFY(image.save(&buffer, "PNG"));
    emit app.radio()->serviceLogoReceived(currentService(), png);

    // In one row with the station name, left of it
    QTRY_VERIFY(app.isVisible("nowPlaying.logo"));
    UiApp::settle();
    const QRectF logo = sceneRect(app.item("nowPlaying.logo"));
    const QRectF name = sceneRect(app.item("nowPlaying.station"));
    QVERIFY(logo.right() <= name.left());
    QVERIFY(qAbs(logo.center().y() - name.center().y()) < 2);

    // Side by side with the slide, below each other and on the page of
    // the phone layout
    for (const QSize& size : {QSize(1400, 800), QSize(800, 480), QSize(400, 800)}) {
        app.resize(size.width(), size.height());
        const bool phoneLayout = app.property("homePage", "layoutMode").toString() == "compact";
        if (phoneLayout) {
            // The mini player shows the logo instead of the slide
            QTRY_VERIFY(app.isVisible("miniPlayer.logo"));
            QVERIFY(!app.isVisible("miniPlayer.slide"));
            app.screenshot(QString("playback-logo-miniplayer-%1x%2").arg(size.width()).arg(size.height()));

            QVERIFY(app.click("miniPlayer"));
            QTRY_VERIFY(app.isVisible("nowPlayingPane"));
            UiApp::settle();
        }
        app.screenshot(QString("playback-logo-%1x%2").arg(size.width()).arg(size.height()));
        if (phoneLayout) {
            app.key(Qt::Key_Escape);
            QTRY_VERIFY(app.isVisible("stationListPane"));
        }
    }

    qInfo() << "The logo of the station is shown in one row with the station name";
}

void UiPlaybackTests::fitsWithoutScrolling_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::addColumn<bool>("sideBySide");
    // The pane is the window without the station list (320 px), in the
    // phone layout (below 600 px) the player page of the mini player
    QTest::newRow("360x640") << 360 << 640 << false;
    QTest::newRow("480x800") << 480 << 800 << false;
    QTest::newRow("800x480 (Raspberry Pi 7\")") << 800 << 480 << false;
    QTest::newRow("1000x700") << 1000 << 700 << false;
    QTest::newRow("1200x800") << 1200 << 800 << true;
    QTest::newRow("1600x900") << 1600 << 900 << true;
}

static QRectF sceneRect(QQuickItem* item)
{
    return item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
}

void UiPlaybackTests::fitsWithoutScrolling()
{
    QFETCH(int, width);
    QFETCH(int, height);
    QFETCH(bool, sideBySide);
    app.resize(width, height);
    const bool phoneLayout = app.property("homePage", "layoutMode").toString() == "compact";
    if (phoneLayout) {
        QVERIFY(app.click("miniPlayer"));
        QTRY_VERIFY(app.isVisible("nowPlayingPane"));
        UiApp::settle();
    }
    QCOMPARE(app.property("nowPlayingPane", "sideBySide").toBool(), sideBySide);

    // Everything is visible without scrolling
    QQuickItem* pane = app.item("nowPlayingPane");
    const QRectF paneRect = sceneRect(pane);
    const QRectF slide = sceneRect(app.item("slideCard"));
    const QRectF station = sceneRect(app.item("stationCard"));
    QVERIFY2(!app.property("nowPlayingFlickable", "interactive").toBool(),
             qPrintable(QString("Pane %1 x %2 has to be scrolled").arg(paneRect.width()).arg(paneRect.height())));
    QVERIFY2(paneRect.contains(station) && paneRect.contains(slide),
             qPrintable(QString("Station %1,%2 %3 x %4, slide %5,%6 %7 x %8, pane %9 x %10")
                            .arg(station.x()).arg(station.y()).arg(station.width()).arg(station.height())
                            .arg(slide.x()).arg(slide.y()).arg(slide.width()).arg(slide.height())
                            .arg(paneRect.width()).arg(paneRect.height())));

    // The slide card is 4:3 like the slides
    QVERIFY(qAbs(slide.width() / slide.height() - 4.0 / 3.0) < 0.02);

    if (sideBySide) {
        // Two cards of the same size, the station on the left
        QVERIFY(qAbs(slide.width() - station.width()) < 1 && qAbs(slide.height() - station.height()) < 1);
        QVERIFY(station.right() < slide.left() && qAbs(slide.top() - station.top()) < 1);
    } else {
        // The station on top, the slide below it scaled to the height left.
        // Together they fill the height of the pane (margins 16 px); with a
        // slide of full width the station card takes the rest.
        QVERIFY(station.bottom() < slide.top());
        QVERIFY(slide.width() <= station.width() + 1);
        QVERIFY2(qAbs(station.top() - paneRect.top() - 16) < 2 && qAbs(paneRect.bottom() - slide.bottom() - 16) < 2,
                 qPrintable(QString("Station from %1, slide to %2, pane %3 to %4")
                                .arg(station.top()).arg(slide.bottom()).arg(paneRect.top()).arg(paneRect.bottom())));
    }

    // Station name (in one row with the logo, if there is one) and DLS are
    // centered in the station card
    const QRectF name = sceneRect(app.item("nowPlaying.station"));
    const QRectF dls = sceneRect(app.item("nowPlaying.dls"));
    QQuickItem* logo = app.item("nowPlaying.logo");
    const QRectF nameRow = logo->isVisible() ? name.united(sceneRect(logo)) : name;
    const qreal blockTop = nameRow.top();
    if (!logo->isVisible())
        QCOMPARE(app.item("nowPlaying.station")->property("horizontalAlignment").toInt(), int(Qt::AlignHCenter));
    QCOMPARE(app.item("nowPlaying.dls")->property("horizontalAlignment").toInt(), int(Qt::AlignHCenter));
    QVERIFY2(qAbs(nameRow.center().x() - station.center().x()) < 2,
             qPrintable(QString("Name row center %1, card center %2").arg(nameRow.center().x()).arg(station.center().x())));
    QVERIFY(qAbs(dls.center().x() - station.center().x()) < 2);
    const qreal blockCenter = (blockTop + dls.bottom()) / 2;
    QVERIFY2(qAbs(blockCenter - station.center().y()) < 2,
             qPrintable(QString("Text block center %1, card center %2").arg(blockCenter).arg(station.center().y())));

    // DAB/DAB+ on the left, the audio format in the center, the genre on
    // the right
    QQuickItem* dabType = app.item("nowPlaying.dabType");
    QQuickItem* audioMode = app.item("nowPlaying.audioMode");
    QQuickItem* genre = app.item("nowPlaying.programmeType");
    QVERIFY(dabType->isVisible() && audioMode->isVisible());
    const QRectF dabRect = sceneRect(dabType);
    const QRectF modeRect = sceneRect(audioMode);
    QVERIFY(qAbs(dabRect.left() - station.left()) < 20);
    QVERIFY(dabRect.right() < modeRect.left());
    // Centered, or next to the left chip if it doesn't fit centered
    const bool centered = qAbs(modeRect.center().x() - station.center().x()) < 2;
    const bool besideLeft = qAbs(modeRect.left() - dabRect.right() - 8) < 2;
    QVERIFY2(centered || besideLeft, qPrintable(QString("Audio format at %1..%2, card %3..%4")
                                                    .arg(modeRect.left()).arg(modeRect.right())
                                                    .arg(station.left()).arg(station.right())));
    // Not cut off if there is room for the whole text
    const qreal naturalWidth = audioMode->property("naturalWidth").toReal();
    if (naturalWidth <= station.width() - 2 * 16 - dabRect.width() - 8)
        QVERIFY2(qAbs(audioMode->width() - naturalWidth) < 1,
                 qPrintable(QString("Audio format %1 of %2 px wide").arg(audioMode->width()).arg(naturalWidth)));
    if (genre->isVisible()) {
        const QRectF genreRect = sceneRect(genre);
        QVERIFY(qAbs(genreRect.right() - station.right()) < 20);
        QVERIFY(modeRect.right() < genreRect.left());
    }

    UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
    app.screenshot(QString("playback-cards-%1x%2").arg(width).arg(height));

    if (phoneLayout) {
        app.key(Qt::Key_Escape);
        QTRY_VERIFY(app.isVisible("stationListPane"));
    }
}

void UiPlaybackTests::stopsAndPlays()
{
    QVERIFY(isPlaying());
    const quint32 sid = currentService();

    QVERIFY(app.click("playStopButton"));
    QTRY_VERIFY(!isPlaying());

    QVERIFY(app.click("playStopButton"));
    QTRY_VERIFY_WITH_TIMEOUT(isPlaying() && currentService() == sid, timeoutPlay);

    qInfo() << "The play/stop button stops and plays the last station again";
}

void UiPlaybackTests::keyboardShortcuts()
{
    const QList<quint32> sids = shownStations().keys();
    QVERIFY(sids.size() >= 2);

    // S: stop and play
    app.key(Qt::Key_S);
    QTRY_VERIFY(!isPlaying());
    app.key(Qt::Key_S);
    QTRY_VERIFY_WITH_TIMEOUT(isPlaying(), timeoutPlay);

    // 1: first station of the list, N: next, P: previous
    QObject* view = app.item("stationListView");
    QObject* model = view->property("model").value<QObject*>();
    auto sidAt = [&](int index) {
        QJSValue station;
        QMetaObject::invokeMethod(model, "get", Q_RETURN_ARG(QJSValue, station), Q_ARG(int, index));
        return station.property("stationSId").toUInt();
    };

    app.key(Qt::Key_1);
    QTRY_COMPARE_WITH_TIMEOUT(currentService(), sidAt(0), timeoutPlay);
    app.key(Qt::Key_N);
    QTRY_COMPARE_WITH_TIMEOUT(currentService(), sidAt(1), timeoutPlay);
    app.key(Qt::Key_P);
    QTRY_COMPARE_WITH_TIMEOUT(currentService(), sidAt(0), timeoutPlay);

    qInfo() << "Shortcuts S, 1, N and P work";
}

void UiPlaybackTests::volumeShortcuts()
{
    // The tests play with volume 0. Up and down again, so that it stays 0.
    const qreal before = app.radio()->property("volume").toReal();
    app.key(Qt::Key_Up, Qt::ControlModifier);
    QTRY_VERIFY(app.radio()->property("volume").toReal() > before);
    QVERIFY(app.property("volumeSlider", "value").toReal() > before);
    app.key(Qt::Key_Down, Qt::ControlModifier);
    QTRY_COMPARE(app.radio()->property("volume").toReal(), before);

    qInfo() << "Ctrl+Up and Ctrl+Down change the volume";
}

void UiPlaybackTests::showsGenreOnTheRight()
{
    // The synthetic recording has no programme type, a real one has
    const QString missing = app.useClip("uk_11d_2020");
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));
    QVERIFY(app.restart());
    app.resize(1200, 800);

    // The stations of the other recording are still in the list
    QVERIFY(app.click("stationListMenuButton"));
    QVERIFY(app.waitVisible("clearListItem"));
    UiApp::settle();
    QVERIFY(app.click("clearListItem"));
    QTRY_VERIFY(app.isVisible("scanButton"));
    UiApp::settle();
    QVERIFY(scanStations());

    QQuickItem* genre = nullptr;
    for (const QJsonObject& service : app.services()) {
        QVERIFY(app.click(stationRow(service["sid"].toInt())));
        if (UiApp::waitFor([&] { return app.isVisible("nowPlaying.programmeType")
                                        && !app.text("nowPlaying.audioMode").isEmpty(); }, 8000)) {
            genre = app.item("nowPlaying.programmeType");
            break;
        }
    }
    if (!genre)
        QSKIP("No service of the recording sends a programme type");

    const QRectF station = sceneRect(app.item("stationCard"));
    const QRectF dabRect = sceneRect(app.item("nowPlaying.dabType"));
    const QRectF modeRect = sceneRect(app.item("nowPlaying.audioMode"));
    const QRectF genreRect = sceneRect(genre);
    QVERIFY(qAbs(dabRect.left() - station.left()) < 20);
    QVERIFY(qAbs(genreRect.right() - station.right()) < 20);
    QVERIFY(dabRect.right() < modeRect.left());
    QVERIFY(modeRect.right() < genreRect.left());
    app.screenshot("playback-genre");

    qInfo().noquote() << QString("\"%1\": %2 | %3 | %4").arg(app.text("nowPlaying.station"),
        app.text("nowPlaying.dabType"), app.text("nowPlaying.audioMode"), app.text("nowPlaying.programmeType"));
}

WELLE_GUI_TEST_MAIN(UiPlaybackTests)

#include "tst_ui_playback.moc"
