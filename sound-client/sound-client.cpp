#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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
 *     Communication via network to a DAB receiver to 
 *	have the sound locally
 */

#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	<QtNetwork>
#include	<QTcpSocket>
#include	<QHostAddress>
#include	"sound-client.h"
#include	"audiosink.h"
//

	soundClient::soundClient (QSettings	*s,
	                          QWidget *parent):QDialog (parent) {
int16_t	i;
	setupUi (this);
	remoteSettings	= s;
	connected	= false;
	connect (connectButton, SIGNAL (clicked (void)),
	         this, SLOT (wantConnect (void)));
	connect (terminateButton, SIGNAL (clicked (void)),
	         this, SLOT (terminate (void)));
	state	-> setText ("waiting to start");

	our_audioSink		= new audioSink		(48000);
	outTable		= new int16_t
	                             [our_audioSink -> numberofDevices ()];
	for (i = 0; i < our_audioSink -> numberofDevices (); i ++)
	   outTable [i] = -1;

	if (!setupSoundOut (streamOutSelector,
	                    our_audioSink, 48000, outTable)) {
	   fprintf (stderr, "Cannot open any output device\n");
	   exit (22);
	}

	our_audioSink	-> selectDefaultDevice ();
	connect (streamOutSelector, SIGNAL (activated (int)),
	              this, SLOT (setStreamOutSelector (int)));
	connectionTimer	= new QTimer ();
	connectionTimer	-> setInterval (1000);
	connect (connectionTimer, SIGNAL (timeout (void)),
	         this, SLOT (timerTick (void)));
}
//

	soundClient::~soundClient	(void) {
	our_audioSink	-> stop ();
	connected	= false;
	delete		our_audioSink;
}
//
void	soundClient::wantConnect (void) {
QString ipAddress;
int16_t	i;
QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

	if (connected)
	   return;
	// use the first non-localhost IPv4 address
	for (i = 0; i < ipAddressesList.size(); ++i) {
	   if (ipAddressesList.at (i) != QHostAddress::LocalHost &&
	      ipAddressesList. at (i). toIPv4Address ()) {
	      ipAddress = ipAddressesList. at(i). toString();
	      break;
	   }
	}
	// if we did not find one, use IPv4 localhost
	if (ipAddress. isEmpty())
	   ipAddress = QHostAddress (QHostAddress::LocalHost).toString ();
	ipAddress = remoteSettings ->
	                value ("remote-server", ipAddress). toString ();
	hostLineEdit -> setText (ipAddress);

	hostLineEdit -> setInputMask ("000.000.000.000");
//	Setting default IP address
	state	-> setText ("Give IP address, return");
	connect (hostLineEdit, SIGNAL (returnPressed (void)),
	         this, SLOT (setConnection (void)));
}

//	if/when a return is pressed in the line edit,
//	a signal appears and we are able to collect the
//	inserted text. The format is the IP-V4 format.
//	Using this text, we try to connect,
void	soundClient::setConnection (void) {
QString s	= hostLineEdit -> text ();
QHostAddress theAddress	= QHostAddress (s);
int32_t	basePort;
	basePort	= 20040;
	disconnect (hostLineEdit, SIGNAL (returnPressed (void)),
	            this, SLOT (setConnection (void)));
//
//	The streamer will provide us with the raw data
	streamer. connectToHost (theAddress, basePort);
	if (!streamer. waitForConnected (2000)) {
	   QMessageBox::warning (this, tr ("client"),
	                                   tr ("setting up stream failed\n"));
	   return;
	}

	connected	= true;
	state -> setText ("Connected");
	connect (&streamer, SIGNAL (readyRead (void)),
	         this, SLOT (readData (void)));
	connectionTimer	-> start (1000);
	our_audioSink	-> restart ();
}

//	These functions are typical for network use
void	soundClient::readData	(void) {
QByteArray d;

	d. resize (4 * 512);
	while (streamer. bytesAvailable () > 4 * 512) {
	   streamer. read (d. data (), d. size ());
	   toBuffer (d);
	}
}
//
#define	SCALE_FACTOR_30 32768
static inline
DSPCOMPLEX	makeComplex (uint8_t *buf) {
int16_t ii = 0; int16_t qq = 0;
int16_t	i = 0;

	uint8_t i0 = buf [i++];
	uint8_t i1 = buf [i++];

	uint8_t q0 = buf [i++];
	uint8_t q1 = buf [i++];

	ii = (i0 << 8) | i1;
	qq = (q0 << 8) | q1;
	return  DSPCOMPLEX ((float)ii / SCALE_FACTOR_30,
	                    (float)qq / SCALE_FACTOR_30);
}

//	we get in 16 bits (signed) integers, packed in unsigned bytes.
//	so, for one I/Q sample we need 4 bytes
void	soundClient::toBuffer (QByteArray d) {
int32_t	i;
int32_t j;
int32_t	length	= d. size ();
DSPCOMPLEX buffer [length / 4 ];
int	size	= 0;

	for (i = 0; i < length / 4; i++) {
	   uint8_t lbuf [4];
	   for (j = 0; j < 4; j ++)
	      lbuf [j] = d [4 * i + j];
	   buffer [i] = makeComplex (&lbuf [0]);
	   size ++;
	}
	our_audioSink ->  putSamples (buffer, size);
}

//	do not forget that ocnt starts with 1, due
//	to Qt list conventions
bool	soundClient::setupSoundOut (QComboBox	*streamOutSelector,
	                            audioSink	*our_audioSink,
	                            int32_t	cardRate,
	                            int16_t	*table) {
uint16_t	ocnt	= 1;
uint16_t	i;

	for (i = 0; i < our_audioSink -> numberofDevices (); i ++) {
	   const QString so = 
	             our_audioSink -> outputChannelwithRate (i, cardRate);
	   qDebug ("Investigating Device %d\n", i);

	   if (so != QString ("")) {
	      streamOutSelector -> insertItem (ocnt, so, QVariant (i));
	      table [ocnt] = i;
	      qDebug (" (output):item %d wordt stream %d (%s)\n", ocnt , i,
	                      so. toLatin1 ().data ());
	      ocnt ++;
	   }
	}

	qDebug () << "added items to combobox";
	return ocnt > 1;
}

void	soundClient::setStreamOutSelector (int idx) {
int16_t outputDevice;
	if (idx == 0)
	   return;

	outputDevice = outTable [idx];
	if (!our_audioSink -> isValidDevice (outputDevice)) 
	   return;

	our_audioSink	-> stop	();
	if (!our_audioSink -> selectDevice (outputDevice)) {
	   QMessageBox::warning (this, tr ("sdr"),
	                               tr ("Selecting  output stream failed\n"));
	   our_audioSink -> selectDefaultDevice ();
	   return;
	}

	qWarning () << "selected output device " << idx << outputDevice;
}

void	soundClient::setGain	(int g) {
	(void)g;
}

void	soundClient::terminate	(void) {
	if (connected) {
	   our_audioSink -> stop ();
	   remoteSettings -> setValue ("remote-server",
	                                streamer. peerAddress(). toString ());
	   streamer. close ();
	}
	accept ();
}

void	soundClient::timerTick (void) {
	if (streamer. state () == QAbstractSocket::UnconnectedState) {
	   state	-> setText ("not connected");
	   connected	= false;
	   connectionTimer	-> stop ();
	}
}
	   
