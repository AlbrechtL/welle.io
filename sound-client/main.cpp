#
/*
 *    Copyright (C) 2008, 2009, 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
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
#include	<QDir>
#include	"sound-client.h"

#define	DEFAULT_INI	".sound-client.ini"

QSettings	*ISettings;
#ifdef	__MINGW32__
#include	"windows.h"
#endif

int	main (int argc, char **argv) {
soundClient	*myClient;

QString	inifile = QDir::homePath ();
	inifile. append ("/");
	inifile. append (DEFAULT_INI);
	inifile	= QDir::toNativeSeparators (inifile);
	fprintf (stderr, " ini file = %s\n", inifile. toLatin1 (). data ());
	ISettings	= new QSettings (inifile, QSettings::IniFormat);

	QApplication a (argc, argv);
	myClient 	= new soundClient (ISettings);
	myClient	-> show ();
	a. exec ();
/*
 *	done:
 */
	fflush (stdout);
	fflush (stderr);
	qDebug ("It is done\n");
}

