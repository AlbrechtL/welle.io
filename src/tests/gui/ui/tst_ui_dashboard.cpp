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

// gui.ui.dashboard: the expert views. The grid of welle.io 2.7 is taken
// over, every view can be added and gets data while a station plays,
// cards are maximized, moved and removed, the layout survives a restart
// and leaving the expert mode keeps the views of the normal mode. The
// service details tell whether the ensemble provides station logos (SPI).
// The I/Q recorder records the input into a file that plays again.

#include <QAbstractSeries>
#include <QBuffer>
#include <QImage>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "gui_helper.h"
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
    void showsStationLogo();
    void graphsGetData_data();
    void graphsGetData();
    void showsImpulseResponseDelay();
    void setsGain();
    void recordsWithDuration();
    void recordsUntilStopped();
    void showsRecordingError();
    void playsRecording();
    void maximizesCard();
    void movesCard();
    void removesCard();
    void keepsLayoutAfterRestart();
    void layout_data();
    void layout();
    void leavingExpertModeKeepsNormalViews();

private:
    QTemporaryDir recordDir;
    QString recording; // File of recordsWithDuration()
    quint32 playingSid = 0;

    bool play()
    {
        if (!app.radio()->property("isPlaying").toBool())
            app.radio()->play(app.radio()->property("channel").toString(), "Test", playingSid);
        return UiApp::waitFor([this]() { return app.radio()->property("isPlaying").toBool(); }, 10000);
    }

    // Shows the recorder card, records into recordDir
    bool showRecorder()
    {
        QQuickItem* card = app.item("card.rawRecorder"); // Added by addsEveryView
        if (!card)
            return false;
        app.radio()->setProperty("recordFolder", recordDir.path());
        app.scrollIntoView(card);
        UiApp::settle();
        return true;
    }

    bool selectDuration(int index)
    {
        if (!app.click("recorderCard.duration"))
            return false;
        UiApp::settle();
        const bool clicked = app.click("comboItem." + QString::number(index));
        UiApp::settle();
        return clicked;
    }

    QStringList recordings() const
    {
        return QDir(recordDir.path()).entryList({"*.iq"}, QDir::Files, QDir::Name);
    }

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
        if (service["type"].toString() == "DAB+") {
            playingSid = quint32(service["sid"].toInt());
            QVERIFY(app.click(stationRow(service["sid"].toInt())));
        }
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

void UiDashboardTests::showsStationLogo()
{
    QImage image(128, 128, QImage::Format_ARGB32);
    image.fill(QColor("#e6007e"));
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    QVERIFY(image.save(&buffer, "PNG"));
    emit app.radio()->serviceLogoReceived(app.radio()->property("service").toUInt(), png);

    // The small logo on the service overview, left of the station name and
    // not overlapping the other rows
    QObject* card = app.find("card.serviceOverview");
    QVERIFY(card);
    auto* logo = qobject_cast<QQuickItem*>(child(card, "nowPlaying.logo"));
    auto* station = qobject_cast<QQuickItem*>(child(card, "nowPlaying.station"));
    auto* ensemble = qobject_cast<QQuickItem*>(child(card, "nowPlaying.ensemble"));
    auto* dabType = qobject_cast<QQuickItem*>(child(card, "nowPlaying.dabType"));
    auto* dls = qobject_cast<QQuickItem*>(child(card, "nowPlaying.dls"));
    QVERIFY(logo && station && ensemble && dabType && dls);
    QTRY_VERIFY(logo->isVisible());
    UiApp::settle();
    auto top = [](QQuickItem* item) { return item->mapToScene(QPointF(0, 0)).y(); };
    auto bottom = [](QQuickItem* item) { return item->mapToScene(QPointF(0, item->height())).y(); };
    QVERIFY(bottom(ensemble) <= top(logo));
    QVERIFY(logo->mapToScene(QPointF(logo->width(), 0)).x() <= station->mapToScene(QPointF(0, 0)).x());
    QVERIFY(bottom(logo) <= top(dls) || !dls->isVisible());
    QVERIFY2(!dls->isVisible() || bottom(dls) <= top(dabType),
             qPrintable(QString("DLS until %1, chips from %2").arg(bottom(dls)).arg(top(dabType))));

    app.scrollIntoView(qobject_cast<QQuickItem*>(card));
    app.screenshot("dashboard-logo");

    qInfo() << "The service overview shows the station logo";
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

void UiDashboardTests::setsGain()
{
    // Added by addsEveryView
    QQuickItem* card = app.item("card.gain");
    QVERIFY(card);
    app.scrollIntoView(card);
    UiApp::settle();
    QObject* receiverSettings = app.find("receiverSettings");
    QVERIFY(receiverSettings);

    // Automatic gain is on: the manual gain can't be changed
    QVERIFY(app.radio()->property("agc").toBool());
    QVERIFY(app.property("gainCard.agc", "checked").toBool());
    QVERIFY(!app.item("gainCard.gain")->isEnabled());

    // Switched off in the card: also off in the settings
    QVERIFY(app.click("gainCard.agc"));
    QTRY_VERIFY(!app.radio()->property("agc").toBool());
    QVERIFY(!app.property("setting.enableAGCState", "checked").toBool());

    // A RAW file has no gain to set: the slider stays disabled, the card
    // says why
    QCOMPARE(app.radio()->property("gainCount").toInt(), 0);
    QVERIFY(!app.item("gainCard.gain")->isEnabled());
    QVERIFY(app.isVisible("gainCard.noGain"));
    QCOMPARE(app.property("gainCard.agc", "description").toString(),
             QString("No effect with this input device"));
    UiApp::settle();
    app.screenshot("dashboard-gain");
    app.resize(800, 480);
    app.scrollIntoView(card);
    UiApp::settle();
    app.screenshot("dashboard-gain-800x480");
    app.resize(1400, 800);
    app.scrollIntoView(card);
    UiApp::settle();

    // A gain set while the settings are closed (as by the slider of the
    // card) is saved by the settings
    const int gain = 7;
    app.radio()->setProperty("gain", gain);
    QCOMPARE(receiverSettings->property("manualGainState").toInt(), gain);

    // The button opens the receiver settings
    QVERIFY(app.click("gainCard.openSettings"));
    QTRY_VERIFY(app.isVisible("settingsPage"));
    QTRY_COMPARE(app.find("settingsPage")->property("currentSection").toString(), QString("receiver"));
    app.key(Qt::Key_Escape);
    QTRY_VERIFY(!app.isVisible("settingsPage"));
    QVERIFY(waitForPage());

    // Saved
    QVERIFY(app.restart());
    QVERIFY(!app.radio()->property("agc").toBool());
    QCOMPARE(app.radio()->property("gain").toInt(), gain);
    QVERIFY(!app.property("gainCard.agc", "checked").toBool());

    // Automatic gain again
    app.scrollIntoView(app.item("card.gain"));
    UiApp::settle();
    QVERIFY(app.click("gainCard.agc"));
    QTRY_VERIFY(app.radio()->property("agc").toBool());
    QVERIFY(app.property("setting.enableAGCState", "checked").toBool());
    QVERIFY(!app.item("gainCard.gain")->isEnabled());

    // The slider shows the gain the AGC has set. A RAW file has none: set
    // it like the device would (checked before the gain timer of the radio
    // controller asks the device again)
    QObject* radio = app.radio();
    radio->setProperty("gainCount", 28);
    radio->setProperty("deviceGain", 12);
    radio->setProperty("deviceGainValue", 19.7);
    QCOMPARE(app.property("gainCard.gain", "value").toInt(), 12);
    QCOMPARE(app.text("gainCard.value"), QString("Value: 19.70"));
    app.screenshot("dashboard-gain-agc");

    // Gain unknown: the manual gain
    radio->setProperty("deviceGain", -1);
    QCOMPARE(app.property("gainCard.gain", "value").toInt(), gain);
    QCOMPARE(app.text("gainCard.value"), QString("Value: 0.00"));
    radio->setProperty("gainCount", 0);

    qInfo() << "The gain card switches the automatic gain, the settings save it and the gain, the button opens the receiver settings, the slider shows the gain of the AGC";
}

void UiDashboardTests::recordsWithDuration()
{
    QVERIFY(recordDir.isValid());
    QVERIFY(showRecorder());
    QCOMPARE(app.text("recorderCard.folder"), recordDir.path());

    // The RAW file is recorded in its format, 2.048 MS/s
    const QString format = app.clip()["format"].toString();
    QVERIFY(app.radio()->property("recorderAvailable").toBool());
    QCOMPARE(app.radio()->property("recorderFormat").toString(), format);
    const double bytesPerSecond = app.radio()->property("recorderBytesPerSecond").toDouble();
    QCOMPARE(bytesPerSecond, 2048000.0 * (format.contains("16") ? 4 : format == "cf32" ? 8 : 2));

    // Without samples from the input there is nothing to record
    app.radio()->stop();
    QTRY_VERIFY(!app.radio()->property("isInputRunning").toBool());
    QVERIFY(!app.item("recorderCard.record")->isEnabled());
    QCOMPARE(app.text("recorderCard.format"), QString("Play a station or tune to a channel to record"));
    QVERIFY(play());
    QVERIFY(app.radio()->property("isInputRunning").toBool());
    QVERIFY(app.item("recorderCard.record")->isEnabled());
    QVERIFY(app.text("recorderCard.format").contains(format));

    // Estimated size of 10 s
    QVERIFY(selectDuration(1));
    QCOMPARE(app.find("card.rawRecorder")->property("duration").toInt(), 10);
    QCOMPARE(app.text("recorderCard.estimate"),
             QString("Estimated file size: %1 MB").arg(10 * bytesPerSecond / 1e6, 0, 'f', 0));
    QVERIFY(!app.isVisible("recorderCard.status"));

    // Recording: elapsed time, live size and progress
    QSignalSpy finished(app.radio(), &CRadioController::recordingFinished);
    QVERIFY(app.click("recorderCard.record"));
    QTRY_VERIFY(app.radio()->property("isRecording").toBool());
    QCOMPARE(app.text("recorderCard.record"), QString("Stop"));
    QVERIFY(!app.item("recorderCard.duration")->isEnabled());
    QVERIFY(!app.item("recorderCard.changeFolder")->isEnabled());
    QTRY_VERIFY_WITH_TIMEOUT(app.radio()->property("recordedBytes").toLongLong() > 1000000, 5000);
    QVERIFY(app.isVisible("recorderCard.status"));
    QVERIFY(app.isVisible("recorderCard.progress"));
    QVERIFY2(app.text("recorderCard.status").contains(" / 0:10"), qPrintable(app.text("recorderCard.status")));
    QVERIFY(app.text("recorderCard.status").contains(" MB"));
    app.screenshot("recorder-recording");
    app.resize(800, 480);
    app.scrollIntoView(app.item("card.rawRecorder"));
    UiApp::settle();
    app.screenshot("recorder-800x480");
    app.resize(360, 640);
    QVERIFY(app.click("miniPlayer"));
    QTRY_VERIFY(app.isVisible("dashboard"));
    UiApp::settle();
    app.scrollIntoView(app.item("card.rawRecorder"));
    UiApp::settle();
    UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
    UI_VERIFY_EMPTY(app.controlsWithoutAccessibleName(), "Controls without accessible name");
    app.screenshot("recorder-360x640");
    app.key(Qt::Key_Escape);
    UiApp::settle();
    app.resize(1400, 800);
    app.scrollIntoView(app.item("card.rawRecorder"));
    UiApp::settle();

    // Stops by itself after 10 s
    QTRY_VERIFY_WITH_TIMEOUT(!app.radio()->property("isRecording").toBool(), 20000);
    QCOMPARE(finished.count(), 1);
    QCOMPARE(finished[0][1].toString(), QString()); // No reason, as wanted
    QVERIFY(!finished[0][2].toBool());
    QCOMPARE(app.text("recorderCard.record"), QString("Record"));
    QCOMPARE(app.radio()->property("recordDroppedBytes").toLongLong(), 0);

    // welle-io_<date>.<format>.iq (a RAW file has no channel), exactly 10 s
    QCOMPARE(recordings().size(), 1);
    const QString name = recordings()[0];
    QVERIFY2(QRegularExpression("^welle-io_\\d{8}-\\d{6}\\." + format + "\\.iq$").match(name).hasMatch(), qPrintable(name));
    recording = recordDir.filePath(name);
    QCOMPARE(finished[0][0].toString(), recording);
    QCOMPARE(QFileInfo(recording).size(), qint64(10 * bytesPerSecond));
    QCOMPARE(app.radio()->property("recordedBytes").toLongLong(), qint64(10 * bytesPerSecond));
    QVERIFY(app.isVisible("recorderCard.lastFile"));
    QCOMPARE(app.text("recorderCard.lastFile"), "Saved: " + name);
    QTRY_VERIFY(app.text("snackbar.text").contains(name));
    app.screenshot("recorder-saved");

    qInfo().noquote() << "Recorded" << name << "with" << QFileInfo(recording).size() << "bytes, stopped after 10 s";
}

void UiDashboardTests::recordsUntilStopped()
{
    QVERIFY(showRecorder());
    QVERIFY(selectDuration(0));
    QCOMPARE(app.find("card.rawRecorder")->property("duration").toInt(), 0);
    QVERIFY(app.text("recorderCard.estimate").endsWith(" per minute"));

    QVERIFY(app.click("recorderCard.record"));
    QTRY_VERIFY(app.radio()->property("isRecording").toBool());
    QVERIFY(!app.isVisible("recorderCard.progress"));
    QTRY_VERIFY_WITH_TIMEOUT(app.radio()->property("recordedBytes").toLongLong() > 4000000, 5000);
    QVERIFY(!app.text("recorderCard.status").contains("/"));
    QVERIFY(app.click("recorderCard.record"));
    QTRY_VERIFY(!app.radio()->property("isRecording").toBool());

    // A second file, as long as recorded
    QCOMPARE(recordings().size(), 2);
    const QString file = app.radio()->property("recordFile").toString();
    QVERIFY(file != recording);
    QCOMPARE(QFileInfo(file).size(), app.radio()->property("recordedBytes").toLongLong());
    QVERIFY(QFileInfo(file).size() > 4000000);

    qInfo().noquote() << "Recorded until stopped:" << QFileInfo(file).fileName() << QFileInfo(file).size() << "bytes";
}

void UiDashboardTests::showsRecordingError()
{
    QVERIFY(showRecorder());

    // A folder that can't be created
    QFile blocker(recordDir.filePath("file"));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    app.radio()->setProperty("recordFolder", recordDir.filePath("file/folder"));
    QCOMPARE(app.text("recorderCard.folder"), recordDir.filePath("file/folder"));

    // The "Saved" message of the last recording goes first
    while (app.find("snackbar")->property("visible").toBool()) {
        app.click("snackbar.close");
        UiApp::settle();
    }
    QVERIFY(app.click("recorderCard.record"));
    QTRY_VERIFY(app.text("snackbar.text").contains("Can't create the folder"));
    QCOMPARE(app.find("snackbar")->property("kind").toString(), QString("error"));
    QVERIFY(!app.radio()->property("isRecording").toBool());
    QCOMPARE(app.text("recorderCard.record"), QString("Record"));
    app.screenshot("recorder-error");
    QVERIFY(app.click("snackbar.close"));

    // A folder from the FolderDialog is a URL
    app.radio()->setProperty("recordFolder", QUrl::fromLocalFile(recordDir.path()).toString());
    QCOMPARE(app.radio()->property("recordFolder").toString(), recordDir.path());

    qInfo().noquote() << "Error shown:" << "Can't create the folder";
}

void UiDashboardTests::playsRecording()
{
    // The RAW file input recognises the format from the name and decodes
    // the recording like the original clip
    QVERIFY(!recording.isEmpty());
    const QString ensemble = app.clip()["expect"].toObject()["ensemble_label"].toString();
    const quint32 sid = playingSid;

    app.gui()->openRawFile(recording, "auto");
    QVERIFY(app.radio()->property("deviceName").toString().contains(QFileInfo(recording).fileName()));
    app.radio()->play(app.radio()->property("channel").toString(), "Recording", sid);
    QTRY_COMPARE_WITH_TIMEOUT(app.radio()->property("ensemble").toString().trimmed(), ensemble, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(app.radio()->property("isPlaying").toBool(), 10000);
    QTRY_VERIFY_WITH_TIMEOUT(app.radio()->property("bitRate").toInt() > 0, 10000);

    // Back to the clip
    app.gui()->openRawFile(app.clipFile(), "auto");
    app.radio()->play(app.radio()->property("channel").toString(), "Clip", sid);
    QTRY_VERIFY_WITH_TIMEOUT(app.radio()->property("isPlaying").toBool(), 10000);

    qInfo().noquote() << "The recording plays:" << ensemble;
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
