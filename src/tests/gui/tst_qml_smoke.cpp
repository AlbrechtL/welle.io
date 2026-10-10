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

// Starts the complete QML user interface like main.cpp does, with the
// settings pointing to the synthetic ODR recording, and checks that the
// stations show up without any QML warnings.

#include <memory>

#include <QIcon>
#include <QJSValue>
#include <QMutex>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSettings>
#include <QTranslator>

#include "expectations.h"
#include "gui_helper.h"
#include "gui_test_main.h"
#include "radio_controller.h"
#include "waterfallitem.h"

static const QString clipId = "odr_dabmod";

// Collects QML warnings and errors from all threads
static QMutex qmlMessagesMutex;
static QStringList qmlMessages;
static QtMessageHandler previousHandler = nullptr;

static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    const bool fromQml = (context.file && QString(context.file).startsWith("qrc:/")) || msg.startsWith("qrc:/");
    if (type != QtDebugMsg && type != QtInfoMsg && fromQml) {
        QMutexLocker lock(&qmlMessagesMutex);
        qmlMessages << msg;
    }
    previousHandler(type, context, msg);
}

class QmlSmokeTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void loadsMainView();
    void opensRawFileFromSettings();
    void showsStations();
    void noQmlWarnings();

private:
    QVariantMap commandLineOptions;
    std::unique_ptr<CRadioController> radioController;
    std::unique_ptr<CGUIHelper> guiHelper;
    std::unique_ptr<QQmlApplicationEngine> engine;
    QTranslator* translator = nullptr;
    QString clipFile;

    QObject* root() const { return engine->rootObjects().value(0); }
};

void QmlSmokeTests::initTestCase()
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

    // What the user would have chosen in the settings: RAW file input
    QSettings settings;
    settings.setValue("device", 5);
    settings.setValue("enableAutoSdr", false);
    settings.setValue("rawFile", clipFile);
    settings.setValue("rawFileFormat", 0);  // auto
    settings.sync();

    previousHandler = qInstallMessageHandler(messageHandler);

    // Same setup as main.cpp
    translator = new QTranslator;
    CGUIHelper::loadTranslationFile(translator, CGUIHelper::mapToLanguage("auto"));
    QCoreApplication::installTranslator(translator);

    qmlRegisterType<WaterfallItem>("io.welle", 1, 0, "Waterfall");
    qRegisterMetaType<mot_file_t>("mot_file_t");

    QStringList themePaths = QIcon::themeSearchPaths();
    themePaths << ":/icons/welle_io_icons";
    QIcon::setThemeSearchPaths(themePaths);
    QIcon::setThemeName("welle_io_icons");

    radioController = std::make_unique<CRadioController>(commandLineOptions);
    radioController->setVolume(0);
    QQuickStyle::setStyle("Universal");

    guiHelper = std::make_unique<CGUIHelper>(radioController.get());
    guiHelper->setTranslator(translator);

    engine = std::make_unique<QQmlApplicationEngine>();
    engine->rootContext()->setContextProperty("guiHelper", guiHelper.get());
    engine->rootContext()->setContextProperty("radioController", radioController.get());
    engine->addImageProvider(QLatin1String("SLS"), guiHelper->motImageProvider);
}

void QmlSmokeTests::cleanupTestCase()
{
    engine.reset();
    guiHelper.reset();
    radioController.reset();
    if (previousHandler)
        qInstallMessageHandler(previousHandler);
}

void QmlSmokeTests::loadsMainView()
{
    engine->load(QUrl("qrc:/QML/MainView.qml"));
    QVERIFY(root());
    QCOMPARE(root()->property("title").toString(), QString("welle.io"));

    qInfo() << "MainView.qml loaded";
}

void QmlSmokeTests::opensRawFileFromSettings()
{
    QTRY_COMPARE_WITH_TIMEOUT(radioController->property("deviceId").value<CDeviceID>(), CDeviceID::RAWFILE, 5000);
    QVERIFY(radioController->property("deviceName").toString().contains(QFileInfo(clipFile).fileName()));

    qInfo().noquote() << "The settings pages opened the input device:"
                      << radioController->property("deviceName").toString();
}

void QmlSmokeTests::showsStations()
{
    QObject* stationList = root()->findChild<QObject*>("stationList");
    QVERIFY(stationList);

    // Same as the "Start station scan" menu entry
    QMetaObject::invokeMethod(radioController.get(), "startScan");

    QTRY_COMPARE_WITH_TIMEOUT(stationList->property("count").toInt(), 2, 20000);

    QStringList names;
    for (int i = 0; i < 2; i++) {
        QJSValue station;
        QVERIFY(QMetaObject::invokeMethod(stationList, "get", Q_RETURN_ARG(QJSValue, station), Q_ARG(int, i)));
        names << station.property("stationName").toString().trimmed();
    }
    names.sort();
    QCOMPARE(names, QStringList({"DAB Test 1", "DAB+ Test 2"}));

    qInfo().noquote() << "Station list in QML:" << names.join(", ");
}

void QmlSmokeTests::noQmlWarnings()
{
    QMutexLocker lock(&qmlMessagesMutex);
    QVERIFY2(qmlMessages.isEmpty(), qPrintable(qmlMessages.join("\n")));

    qInfo() << "No QML warnings or errors";
}

WELLE_GUI_TEST_MAIN(QmlSmokeTests)

#include "tst_qml_smoke.moc"
