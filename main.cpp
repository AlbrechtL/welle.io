#
/*
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Main program
 */
#include	<QApplication>
#include	<QSettings>
#include	<QDir>
#include	<QCommandLineParser>
#include	<unistd.h>
#include	"dab-constants.h"
#include	"gui.h"

QString	fullPathfor (QString v) {
int16_t	i;
QString	fileName;

	if (v == QString ("")) 
	   return QString ("/tmp/xxx");

	if (v. at (0) == QChar ('/')) 		// full path specified
	   return v;

	fileName = QDir::homePath ();
	fileName. append ("/");
	fileName. append (v);
	fileName = QDir::toNativeSeparators (fileName);

	if (!fileName. endsWith (".ini"))
	   fileName. append (".ini");

	return fileName;
}

#define	DEFAULT_INI	".welle.io.ini"

int	main (int argc, char **argv) {
QString	initFileName;
RadioInterface	*MyRadioInterface;

    QApplication a (argc, argv);
    a.setOrganizationName("AlbrechtL");
    a.setOrganizationDomain("welle.io");
    a.setApplicationName("welle.io");
    // Default values
    uint8_t		syncMethod	= 2;
    QSettings	*dabSettings;		// ini file
    uint8_t		dabMode		= 127;	// illegal value
    QString		dabDevice	= QString ("");
    QString		dabBand		= QString ("");
    QString		ipAddress	= QString ("");
    uint16_t	ipPort		= 1234;
    //
    //	Newer versions of Qt provide all kinds of nice mechanisms,
    //	unfortunately, there are quite some people (including me (jvk))
    //	who also work with older versions of Qt,
    QCoreApplication::setApplicationName("welle.io");
	QCoreApplication::setApplicationVersion(CURRENT_VERSION);

	QCommandLineParser optionParser;
    optionParser.setApplicationDescription("welle.io Help");
	optionParser.addHelpOption();
	optionParser.addVersionOption();

	QCommandLineOption INIFileOption ("i",
	          QCoreApplication::translate("main", "Settings INI-file path"),
	          QCoreApplication::translate("main", "Path"));
	optionParser.addOption(INIFileOption);

	QCommandLineOption SYNCOption ("S",
	          QCoreApplication::translate("main", "Sync method"),
	          QCoreApplication::translate("main", "Number"));
	optionParser.addOption(SYNCOption);
	QCommandLineOption InputOption ("D",
	          QCoreApplication::translate("main", "Input device"),
	          QCoreApplication::translate("main", "Name"));
	optionParser.addOption(InputOption);

	QCommandLineOption DABModeOption ("M",
	          QCoreApplication::translate("main", "DAB mode, possible are: 1,2 or 4, default: 1"),
	          QCoreApplication::translate("main", "Mode"));
	optionParser.addOption (DABModeOption);

	QCommandLineOption DABBandOption ("B",
	          QCoreApplication::translate("main", "DAB band"),
	          QCoreApplication::translate("main", "Band"));
	optionParser.addOption (DABBandOption);

	QCommandLineOption RTL_TCPServerIPOption ("I",
	          QCoreApplication::translate ("main", "rtl_tcp server IP address. Only valid for input rtl_tcp."),
	          QCoreApplication::translate ("main", "IP Address"));
	optionParser.addOption(RTL_TCPServerIPOption);

	QCommandLineOption RTL_TCPServerIPPort ("P",
	          QCoreApplication::translate("main", "rtl_tcp server IP port. Only valid for input rtl_tcp."),
	          QCoreApplication::translate ("main", "Port"));
	optionParser.addOption(RTL_TCPServerIPPort);

//	Process the actual command line arguments given by the user
	optionParser.process(a);

//	Process INI file option
	QString INIFileValue = optionParser.value(INIFileOption);
	if (INIFileValue != "")
	   initFileName = fullPathfor (INIFileValue);
	else
	   initFileName = fullPathfor (QString (DEFAULT_INI));

	dabSettings =  new QSettings (initFileName, QSettings::IniFormat);

//	Process Sync method option
	QString SYNCOptionValue = optionParser.value (SYNCOption);
	if (SYNCOptionValue != "")
	   syncMethod = SYNCOptionValue. toInt ();

//	Process input device option
	QString InputValue = optionParser. value (InputOption);
	if (InputValue != "")
	   dabDevice = InputValue;

//	Process DAB mode option
	QString DABModValue = optionParser.value(DABModeOption);
	if (DABModValue != "") {
	   dabMode	= DABModValue. toInt();
	   if (!(dabMode == 1) || (dabMode == 2) || (dabMode == 4))
	      dabMode = 1;
	}

//	Process DAB band option
	QString DABBandValue = optionParser.value (DABBandOption);
	if (DABBandValue != "")
	   dabBand = DABBandValue;

//	Process rtl_tcp server IP address option
	QString RTL_TCPServerIPValue =
	                    optionParser. value (RTL_TCPServerIPOption);
	if (RTL_TCPServerIPValue != "")
	   ipAddress = RTL_TCPServerIPValue;

//	Process rtl_tcp server IP portoption
	QString RTL_TCPServerPortValue =
	                    optionParser. value(RTL_TCPServerIPPort);
	if (RTL_TCPServerPortValue != "")
	   ipPort = RTL_TCPServerPortValue. toInt ();

//	We create the possibility for
//	passing appropriate parameters to the command
//	Selections - if any - will be default for the next session

	if (dabMode == 127)
	   dabMode = dabSettings -> value ("dabMode", 1). toInt ();
	if (dabDevice == QString (""))
	   dabDevice = dabSettings -> value ("device", "dabstick"). toString ();
	if (dabBand == QString (""))
	   dabBand = dabSettings -> value ("band", "BAND III"). toString ();
//
//	For gui_3 the data of a possible rtl_tcp should be saved
	a.setWindowIcon(QIcon(":/QML/images/icon.png"));
	dabSettings -> beginGroup ("rtl_tcp_client");
	if (ipAddress != QString ("")) {
	   dabSettings -> setValue ("rtl_tcp_address", ipAddress);
	   dabSettings -> setValue ("rtl_tcp_port", ipPort);
	}
	dabSettings -> endGroup ();

	(void)syncMethod;
	dabSettings -> setValue ("dabMode",	dabMode);
	dabSettings -> setValue ("device",	dabDevice);
	dabSettings -> setValue ("band",	dabBand);
	dabSettings	-> sync ();
	MyRadioInterface = new RadioInterface (dabSettings,
                                               dabDevice,
                                               dabMode,
                                               dabBand);

#if QT_VERSION >= 0x050600
	QGuiApplication::setAttribute (Qt::AA_EnableHighDpiScaling);
#endif    

    QQmlApplicationEngine *engine;

    // Create new QML application, set some requried options and load the QML file
    engine 	= new QQmlApplicationEngine;
    QQmlContext *rootContext = engine -> rootContext();
    rootContext -> setContextProperty("cppGUI", MyRadioInterface);
    engine->load(QUrl("qrc:/QML/main.qml"));
    engine->addImageProvider(QLatin1String("motslideshow"), MyRadioInterface->MOTImage);

	a. exec ();
/*
 *	done:
 */
	fflush (stdout);
	fflush (stderr);
	qDebug ("It is done\n");
	MyRadioInterface	-> ~RadioInterface ();
	dabSettings		-> ~QSettings ();
	exit (1);
}

