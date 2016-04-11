
#
/*
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the  SDR-J series.
 *    Many of the ideas as implemented in the SDR-J are derived from
 *    other work, made available through the (a) GNU general Public License. 
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
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

void	RadioInterface::newgui (void) {
	theFrame	= new QWidget;
	theLayout	= new verticalLayout;
	topRow		= new horizontalLayout;
//
	ensembleName	= new QLabel;
	ensembleId	= new QLCDNumber;
	snrDisplay	= new QLCDNumber;
	ficRatioDisplay	= new QLCDNumber;
	errorDisplay	= new QLCDNumber;
	fineCorrectorDisplay	= new QLCDNmber;
	coarseCorrectorDisplay	= new QLCDNmber;
	syncedLabel	= new QLabel;
	topRow		-> addWidget (ensembleName,	1);
	topRow		-> addWidget (ensembleId,	2);
	topRow		-> addWidget (snrDisplay,	3);
	topRow		-> addWidget (ficRatioDisplay,	4);
	topRow		-> addWidget (errorDisplay,	5);
	topRow		-> addWidget (fineCorrectorDisplay, 6);
	topRow		-> addWidget (coarseCorrectorDisplay, 7);
	topRow		-> addWidget (syncedLabel,	8);

	secondRow	= new horizontalLayout;
	startButton	= new QButton;
	quitButton	= new QButton;
	resetButton	= new QButton;
	modeSelector	= new QComboBox;
	modeSelector	-> addItem ("1");
	modeSelector	-> addItem ("2");
	modeSelector	-> addItem ("3");
	modeSelector	-> addItem ("4");
	bandSelector	= new QComboBox;
	bandSelector	-> addItem ("Band III");
	bandSelector	-> addItem ("L Band");
	channelSelect	= new QComboBox;
	deviceSelector	= new QComboBox;
	secondRow	-> addWidget (startButton, 1);
	secondRow	-> addWidget (quitButton, 2);
	secondRow	-> addWidget (resetButton, 3);
	secondRow	-> addWidget (modeSelect, 4);
	secondRow	-> addWidget (bandSelect, 5);
	secondRow	-> addWidget (channelSelect, 6);

	thirdRow	= new horizontalLayout;
	audioDumpButton	= new QButton ("audio");
	dumpButton	= new QButton ("dump");
	dynamicLabel	= new QLabel;
	crcErrors1	= new QLCDNumber;
	crcErrors1	= new QLCDNumber;
	thirdRow	-> addWidget (audioDumpButton, 1);
	thirdRow	-> addWidget (dumpButton, 2);
	thirdRow	-> addWidget (dynamicLabel, 3);
	thirdRow	-> addWidget (crcErrors1, 4);
	thirdRow	-> addWidget (crcErrors2, 5);

	fourthRow	= new horizontalLayout;
	ensembleDisplay	= new QListView;
	fourthRow	-> addWidget (ensembleDisplay, 1);

	fifthRow	= new horizontalLayout;
	versionName	= new QLabel;
	timeDisplay	= new QLabel;
	fifthRow	-> addWidget (versionName, 1);
	fifthRow	-> addWidget (timeDisplay, 2);

	theLayout	-> addWidget (topRow, 1);
	theLayout	-> addWidget (secondRow, 2);
	theLayout	-> addWidget (thirdRow, 3);
	theLayout	-> addWidget (fourthRow, 4);
	theLayout	-> addWidget (fifthRow, 5);
	theFrame	-> setLayout (theLayout);
}

