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

// Drives the C++ part of the GUI (CRadioController, CGUIHelper) the way the
// QML code does, with the synthetic ODR recording played in real time.

#include <memory>

#include <QSignalSpy>

#include "expectations.h"
#include "gui_helper.h"
#include "gui_test_main.h"
#include "radio_controller.h"

static const QString clipId = "odr_dabmod";
static const quint32 sidDab = 0x8daa;
static const quint32 sidDabPlus = 0x8dab;
static const QString dls = "You are listening to Ghost Dance from Kevin MacLeod";

// The recording plays in real time, these timeouts leave enough margin
static const int timeoutStations = 20000;
static const int timeoutAudio = 15000;
static const int timeoutSlide = 40000;

class RadioControllerTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void opensRawFile();
    void findsStations();
    void playsDabPlus();
    void switchDropsOldPad();
    void playsDab();
    void switchesEnsemble();
    void stops();

private:
    QVariantMap commandLineOptions;
    std::unique_ptr<CRadioController> radioController;
    std::unique_ptr<CGUIHelper> guiHelper;
    QString clipFile;

    QString text(const char* property) const { return radioController->property(property).toString().trimmed(); }
};

void RadioControllerTests::initTestCase()
{
    hideDebugOutput();

    const QString missing = missingTestData();
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));

    QJsonObject clip;
    for (const auto& value : loadManifest()["clips"].toArray()) {
        if (value.toObject()["id"].toString() == clipId)
            clip = value.toObject();
    }
    if (clip.isEmpty())
        QFAIL(qPrintable("Clip " + clipId + " is not in the manifest"));
    if (!missingClip(clip).isEmpty())
        QSKIP(qPrintable(missingClip(clip)));
    clipFile = clipPath(clip);

    qRegisterMetaType<mot_file_t>("mot_file_t");

    radioController = std::make_unique<CRadioController>(commandLineOptions);
    guiHelper = std::make_unique<CGUIHelper>(radioController.get());
    radioController->setVolume(0);
}

void RadioControllerTests::cleanupTestCase()
{
    guiHelper.reset();
    radioController.reset();
}

void RadioControllerTests::opensRawFile()
{
    QSignalSpy deviceReady(radioController.get(), &CRadioController::deviceReady);

    guiHelper->openRawFile(clipFile, "auto");

    QCOMPARE(deviceReady.count(), 1);
    QCOMPARE(radioController->property("deviceId").value<CDeviceID>(), CDeviceID::RAWFILE);
    QVERIFY2(text("deviceName").contains(QFileInfo(clipFile).fileName()), qPrintable(text("deviceName")));

    qInfo().noquote() << "Input device:" << text("deviceName");
}

void RadioControllerTests::findsStations()
{
    QSignalSpy stations(radioController.get(), &CRadioController::newStationNameReceived);

    // The GUI scans a RAW file like a single channel called "File"
    radioController->startScan();

    QTRY_VERIFY_WITH_TIMEOUT(stations.count() >= 2, timeoutStations);

    QMap<quint32, QString> found;
    for (const auto& args : stations) {
        found[args.at(1).toUInt()] = args.at(0).toString().trimmed();
        QCOMPARE(args.at(2).toString(), QString("File"));
    }
    QCOMPARE(found.value(sidDab), QString("DAB Test 1"));
    QCOMPARE(found.value(sidDabPlus), QString("DAB+ Test 2"));

    QTRY_COMPARE_WITH_TIMEOUT(text("ensemble"), QString("OpenDigitalRadio"), timeoutStations);
    QCOMPARE(radioController->property("ensembleId").toInt(), 0x4fff);
    QVERIFY(radioController->property("isSync").toBool());

    qInfo().noquote() << QString("Ensemble 0x%1 \"%2\", stations 0x%3 \"%4\" and 0x%5 \"%6\"")
        .arg(radioController->property("ensembleId").toInt(), 4, 16, QChar('0')).arg(text("ensemble"))
        .arg(sidDab, 4, 16, QChar('0')).arg(found.value(sidDab))
        .arg(sidDabPlus, 4, 16, QChar('0')).arg(found.value(sidDabPlus));
}

void RadioControllerTests::playsDabPlus()
{
    QSignalSpy slides(radioController.get(), &CRadioController::motChanged);

    radioController->play("File", "DAB+ Test 2", sidDabPlus);

    QTRY_VERIFY_WITH_TIMEOUT(radioController->property("isPlaying").toBool(), timeoutAudio);
    QTRY_COMPARE_WITH_TIMEOUT(text("audioMode"), QString("AAC-LC, 48 kHz Stereo @ 96 kbit/s"), timeoutAudio);
    QCOMPARE(radioController->property("isDAB").toBool(), false);
    QCOMPARE(radioController->property("bitRate").toInt(), 96);
    QCOMPARE(text("title"), QString("DAB+ Test 2"));

    QTRY_COMPARE_WITH_TIMEOUT(text("text"), dls, timeoutAudio);

    QTRY_VERIFY_WITH_TIMEOUT(slides.count() >= 1, timeoutSlide);
    const auto slide = slides.first().first().value<mot_file_t>();
    QVERIFY(slide.data.size() > 2);
    QCOMPARE(slide.data[0], uint8_t(0xFF));  // JPEG
    QCOMPARE(slide.data[1], uint8_t(0xD8));

    QCOMPARE(radioController->property("rsUncorrectedErrors").toInt(), 0);
    QCOMPARE(radioController->property("frameErrors").toInt(), 0);

    qInfo().noquote() << QString("Playing \"%1\": %2, DLS \"%3\", slide %4 (JPEG, %5 bytes)")
        .arg(text("title"), text("audioMode"), text("text"),
             QString::fromStdString(slide.content_name)).arg(slide.data.size());
}

void RadioControllerTests::switchDropsOldPad()
{
    // Issue #912: after a switch within the ensemble, nothing of the previous
    // service may come back, e.g. its slides. DAB Test 1 has no slides.
    QSignalSpy slides(radioController.get(), &CRadioController::motChanged);
    QSignalSpy audioModes(radioController.get(), &CRadioController::audioModeChanged);

    // A slide of DAB+ Test 2 that the decoder delivered just before the switch
    mot_file_t slide;
    slide.content_name = "previous.jpg";
    slide.data = {0xFF, 0xD8};
    radioController->onMOT(slide);

    radioController->play("File", "DAB Test 1", sidDab);
    QTest::qWait(3000);

    QCOMPARE(slides.count(), 0);

    // The decoder of DAB+ Test 2 has stopped with the switch
    QStringList modes;
    for (const auto& args : audioModes)
        modes << args.at(0).toString();
    QVERIFY2(!modes.join("").contains("AAC"), qPrintable(modes.join(" | ")));
}

void RadioControllerTests::playsDab()
{
    radioController->play("File", "DAB Test 1", sidDab);

    QTRY_COMPARE_WITH_TIMEOUT(text("audioMode").replace("Joint Stereo", "Stereo"),
                              QString("MPEG 1.0 Layer II, 48 kHz Stereo @ 128 kbit/s"), timeoutAudio);
    QCOMPARE(radioController->property("isDAB").toBool(), true);
    QCOMPARE(radioController->property("bitRate").toInt(), 128);
    QCOMPARE(radioController->property("service").toUInt(), sidDab);
    QCOMPARE(text("title"), QString("DAB Test 1"));
    QCOMPARE(radioController->property("frameErrors").toInt(), 0);

    qInfo().noquote() << QString("Playing \"%1\": %2").arg(text("title"), text("audioMode"));
}

void RadioControllerTests::switchesEnsemble()
{
    // The station list's ensemble menu plays the same service on another
    // channel. A RAW file plays whatever channel is asked for.
    QSignalSpy audioModes(radioController.get(), &CRadioController::audioModeChanged);

    radioController->play("5C", "DAB Test 1", sidDab);

    // Reset, then decoded again by the new receiver
    QTRY_VERIFY_WITH_TIMEOUT(audioModes.count() >= 2
                             && audioModes.last().at(0).toString().startsWith("MPEG"), timeoutAudio);
    QVERIFY(radioController->property("isPlaying").toBool());
    QCOMPARE(radioController->property("service").toUInt(), sidDab);

    qInfo().noquote() << QString("Playing \"%1\" again after the ensemble switch: %2").arg(text("title"), text("audioMode"));
}

void RadioControllerTests::stops()
{
    radioController->stop();

    QCOMPARE(text("text"), QString("Stopped"));
    QCOMPARE(radioController->property("bitRate").toInt(), 0);
}

WELLE_GUI_TEST_MAIN(RadioControllerTests)

#include "tst_radiocontroller.moc"
