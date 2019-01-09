/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
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

#include <unistd.h>

#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "version.h"
#include "dab-constants.h"
#include "radio_controller.h"
#include "gui_helper.h"
#include "debug_output.h"

#ifdef Q_OS_ANDROID
#include <QtAndroid>
//#include <QAndroidService>
//#include <QAndroidJniObject>
//#include "android_jni.h"
//#include "rep_radio_controller_replica.h"
#endif

int main(int argc, char** argv)
{
    QString Version = QString(CURRENT_VERSION) + " Git: " + GITHASH;

    QCoreApplication::setOrganizationName("welle.io");
    QCoreApplication::setOrganizationDomain("welle.io");
    QCoreApplication::setApplicationName("welle.io");
    QCoreApplication::setApplicationVersion(Version);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
//    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);

    // Handle debug output
    CDebugOutput::init();

//#ifdef Q_OS_ANDROID

//    //  Process Android Service
//    if (argc == 2 && qstrcmp(argv[1], "-S") == 0) {
//        qDebug() << "main:" <<  "Run as service, pid:" << QCoreApplication::applicationPid();

//        // Create new QT core application
//        QAndroidService app(argc, argv);

//        // Default values
//        DABParams dabparams(1);
//        QVariantMap commandLineOptions;

//        // Create a new radio interface instance
//        CRadioController* radioController = new CRadioController(commandLineOptions, dabparams);

//        // Enable remoting source
//        QRemoteObjectHost srcNode(QUrl(QStringLiteral("local:replica")));
//        srcNode.enableRemoting(radioController);

//        CAndroidJNI::getInstance().setRadioController(radioController);

//        // Run application
//        app.exec();

//        // Delete the RadioController controller to ensure a save shutdown
//        delete radioController;

//        qDebug() << "main:" <<  "Service closed";

//        return 0;
//    } else {
//        qDebug() << "main:" <<  "Run as application, pid:" << QCoreApplication::applicationPid();
//    }

//#endif

    // Before printing anything, we set
    setlocale(LC_ALL, "");

    // Create new QT application
    QApplication app(argc, argv);

    // Set icon path
    QStringList themePaths;
    themePaths << ":/icon";
    QIcon::setThemeSearchPaths(themePaths);
    QIcon::setThemeName("welle_io_icons");

    // Set icon
    app.setWindowIcon(QIcon(":/icon/icon.png"));

    // Init translations
    QString locale = QLocale::system().name();
    QTranslator *Translator = CGUIHelper::addTranslator(locale);

    // Default values
    DABParams dabparams(1);

    // Handle the command line
    QCommandLineParser optionParser;
    optionParser.setApplicationDescription("welle.io Help");
    optionParser.addHelpOption();
    optionParser.addVersionOption();

    QCommandLineOption dumpFileName("dump-file",
        QCoreApplication::translate("main", "Records DAB frames (*.mp2) or DAB+ superframes with RS coding (*.dab). This file can be used to analyse X-PAD data with XPADxpert"),
        QCoreApplication::translate("main", "File name"));
    optionParser.addOption(dumpFileName);

    QCommandLineOption LogFileName("log-file",
        QCoreApplication::translate("main", "Redirects all log output texts to a file."),
        QCoreApplication::translate("main", "File name"));
    optionParser.addOption(LogFileName);

    QCommandLineOption Language("language",
        QCoreApplication::translate("main", "Sets the GUI language according to the ISO country codes (e.g. de_DE)"),
        QCoreApplication::translate("main", "Language"));
    optionParser.addOption(Language);

    //	Process the actual command line arguments given by the user
    optionParser.process(app);

    // First of all process the log file
    QString LogFileNameValue = optionParser.value(LogFileName);
    if (LogFileNameValue != "")
    {
        CDebugOutput::setFileName(LogFileNameValue);
        qDebug() << "main: Version:" << Version;
    }

    //	Process language option
    QString languageValue = optionParser.value(Language);
    if (languageValue != "")
        CGUIHelper::addTranslator(languageValue, Translator);

//#ifdef Q_OS_ANDROID

//    // Start background service
//    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabDelegate",
//                                              "startDab",
//                                              "(Landroid/content/Context;)V",
//                                              QtAndroid::androidActivity().object());

//    // Create a radio interface replica and connect to source
//    QRemoteObjectNode repNode;
//    repNode.connectToNode(QUrl(QStringLiteral("local:replica")));
//    CRadioControllerReplica* radioController = repNode.acquire<CRadioControllerReplica>();
//    bool res = radioController->waitForSource();
//    Q_ASSERT(res);

//#else

    QVariantMap commandLineOptions;
    commandLineOptions["dumpFileName"] = optionParser.value(dumpFileName);

    // Create a new radio interface instance
    CRadioController* radioController = new CRadioController(commandLineOptions, dabparams);
//#endif

    QSettings settings;

    // Should we play the last station we have listened to previously?
    if( settings.value("enableLastPlayedStationState", false).toBool() ) {

        QStringList lastStation = settings.value("lastchannel").toStringList();
        if( lastStation.count() == 2 )
            radioController->setAutoPlay(lastStation[1], lastStation[0]);
    }

    CGUIHelper *guiHelper = new CGUIHelper(radioController);

    // Create new QML application, set some requried options and load the QML file
    QQmlApplicationEngine* engine = new QQmlApplicationEngine;
    QQmlContext* rootContext = engine->rootContext();

    // Connect C++ code to QML GUI
    rootContext->setContextProperty("guiHelper", guiHelper);
    rootContext->setContextProperty("radioController", radioController);

    // Load main page
    engine->load(QUrl("qrc:/QML/MainView.qml"));

    // Add MOT slideshow provider
    engine->addImageProvider(QLatin1String("motslideshow"), guiHelper->motImage);

    // Run application
    app.exec();

    qDebug() << "main:" <<  "Application closed";

    // Delete the RadioController controller to ensure a save shutdown
    delete radioController;
    delete guiHelper;

    return 0;
}
