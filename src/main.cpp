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
#include <QCommandLineParser>
#include <QDir>
#include <QSettings>
#include <QIcon>
#include <QtQml/QQmlApplicationEngine>
#include <QQmlContext>

#include "CInputFactory.h"
#include "CRAWFile.h"
#include "CRTL_TCP_Client.h"
#include "DabConstants.h"
#include "CRadioController.h"
#include "CGUI.h"

QTranslator* AddTranslator(QApplication *app, QString Language, QTranslator *OldTranslator = NULL);


int main(int argc, char** argv)
{
    QCoreApplication::setOrganizationName("welle.io");
    QCoreApplication::setOrganizationDomain("welle.io");
    QCoreApplication::setApplicationName("welle.io");
    QCoreApplication::setApplicationVersion(CURRENT_VERSION);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // Before printing anything, we set
    setlocale(LC_ALL, "");

    // Create new QT application
    QApplication app(argc, argv);

    // Set ICON
    app.setWindowIcon(QIcon(":/icon.png"));

    // Init translations
    QString locale = QLocale::system().name();
    qDebug() << "main:" <<  "Detected system language" << locale;

    QTranslator *Translator = AddTranslator(&app, locale);

    // Default values
    CDABParams DABParams(1);
    QString dabDevice = "auto";
    QString ipAddress = "127.0.0.1";
    uint16_t ipPort = 1234;
    QString rawFile = "";
    QString rawFileFormat = "u8";

    // Handle the command line
    QCommandLineParser optionParser;
    optionParser.setApplicationDescription("welle.io Help");
    optionParser.addHelpOption();
    optionParser.addVersionOption();

    QCommandLineOption Language("L",
        QCoreApplication::translate("main", "Set the GUI language (e.g. de-DE)"),
        QCoreApplication::translate("main", "Language"));
    optionParser.addOption(Language);

    QCommandLineOption InputOption("D",
        QCoreApplication::translate("main", "Input device"),
        QCoreApplication::translate("main", "Name"));
    optionParser.addOption(InputOption);

    QCommandLineOption DABModeOption("M",
        QCoreApplication::translate("main", "DAB mode. Possible is: 1, 2 or 4, default: 1"),
        QCoreApplication::translate("main", "Mode"));
    optionParser.addOption(DABModeOption);

    QCommandLineOption RTL_TCPServerIPOption("I",
        QCoreApplication::translate("main", "rtl_tcp server IP address. Only valid for input rtl_tcp."),
        QCoreApplication::translate("main", "IP address"));
    optionParser.addOption(RTL_TCPServerIPOption);

    QCommandLineOption RTL_TCPServerIPPort("P",
        QCoreApplication::translate("main", "rtl_tcp server IP port. Only valid for input rtl_tcp."),
        QCoreApplication::translate("main", "Port"));
    optionParser.addOption(RTL_TCPServerIPPort);

    QCommandLineOption RAWFile("F",
        QCoreApplication::translate("main", "I/Q RAW file. Only valid for input rawfile."),
        QCoreApplication::translate("main", "I/Q RAW file"));
    optionParser.addOption(RAWFile);

    QCommandLineOption RAWFileFormat("B",
        QCoreApplication::translate("main", "I/Q RAW file format. Possible is: u8, s16le, default: u8. Only valid for input rawfile."),
        QCoreApplication::translate("main", "I/Q RAW file format"));
    optionParser.addOption(RAWFileFormat);

    //	Process the actual command line arguments given by the user
    optionParser.process(app);

    //	Process language option
    QString languageValue = optionParser.value(Language);
    if (languageValue != "")
        AddTranslator(&app, languageValue, Translator);

    //	Process input device option
    QString InputValue = optionParser.value(InputOption);
    if (InputValue != "")
        dabDevice = InputValue;

    //	Process DAB mode option
    QString DABModValue = optionParser.value(DABModeOption);
    if (DABModValue != "") {
        int Mode = DABModValue.toInt();
        if ((Mode < 1) || (Mode > 4))
            Mode = 1;

        DABParams.setMode(Mode);
    }

    //	Process rtl_tcp server IP address option
    QString RTL_TCPServerIPValue = optionParser.value(RTL_TCPServerIPOption);
    if (RTL_TCPServerIPValue != "")
        ipAddress = RTL_TCPServerIPValue;

    //	Process rtl_tcp server IP port option
    QString RTL_TCPServerPortValue = optionParser.value(RTL_TCPServerIPPort);
    if (RTL_TCPServerPortValue != "")
        ipPort = RTL_TCPServerPortValue.toInt();

    //	Process RAW file
    QString RAWFileValue = optionParser.value(RAWFile);
    if (RAWFileValue != "")
        rawFile = RAWFileValue;

    //	Process RAW file format
    QString RAWFileFormatValue = optionParser.value(RAWFileFormat);
    if (RAWFileFormatValue != "")
        rawFileFormat = RAWFileFormatValue;

    // Init device
    CVirtualInput* Device = CInputFactory::GetDevice(dabDevice);

    // Set rtl_tcp settings
    if (Device->getID() == CDeviceID::RTL_TCP) {
        CRTL_TCP_Client* RTL_TCP_Client = (CRTL_TCP_Client*)Device;

        RTL_TCP_Client->setIP(ipAddress);
        RTL_TCP_Client->setPort(ipPort);
    }

    // Set rawfile settings
    if (Device->getID() == CDeviceID::RAWFILE) {
        CRAWFile* RAWFile = (CRAWFile*)Device;

        RAWFile->setFileName(rawFile, rawFileFormat);
    }

    // Create a new radio interface instance
    CRadioController* RadioController = new CRadioController(Device, DABParams);
    CGUI *GUI = new CGUI(RadioController, &DABParams);

    // Create new QML application, set some requried options and load the QML file
    QQmlApplicationEngine* engine = new QQmlApplicationEngine;
    QQmlContext* rootContext = engine->rootContext();

    // Connect C++ code to QML GUI
    rootContext->setContextProperty("cppGUI", GUI);

    // Load main page
    engine->load(QUrl("qrc:/src/gui/QML/main.qml"));

    // Add MOT slideshow provider
    engine->addImageProvider(QLatin1String("motslideshow"), GUI->MOTImage);

    // Run application
    app.exec();

    // Delete the RadioController controller to ensure a save shutdown
    delete GUI;
    delete RadioController;

    return 0;
}

QTranslator* AddTranslator(QApplication *app, QString Language, QTranslator *OldTranslator)
{
    if(OldTranslator)
        app->removeTranslator(OldTranslator);

    QTranslator *Translator = new QTranslator;
    bool isTranslation = Translator->load(QString(":/i18n/") + Language);

    qDebug() << "main:" <<  "Set language" << Language;
    app->installTranslator(Translator);

    if(!isTranslation)
    {
        qDebug() << "main:" <<  "Error while loading language" << Language << "use English \"en_GB\" instead";
        Language = "en_GB";
    }

    QLocale curLocale(QLocale((const QString&)Language));
    QLocale::setDefault(curLocale);

    return Translator;
}
