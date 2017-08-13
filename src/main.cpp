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

#include "DabConstants.h"
#include "CRadioController.h"
#include "CGUI.h"
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
#include "CAndroidJNI.h"
#include "rep_CRadioController_replica.h"
#endif

QTranslator* AddTranslator(QApplication *app, QString Language, QTranslator *OldTranslator = NULL);


int main(int argc, char** argv)
{
    QCoreApplication::setOrganizationName("welle.io");
    QCoreApplication::setOrganizationDomain("welle.io");
    QCoreApplication::setApplicationName("welle.io");
    QCoreApplication::setApplicationVersion(CURRENT_VERSION);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // Default values
    CDABParams DABParams(1);
    QVariantMap commandLineOptions;

#ifdef Q_OS_ANDROID
    //  Process Android Service
    if (argc == 2 && qstrcmp(argv[1], "-S") == 0) {
        qDebug() << "main:" <<  "Run as service, pid:" << QCoreApplication::applicationPid();

        // Create new QT core application
        QCoreApplication app(argc, argv);

        // Create a new radio interface instance
        CRadioController RadioController(commandLineOptions, DABParams);

        // Enable remoting source
        QRemoteObjectHost srcNode(QUrl(QStringLiteral("local:replica")));
        srcNode.enableRemoting(&RadioController);

        CAndroidJNI::getInstance().setRadioController(&RadioController);

        // Run application
        return app.exec();
    } else {
        // Start background service first
        QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                                  "startDabService",
                                                  "(Landroid/content/Context;)V",
                                                  QtAndroid::androidActivity().object());

        qDebug() << "main:" <<  "Run as application, pid:" << QCoreApplication::applicationPid();
    }
#endif

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
        QCoreApplication::translate("main", "I/Q RAW file format. Possible is: u8 (standard), s8, s16le, s16be. Only valid for input rawfile."),
        QCoreApplication::translate("main", "I/Q RAW file format"));
    optionParser.addOption(RAWFileFormat);

    //	Process the actual command line arguments given by the user
    optionParser.process(app);

    //	Process language option
    QString languageValue = optionParser.value(Language);
    if (languageValue != "")
        AddTranslator(&app, languageValue, Translator);

    //	Process DAB mode option
    QString DABModValue = optionParser.value(DABModeOption);
    if (DABModValue != "") {
        int Mode = DABModValue.toInt();
        if ((Mode < 1) || (Mode > 4))
            Mode = 1;

        DABParams.setMode(Mode);
    }

#ifdef Q_OS_ANDROID

    // Create a radio interface replica and connect to source
    QRemoteObjectNode repNode;
    repNode.connectToNode(QUrl(QStringLiteral("local:replica")));
    CRadioControllerReplica* RadioController = repNode.acquire<CRadioControllerReplica>();
    bool res = RadioController->waitForSource();
    Q_ASSERT(res);

#else

    commandLineOptions["dabDevice"] = optionParser.value(InputOption);
    commandLineOptions["ipAddress"] = optionParser.value(RTL_TCPServerIPOption);
    commandLineOptions["ipPort"] = optionParser.value(RTL_TCPServerIPPort);
    commandLineOptions["rawFile"] = optionParser.value(RAWFile);
    commandLineOptions["rawFileFormat"] = optionParser.value(RAWFileFormat);

    // Create a new radio interface instance
    CRadioController* RadioController = new CRadioController(commandLineOptions, DABParams);

#endif

    CGUI *GUI = new CGUI(RadioController);

    // Create new QML application, set some requried options and load the QML file
    QQmlApplicationEngine* engine = new QQmlApplicationEngine;
    QQmlContext* rootContext = engine->rootContext();

    // Connect C++ code to QML GUI
    rootContext->setContextProperty("cppGUI", GUI);
    rootContext->setContextProperty("cppRadioController", RadioController);

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

    // Special handling for German
    if(Language == "de_AT" || Language ==  "de_CH")
        Language = "de_DE";

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
