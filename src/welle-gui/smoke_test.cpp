/*
 *    Smoke test of an installed welle-io (command line option --smoke-test):
 *    plays every service of a RAW file through the complete user interface,
 *    writes what it found into a JSON report and quits with an exit code.
 *    Used by the package workflows to check the packages, see
 *    src/tests/README.md.
 *
 *    This file was generated with the help of an AI assistant (Claude Code).
 *
 *    This file is part of the welle.io.
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

#include <QCoreApplication>
#include <QDirIterator>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJSValue>
#include <QJsonDocument>
#include <QMutex>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSettings>
#include <QSysInfo>
#include <QTemporaryDir>
#include <memory>

#include "smoke_test.h"
#include "radio_controller.h"
#include "version.h"

// Time without a new service before the scan counts as complete
static const int scanSettleMs = 3000;
// Audio has to be decoded for this long before a service counts as played
static const int audioSettleMs = 2000;
// How long to wait for a DLS of a service that plays
static const int dlsWaitMs = 10000;

const QStringList CSmokeTest::rawFileFormats = { "auto", "u8", "s8", "s16le", "s16be", "cf32" };

// Warnings of the whole application, from all threads
static QMutex messagesMutex;
static QStringList qmlWarnings;
static QStringList otherWarnings;
static QtMessageHandler previousHandler = nullptr;

static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (type != QtDebugMsg && type != QtInfoMsg) {
        const bool fromQml = (context.file && QString(context.file).startsWith("qrc:/")) || msg.startsWith("qrc:/");
        QMutexLocker lock(&messagesMutex);
        if (fromQml)
            qmlWarnings << msg;
        else if (otherWarnings.size() < 50)
            otherWarnings << msg;
    }
    if (previousHandler)
        previousHandler(type, context, msg);
}

bool CSmokeTest::selectRawFile(const QString& fileName, const QString& format)
{
    const int formatIndex = rawFileFormats.indexOf(format);
    if (formatIndex < 0)
        return false;

    // The keys of ReceiverSettings.qml and RawFileSettings.qml
    QSettings settings;
    settings.setValue("device", 5);
    settings.setValue("enableAutoSdr", false);
    settings.setValue("rawFile", QFileInfo(fileName).absoluteFilePath());
    settings.setValue("rawFileFormat", formatIndex);
    settings.sync();
    return true;
}

bool CSmokeTest::isolateSettings()
{
    static std::unique_ptr<QTemporaryDir> settingsDir;
    settingsDir = std::make_unique<QTemporaryDir>();
    if (!settingsDir->isValid())
        return false;

    // The QML Settings use the default format as well
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir->path() + "/user");
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settingsDir->path() + "/system");

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN) && !defined(Q_OS_ANDROID)
    // Station logos and the QML cache. Read on first use, so this is early
    // enough. Elsewhere they stay in the user's folders; the smoke test
    // recording has no logos.
    qputenv("XDG_DATA_HOME", QFile::encodeName(settingsDir->path() + "/data"));
    qputenv("XDG_CACHE_HOME", QFile::encodeName(settingsDir->path() + "/cache"));
#endif
    return true;
}

CSmokeTest::CSmokeTest(const QString& reportFile, int timeoutSeconds,
                       CRadioController* radioController, QQmlApplicationEngine* engine,
                       QObject* parent)
    : QObject(parent)
    , reportFile(reportFile)
    , timeoutSeconds(timeoutSeconds > 0 ? timeoutSeconds : 120)
    , radioController(radioController)
    , engine(engine)
{
    previousHandler = qInstallMessageHandler(messageHandler);

    connect(radioController, &CRadioController::newStationNameReceived, this,
            [this](QString station, quint32 sId, QString channel) {
        if (!services.contains(sId))
            serviceOrder << sId;
        Service& service = services[sId];
        service.sId = sId;
        service.label = station.trimmed();
        service.channel = channel;
        sinceLastStation.restart();
    });

    connect(radioController, &CRadioController::motChanged, this, [this]() {
        if (step == Step::Play)
            slides++;
    });

    connect(radioController, &CRadioController::showErrorMessage, this, [this](QString text) {
        errorMessages << text;
    });

    connect(&pollTimer, &QTimer::timeout, this, &CSmokeTest::tick);
    connect(&timeoutTimer, &QTimer::timeout, this, [this]() {
        static const char* steps[] = { "opening the RAW file", "scanning", "playing", "done" };
        finish(QString("Timeout after %1 s while %2").arg(this->timeoutSeconds).arg(steps[int(step)]));
    });
}

CSmokeTest::~CSmokeTest()
{
    qInstallMessageHandler(previousHandler);
}

void CSmokeTest::start()
{
    total.start();
    qInfo() << "SmokeTest: Started, report:" << reportFile;

    if (engine->rootObjects().isEmpty()) {
        // QCoreApplication::exit() only works once the event loop runs
        QTimer::singleShot(0, this, [this]() { finish("MainView.qml could not be loaded"); });
        return;
    }

    radioController->setVolume(0);
    compileAllQml();

    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(timeoutSeconds * 1000);
    pollTimer.start(200);
}

void CSmokeTest::compileAllQml()
{
    // Parts of the user interface are only loaded when they are opened, e.g.
    // the expert views with QtCharts. Compiling every QML file finds the
    // QML modules that are missing in a package, without creating anything.
    int count = 0;
    QDirIterator it(":/QML", { "*.qml" }, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString file = it.next();
        QQmlComponent component(engine, QUrl("qrc" + file), QQmlComponent::PreferSynchronous);
        if (component.isError()) {
            for (const auto& error : component.errors())
                qmlCompileErrors << error.toString();
        }
        count++;
    }
    qInfo() << "SmokeTest: Compiled" << count << "QML files," << qmlCompileErrors.size() << "errors";
}

void CSmokeTest::tick()
{
    switch (step) {
    case Step::WaitForDevice:
        // RawFileSettings.qml opens the file from the settings
        if (radioController->property("deviceId").value<CDeviceID>() == CDeviceID::RAWFILE) {
            qInfo().noquote() << "SmokeTest: Input device" << text("deviceName");
            step = Step::Scan;
            radioController->startScan();
        }
        break;

    case Step::Scan:
        if (!services.isEmpty() && sinceLastStation.elapsed() > scanSettleMs && !text("ensemble").isEmpty()) {
            qInfo().noquote() << "SmokeTest: Ensemble" << text("ensemble") << "with" << services.size() << "services";
            step = Step::Play;
            playNext();
        }
        break;

    case Step::Play: {
        Service& service = services[serviceOrder[playing]];
        if (radioController->property("service").toUInt() != service.sId)
            break;

        const QString mode = text("audioMode");
        if (mode.isEmpty()) {
            sincePlay.restart();  // audio has to be decoded without a gap
            break;
        }
        if (sincePlay.elapsed() < audioSettleMs)
            break;

        service.mode = mode;
        service.isDab = radioController->property("isDAB").toBool();
        const QString dls = text("text");
        if (!dls.isEmpty())
            service.dls = dls;

        if (!service.dls.isEmpty() || sincePlay.elapsed() > audioSettleMs + dlsWaitMs) {
            service.played = true;
            qInfo().noquote() << "SmokeTest: Played" << service.label << "-" << service.mode
                              << "- DLS:" << service.dls;
            playNext();
        }
        break;
    }

    case Step::Done:
        break;
    }
}

void CSmokeTest::playNext()
{
    playing++;
    if (playing >= serviceOrder.size()) {
        finish(QString());
        return;
    }

    const Service& service = services[serviceOrder[playing]];
    sincePlay.restart();
    radioController->play(service.channel, service.label, service.sId);
}

void CSmokeTest::finish(const QString& error)
{
    if (step == Step::Done)
        return;
    step = Step::Done;
    pollTimer.stop();
    timeoutTimer.stop();

    const QJsonObject result = report(error);
    radioController->stop();

    int exitCode = result["ok"].toBool() ? 0 : 1;
    if (engine->rootObjects().isEmpty())
        exitCode = 2;

    QFile file(reportFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCritical().noquote() << "SmokeTest: Cannot write the report" << reportFile << ":" << file.errorString();
        exitCode = 3;
    }
    else {
        file.write(QJsonDocument(result).toJson());
        file.close();
    }

    if (exitCode == 0)
        qInfo() << "SmokeTest: Passed";
    else
        qCritical().noquote() << "SmokeTest: Failed:" << result["error"].toString();

    // Leave the event loop, main() returns the exit code
    QCoreApplication::exit(exitCode);
}

QJsonObject CSmokeTest::report(const QString& error) const
{
    QJsonObject result;
    result["version"] = QString(CURRENT_VERSION) + " Git: " + GITHASH;
    result["qt_version"] = QString(qVersion());
    result["os"] = QSysInfo::prettyProductName();
    result["cpu"] = QSysInfo::currentCpuArchitecture();
    result["platform"] = QGuiApplication::platformName();
    result["graphics_api"] = graphicsApi();
    result["device_name"] = text("deviceName");
    result["duration_s"] = total.elapsed() / 1000.0;

    QJsonObject ensemble;
    ensemble["id"] = QString("0x%1").arg(radioController->property("ensembleId").toInt(), 4, 16, QChar('0'));
    ensemble["label"] = text("ensemble");
    result["ensemble"] = ensemble;

    bool allPlayed = !serviceOrder.isEmpty();
    QJsonArray serviceList;
    for (quint32 sId : serviceOrder) {
        const Service& service = services[sId];
        QJsonObject entry;
        entry["sid"] = QString("0x%1").arg(sId, 4, 16, QChar('0'));
        entry["label"] = service.label;
        entry["channel"] = service.channel;
        entry["played"] = service.played;
        entry["type"] = service.mode.isEmpty() ? QString() : service.isDab ? QString("DAB") : QString("DAB+");
        entry["mode"] = service.mode;
        entry["dls"] = service.dls;
        serviceList << entry;
        allPlayed = allPlayed && service.played;
    }
    result["services"] = serviceList;
    result["gui_stations"] = guiStations();
    result["slides"] = slides;

    QMutexLocker lock(&messagesMutex);
    result["qml_warnings"] = QJsonArray::fromStringList(qmlWarnings);
    result["qml_compile_errors"] = QJsonArray::fromStringList(qmlCompileErrors);
    result["warnings"] = QJsonArray::fromStringList(otherWarnings);
    result["error_messages"] = QJsonArray::fromStringList(errorMessages);

    QString failure = error;
    if (failure.isEmpty() && !allPlayed)
        failure = "Not every service was played";
    if (failure.isEmpty() && !qmlCompileErrors.isEmpty())
        failure = QString("%1 QML files could not be compiled").arg(qmlCompileErrors.size());
    if (failure.isEmpty() && !qmlWarnings.isEmpty())
        failure = QString("%1 QML warnings").arg(qmlWarnings.size());
    result["error"] = failure;
    result["ok"] = failure.isEmpty();
    return result;
}

QJsonArray CSmokeTest::guiStations() const
{
    // What the station list of the user interface shows
    QJsonArray names;
    if (engine->rootObjects().isEmpty())
        return names;
    QObject* stationList = engine->rootObjects().first()->findChild<QObject*>("stationList");
    if (!stationList)
        return names;

    const int count = stationList->property("count").toInt();
    for (int i = 0; i < count; i++) {
        QJSValue station;
        if (QMetaObject::invokeMethod(stationList, "get", Q_RETURN_ARG(QJSValue, station), Q_ARG(int, i)))
            names << station.property("stationName").toString().trimmed();
    }
    return names;
}

QString CSmokeTest::graphicsApi() const
{
    if (engine->rootObjects().isEmpty())
        return QString();
    auto* window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
    if (!window || !window->rendererInterface())
        return QString();

    switch (window->rendererInterface()->graphicsApi()) {
    case QSGRendererInterface::Software: return "software";
    case QSGRendererInterface::OpenGL: return "opengl";
    case QSGRendererInterface::Direct3D11: return "direct3d11";
    case QSGRendererInterface::Direct3D12: return "direct3d12";
    case QSGRendererInterface::Vulkan: return "vulkan";
    case QSGRendererInterface::Metal: return "metal";
    case QSGRendererInterface::Null: return "null";
    default: return "unknown";
    }
}

QString CSmokeTest::text(const char* property) const
{
    return radioController->property(property).toString().trimmed();
}
