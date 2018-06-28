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
#include "CRadioController.h"
#include "CGUI.h"
#include "CLogFile.h"
#include "CSplashScreen.h"

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
#include "CAndroidJNI.h"
#include "rep_CRadioController_replica.h"
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

    qRegisterMetaTypeStreamOperators<QList<StationElement*>>("StationList");
#ifdef Q_OS_ANDROID

    //  Process Android Service
    if (argc == 2 && qstrcmp(argv[1], "-S") == 0) {
        qDebug() << "main:" <<  "Run as service, pid:" << QCoreApplication::applicationPid();

        // Create new QT core application
        QCoreApplication app(argc, argv);

        // Default values
        DABParams dabparams(1);
        QVariantMap commandLineOptions;

        // Create a new radio interface instance
        CRadioController* RadioController = new CRadioController(commandLineOptions, dabparams);

        // Enable remoting source
        QRemoteObjectHost srcNode(QUrl(QStringLiteral("local:replica")));
        srcNode.enableRemoting(RadioController);

        CAndroidJNI::getInstance().setRadioController(RadioController);

        // Run application
        app.exec();

        // Delete the RadioController controller to ensure a save shutdown
        delete RadioController;

        qDebug() << "main:" <<  "Service closed";

        return 0;
    } else {
        qDebug() << "main:" <<  "Run as application, pid:" << QCoreApplication::applicationPid();
    }

#endif

    // Before printing anything, we set
    setlocale(LC_ALL, "");

    // Create new QT application
    QApplication app(argc, argv);

    // Set icon path
    QStringList themePaths;
    themePaths << ":/src/gui/icon";
    QIcon::setThemeSearchPaths(themePaths);
    QIcon::setThemeName("welle_io_icons");

    // Set icon
    app.setWindowIcon(QIcon(":/icon.png"));

    // Init translations
    QString locale = QLocale::system().name();
    QTranslator *Translator = CGUI::AddTranslator(locale);

    // Default values
    DABParams dabparams(1);

    // Handle the command line
    QCommandLineParser optionParser;
    optionParser.setApplicationDescription("welle.io Help");
    optionParser.addHelpOption();
    optionParser.addVersionOption();

    QCommandLineOption InputOption(QStringList() << "d" << "device",
        QCoreApplication::translate("main", "Input device. Possible is: auto (default), airspy, rtl_tcp, rtl_sdr, rawfile, soapysdr"),
        QCoreApplication::translate("main", "Name"));
    optionParser.addOption(InputOption);

    QCommandLineOption RTL_TCPServerIPOption("rtl_tcp-address",
        QCoreApplication::translate("main", "rtl_tcp server IP address. Only valid for input rtl_tcp."),
        QCoreApplication::translate("main", "IP address"));
    optionParser.addOption(RTL_TCPServerIPOption);

    QCommandLineOption RTL_TCPServerIPPort("rtl_tcp-port",
        QCoreApplication::translate("main", "rtl_tcp server IP port. Only valid for input rtl_tcp."),
        QCoreApplication::translate("main", "Port"));
    optionParser.addOption(RTL_TCPServerIPPort);

    QCommandLineOption RAWFile("raw-file",
        QCoreApplication::translate("main", "I/Q RAW file. Only valid for input rawfile."),
        QCoreApplication::translate("main", "I/Q RAW file"));
    optionParser.addOption(RAWFile);

    QCommandLineOption RAWFileFormat("raw-format",
        QCoreApplication::translate("main", "I/Q RAW file format. Possible is: u8 (standard), s8, s16le, s16be, cf32. Only valid for input rawfile."),
        QCoreApplication::translate("main", "I/Q RAW file format"));
    optionParser.addOption(RAWFileFormat);

#ifdef HAVE_SOAPYSDR
    QCommandLineOption SDRDriverArgsOption("sdr-driver-args",
        QCoreApplication::translate("main", "The value depends on the SDR driver and is directly passed to it (currently only SoapySDR::Device::make(args)). A typical value for SoapySDR is a string like driver=remote,remote=127.0.0.1,remote:driver=rtlsdr,rtl=0"),
        QCoreApplication::translate("main", "args"));
    optionParser.addOption(SDRDriverArgsOption);

    QCommandLineOption SDRAntennaOption("sdr-antenna",
        QCoreApplication::translate("main", "The value depends on the SDR Hardware, typical values are TX/RX, RX2. Just query it with SoapySDRUtil --probe=driver=uhd"),
        QCoreApplication::translate("main", "antenna"));
    optionParser.addOption(SDRAntennaOption);

    QCommandLineOption SDRClockSourceOption("sdr-clock-source",
        QCoreApplication::translate("main", "The value depends on the SDR Hardware, typical values are internal, external, gpsdo. Just query it with SoapySDRUtil --probe=driver=uhd"),
        QCoreApplication::translate("main", "clock_source"));
    optionParser.addOption(SDRClockSourceOption);
#endif /* HAVE_SOAPYSDR */

    QCommandLineOption DABModeOption("dab-mode",
        QCoreApplication::translate("main", "DAB mode. Possible is: 1, 2 or 4, default: 1"),
        QCoreApplication::translate("main", "Mode"));
    optionParser.addOption(DABModeOption);

    QCommandLineOption dumpFileName("dump-file",
        QCoreApplication::translate("main", "Records DAB frames or DAB+ superframes. This file can be used to analyse the X-PAD data with XPADexpert."),
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

    QCommandLineOption InitialStation("station",
        QCoreApplication::translate("main", "Tries to play a station from a previous channel scan on program start."),
        QCoreApplication::translate("main", "Station name"));
    optionParser.addOption(InitialStation);

    QCommandLineOption DisableSplash("disable-splash",
        QCoreApplication::translate("main", "Disables the splash screen"));
    optionParser.addOption(DisableSplash);

    //	Process the actual command line arguments given by the user
    optionParser.process(app);

    bool isDisableSplash = optionParser.isSet(DisableSplash);
    if (!isDisableSplash)
    {
        CSplashScreen::Show();
        CSplashScreen::ShowMessage(QCoreApplication::translate("main","Starting welle.io"));
    }

    // First of all process the log file
    QString LogFileNameValue = optionParser.value(LogFileName);
    if (LogFileNameValue != "")
    {
        CLogFile::SetFileName(LogFileNameValue);
        qInstallMessageHandler(CLogFile::CustomMessageHandler);
        qDebug() << "main: Version:" << Version;
    }

    //	Process language option
    QString languageValue = optionParser.value(Language);
    if (languageValue != "")
        CGUI::AddTranslator(languageValue, Translator);

    //	Process DAB mode option
    QString DABModValue = optionParser.value(DABModeOption);
    if (DABModValue != "") {
        int Mode = DABModValue.toInt();
        if ((Mode < 1) || (Mode > 4))
            Mode = 1;

        dabparams.setMode(Mode);
    }

#ifdef Q_OS_ANDROID

    // Start background service
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabDelegate",
                                              "startDab",
                                              "(Landroid/content/Context;)V",
                                              QtAndroid::androidActivity().object());

    // Create a radio interface replica and connect to source
    QRemoteObjectNode repNode;
    repNode.connectToNode(QUrl(QStringLiteral("local:replica")));
    CRadioControllerReplica* RadioController = repNode.acquire<CRadioControllerReplica>();
    bool res = RadioController->waitForSource();
    Q_ASSERT(res);

#else

    QVariantMap commandLineOptions;
    commandLineOptions["dabDevice"] = optionParser.value(InputOption);
#ifdef HAVE_SOAPYSDR
    commandLineOptions["sdr-driver-args"] = optionParser.value(SDRDriverArgsOption);
    commandLineOptions["sdr-antenna"] = optionParser.value(SDRAntennaOption);
    commandLineOptions["sdr-clock-source"] = optionParser.value(SDRClockSourceOption);
#endif /* HAVE_SOAPYSDR */
    commandLineOptions["ipAddress"] = optionParser.value(RTL_TCPServerIPOption);
    commandLineOptions["ipPort"] = optionParser.value(RTL_TCPServerIPPort);
    commandLineOptions["rawFile"] = optionParser.value(RAWFile);
    commandLineOptions["rawFileFormat"] = optionParser.value(RAWFileFormat);
    commandLineOptions["dumpFileName"] = optionParser.value(dumpFileName);
    commandLineOptions["initialStation"] = optionParser.value(InitialStation);

    // Create a new radio interface instance
    CRadioController* RadioController = new CRadioController(commandLineOptions, dabparams);
    QTimer::singleShot(0, RadioController, SLOT(onEventLoopStarted())); // The timer is used to signal if the QT event lopp is running

#endif


    QSettings settings;

    // Should we play the last staiion we have listened to previously?
    if( settings.value("enableLastPlayedStationState", false).toBool() ) {

        QStringList lastStation = settings.value("lastchannel").toStringList();
        if( lastStation.count() == 2 )
            RadioController->setAutoPlay( lastStation[1], lastStation[0]);
    }


    // Should we start with a inital station given on command line?
    if( RadioController->Stations().count() > 0 && commandLineOptions["initialStation"] != "" ) {

        static QString channelToSearchFor = commandLineOptions["initialStation"].toString().simplified();

        QList<StationElement*> stationList = RadioController->Stations();
        QList<StationElement*>::iterator it;

        // try to find station name in the station list
        it = std::find_if(stationList.begin(), stationList.end(), [](StationElement* station) {

                return station->getStationName().simplified().indexOf( channelToSearchFor ) == 0;
        });

        if(it != stationList.end())
            RadioController->setAutoPlay((*it)->getChannelName(), (*it)->getStationName());
    }

    CGUI *GUI = new CGUI(RadioController);

    // Create new QML application, set some requried options and load the QML file
    QQmlApplicationEngine* engine = new QQmlApplicationEngine;
    QQmlContext* rootContext = engine->rootContext();

    // Connect C++ code to QML GUI
    rootContext->setContextProperty("cppGUI", GUI);
    rootContext->setContextProperty("cppRadioController", RadioController);

    // Load main page
    engine->load(QUrl("qrc:/src/gui/QML/MainView.qml"));

    // Add MOT slideshow provider
    engine->addImageProvider(QLatin1String("motslideshow"), GUI->MOTImage);

    // Run application
    app.exec();

    // Delete the RadioController controller to ensure a save shutdown
    delete GUI;
    delete RadioController;

    qDebug() << "main:" <<  "Application closed";

    return 0;
}
