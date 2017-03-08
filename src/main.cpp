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
#include <QCommandLineParser>
#include <QDir>
#include <QSettings>
#include <unistd.h>

#include "CInputFactory.h"
#include "CRAWFile.h"
#include "CRTL_TCP_Client.h"
#include "DabConstants.h"
#include "gui.h"

int main(int argc, char** argv)
{
    QCoreApplication::setOrganizationName("welle.io");
    QCoreApplication::setOrganizationDomain("welle.io");
    QCoreApplication::setApplicationName("welle.io");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icon.png"));

    // Default values
    CDABParams DABParams(1);
    QString dabDevice = "auto";
    QString ipAddress = "127.0.0.1";
    uint16_t ipPort = 1234;
    QString rawFile = "";

    //	Newer versions of Qt provide all kinds of nice mechanisms,
    //	unfortunately, there are quite some people (including me (jvk))
    //	who also work with older versions of Qt,
    QCoreApplication::setApplicationName("welle.io");
    QCoreApplication::setApplicationVersion(CURRENT_VERSION);

    QCommandLineParser optionParser;
    optionParser.setApplicationDescription("welle.io Help");
    optionParser.addHelpOption();
    optionParser.addVersionOption();

    QCommandLineOption InputOption("D",
        QCoreApplication::translate("main", "Input device"),
        QCoreApplication::translate("main", "Name"));
    optionParser.addOption(InputOption);

    QCommandLineOption DABModeOption("M",
        QCoreApplication::translate("main", "DAB mode, possible are: 1,2 or 4, default: 1"),
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

    QCommandLineOption RAWFILE("F",
        QCoreApplication::translate("main", "I/Q RAW file. Only valid for input rawfile."),
        QCoreApplication::translate("main", "I/Q RAW file"));
    optionParser.addOption(RAWFILE);

    //	Process the actual command line arguments given by the user
    optionParser.process(a);

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

    //	Process raw file
    QString RAWFileValue = optionParser.value(RAWFILE);
    if (RAWFileValue != "")
        rawFile = RAWFileValue;

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

        RAWFile->setFileName(rawFile);
    }

    // Create a new radio interface instance
    RadioInterface* GUI = new RadioInterface(Device, DABParams);

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
    a.exec();

    return 0;
}
