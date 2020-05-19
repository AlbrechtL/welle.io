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
#include <QQuickStyle>
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
#include "waterfallitem.h"

#ifdef __ANDROID__
    #include <QtAndroid>
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

    // Before printing anything, we set
    setlocale(LC_ALL, "");

    // Create new QT application
    QApplication app(argc, argv);

    //Initialise translation
    QTranslator *translator = new QTranslator;
    QString lang = CGUIHelper::mapToLanguage("auto");
    CGUIHelper::loadTranslationFile(translator, lang);
    QCoreApplication::installTranslator(translator);

    // Register custiom types
    qmlRegisterType<WaterfallItem>("io.welle", 1, 0, "Waterfall");
    qRegisterMetaType<mot_file_t>("mot_file_t");

    // Set icon path
    QStringList themePaths;
    themePaths << ":/icons";
    QIcon::setThemeSearchPaths(themePaths);
    QIcon::setThemeName("welle_io_icons");

    // Set icon
    app.setWindowIcon(QIcon(":/icons/icon.png"));

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

    QCommandLineOption styleName("qqc-style",
        QCoreApplication::translate("main", "Qt Quick Controls Style for the 1st launch"),
        QCoreApplication::translate("main", "style_name"));
    optionParser.addOption(styleName);

    //	Process the actual command line arguments given by the user
    optionParser.process(app);

    // First of all process the log file
    QString LogFileNameValue = optionParser.value(LogFileName);
    if (LogFileNameValue != "")
    {
        CDebugOutput::setFileName(LogFileNameValue);
        qDebug() << "main: Version:" << Version;
    }

    QVariantMap commandLineOptions;
    commandLineOptions["dumpFileName"] = optionParser.value(dumpFileName);

    CRadioController radioController(commandLineOptions);

    QString styleNameArg = optionParser.value(styleName);
    //qDebug() << "Command line style_name: " << styleNameArg ;
    
    // Set the Qt Quick Style.
    QString styleToLoad = CGUIHelper::getQQStyleToLoad(styleNameArg);
    if (!styleToLoad.isEmpty())
        QQuickStyle::setStyle(styleToLoad);

    QSettings settings;
    settings.setValue("version", QString(CURRENT_VERSION));

    // Should we play the last station we have listened to previously?
    bool isAutoPlay = settings.value("enableLastPlayedStationState", false).toBool();
    QStringList lastStation = settings.value("lastchannel").toStringList();
    if( lastStation.count() == 2 )
        radioController.setAutoPlay(isAutoPlay, lastStation[1], lastStation[0]);

    // Load mandatory driver arguments to init input device
    radioController.setDeviceParam("SoapySDRDriverArgs", settings.value("soapyDriverArgs","").toString());
    radioController.setDeviceParam("SoapySDRAntenna", settings.value("soapyDriverAntenna","").toString());
    radioController.setDeviceParam("SoapySDRClockSource", settings.value("soapyDriverClockSource","").toString());

    CGUIHelper guiHelper(&radioController);
    guiHelper.setTranslator(translator);

    // Create new QML application, set some requried options and load the QML file
    QQmlApplicationEngine engine;
    QQmlContext* rootContext = engine.rootContext();

    // Connect C++ code to QML GUI
    rootContext->setContextProperty("guiHelper", &guiHelper);
    rootContext->setContextProperty("radioController", &radioController);

    // Load main page
    engine.load(QUrl("qrc:/QML/MainView.qml"));

    // Add MOT slideshow provider
    engine.addImageProvider(QLatin1String("SLS"), guiHelper.motImageProvider);

    // Run application
    app.exec();

    // Disconnect all signals and slots
    guiHelper.disconnect();
    radioController.disconnect();

    qDebug() << "main:" <<  "Application closed";

    return 0;
}
