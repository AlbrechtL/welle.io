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

#include "version.h"
#include "app_setup.h"
#include "radio_controller.h"
#include "gui_helper.h"
#include "debug_output.h"
#include "smoke_test.h"

int main(int argc, char** argv)
{
    QString Version = QString(CURRENT_VERSION) + " Git: " + GITHASH;

    QCoreApplication::setOrganizationName("welle.io");
    QCoreApplication::setOrganizationDomain("welle.io");
    QCoreApplication::setApplicationName("welle.io");
    QCoreApplication::setApplicationVersion(Version);
    
    // Handle debug output
    CDebugOutput::init();

    // Before printing anything, we set
    setlocale(LC_ALL, "");

    // Create new QT application
    QApplication app(argc, argv);
    qDebug() << "main: Platform name" <<  app.platformName();

    //Initialise translation
    QTranslator *translator = installTranslator();

    // QML types, icon theme and style
    setupQmlEnvironment();

    // Set icon
    app.setWindowIcon(QIcon(":/icons/icon.png"));

    // Handle the command line
    QCommandLineParser optionParser;
    optionParser.setApplicationDescription("welle.io is an open source DAB and DAB+ software defined radio (SDR) with support for rtl-sdr (RTL2832U), airspy and SoapySDR.");
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

    QCommandLineOption rawFileName("raw-file",
        QCoreApplication::translate("main", "Selects this RAW file as input device, like the settings page does."),
        QCoreApplication::translate("main", "File name"));
    optionParser.addOption(rawFileName);

    QCommandLineOption rawFileFormat("raw-format",
        QCoreApplication::translate("main", "Sample format of the RAW file: %1 (default: auto).").arg(CSmokeTest::rawFileFormats.join(", ")),
        QCoreApplication::translate("main", "Format"), "auto");
    optionParser.addOption(rawFileFormat);

    QCommandLineOption smokeTestReport("smoke-test",
        QCoreApplication::translate("main", "Plays every service of the RAW file with temporary settings, writes the result to a JSON file and quits. Exit code 0 if every service played."),
        QCoreApplication::translate("main", "Report file"));
    optionParser.addOption(smokeTestReport);

    QCommandLineOption smokeTestTimeout("smoke-timeout",
        QCoreApplication::translate("main", "Time limit of the smoke test (default: 120)."),
        QCoreApplication::translate("main", "Seconds"), "120");
    optionParser.addOption(smokeTestTimeout);

    //	Process the actual command line arguments given by the user
    optionParser.process(app);

    // First of all process the log file
    QString LogFileNameValue = optionParser.value(LogFileName);
    if (LogFileNameValue != "")
    {
        CDebugOutput::setFileName(LogFileNameValue);
        qDebug() << "main: Version:" << Version;
    }

    const bool isSmokeTest = optionParser.isSet(smokeTestReport);
    if (isSmokeTest && !optionParser.isSet(rawFileName)) {
        qCritical() << "main: --smoke-test needs --raw-file";
        return 1;
    }
    if (isSmokeTest && !CSmokeTest::isolateSettings()) {
        qCritical() << "main: No temporary directory for the settings of the smoke test";
        return 1;
    }
    if (optionParser.isSet(rawFileName)
            && !CSmokeTest::selectRawFile(optionParser.value(rawFileName), optionParser.value(rawFileFormat))) {
        qCritical().noquote() << "main: Unknown RAW file format" << optionParser.value(rawFileFormat);
        return 1;
    }

    QVariantMap commandLineOptions;
    commandLineOptions["dumpFileName"] = optionParser.value(dumpFileName);

    CRadioController radioController(commandLineOptions);

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

    // Create new QML application, set some required options and load the QML file
    QQmlApplicationEngine engine;
    setupQmlEngine(engine, &guiHelper, &radioController);

    // Created before loading, to see the QML warnings while loading
    std::unique_ptr<CSmokeTest> smokeTest;
    if (isSmokeTest) {
        smokeTest = std::make_unique<CSmokeTest>(optionParser.value(smokeTestReport),
            optionParser.value(smokeTestTimeout).toInt(), &radioController, &engine);
    }

    // Load main page
    engine.load(QUrl("qrc:/QML/MainView.qml"));

    if (smokeTest)
        smokeTest->start();

    // Run application
    int exitCode = app.exec();
    smokeTest.reset();

    // Disconnect all signals and slots
    guiHelper.disconnect();
    radioController.disconnect();

    qDebug() << "main:" <<  "Application closed";

    return exitCode;
}
