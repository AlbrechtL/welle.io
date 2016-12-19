#
/*
 *    Copyright (C) 2016
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
 *     Communication via network to a dab-rpi program
 */
#include        <QtNetwork>
#include	<QThread>
#include        <QSettings>
#include        <QLabel>
#include        <QMessageBox>
#include        <QLineEdit>
#include        <QTimer>
#include        <QComboBox>
#include        "dab-client.h"
#include	"audiosink.h"
//
//	the messages that might be received from the "dab-server"
//
enum messages {
	COARSE_CORRECTOR	= 1,	// parameter is an int
	CLEAR_ENSEMBLE		= 2,	// no parameter
	ENSEMBLE_NAME		= 3,	// parameter is a QString
	PROGRAM_NAME		= 4,	// parameter is a QString
	SUCCESS_RATE		= 5,	// parameter is an int
	SIGNAL_POWER		= 6,	// parameter is an int
	SYNC_FLAG		= 7,	// parameter is an int
	STATION_TEXT		= 8,	// parameter is unknown
	FIC_FLAG		= 9,	// parameter is an int
	STEREO_FLAG		= 10	// parameter is an int
};

	dabClient::dabClient (QSettings *clientSettings,
	                      QWidget *parent):QDialog (parent) {
int	i;
int	attenuation;
	this	-> clientSettings	= clientSettings;
	setupUi (this);

	attenuation	= clientSettings	-> value ("attenuationslider", 50). toInt ();
	attenuationSlider	-> setValue (attenuation);
	
	connected	= false;
        connect (connectButton, SIGNAL (clicked (void)),
                 this, SLOT (wantConnect (void)));
        connect (disconnectButton, SIGNAL (clicked (void)),
                 this, SLOT (setDisconnect (void)));
        connect (startButton, SIGNAL (clicked (void)),
                 this, SLOT (setStart (void)));
        connect (quitButton, SIGNAL (clicked (void)),
                 this, SLOT (setQuit (void)));
        connect (channelSelector, SIGNAL (activated (const QString &)),
                 this, SLOT (setChannel(const QString &)));
	connect (ensembleDisplay, SIGNAL (clicked (QModelIndex)),
	         this, SLOT (selectService (QModelIndex)));
	connect (attenuationSlider, SIGNAL (valueChanged (int)),
	         this, SLOT (setAttenuation (int)));
	state	-> setText ("Waiting to start");

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
	stereoFlag ->
	         setStyleSheet ("QLabel {background-color : red}");
	syncFlag -> 
	         setStyleSheet ("QLabel {background-color : red}");
}

	dabClient::~dabClient	(void) {
	clientSettings	-> setValue ("attenuationSlider", 
	                                  attenuationSlider -> value ());
}

void	dabClient::wantConnect (void) {
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
	ipAddress = clientSettings -> 
	                value ("dab-server", ipAddress). toString ();

	hostLineEdit	->  setText (ipAddress);
	hostLineEdit	-> setInputMask ("000.000.000.000");
//	Setting default IP address
	hostLineEdit	-> show ();
	state	-> setText ("Give IP address, return");
	connect (hostLineEdit, SIGNAL (returnPressed (void)),
	         this, SLOT (setConnection (void)));
}

//	if/when a return is pressed in the line edit,
//	a signal appears and we are able to collect the
//	inserted text. The format is the IP-V4 format.
//	Using this text, we try to connect,
void	dabClient::setConnection (void) {
QString s	= hostLineEdit -> text ();
QHostAddress theAddress	= QHostAddress (s);

	serverAddress	= QHostAddress (s);
	basePort	= 20020;
	disconnect (hostLineEdit, SIGNAL (returnPressed (void)),
	            this, SLOT (setConnection (void)));
	controlServer. connectToHost (serverAddress, basePort);
	if (!controlServer. waitForConnected (2000)) {
	   QMessageBox::warning (this, tr ("sdr"),
	                                   tr ("connection (control) failed\n"));
	   return;
	}
	soundServer. connectToHost (serverAddress, basePort + 20);
	if (!soundServer. waitForConnected (2000)) {
	   QMessageBox::warning (this, tr ("sdr"),
	                                   tr ("connection (sound) failed\n"));
	   return;
	}

	connected	= true;
	state -> setText ("Connected");
	connect (&controlServer, SIGNAL (readyRead (void)),
	         this, SLOT (readMessages (void)));
	connect (&soundServer, SIGNAL (readyRead (void)),
	         this, SLOT (readSound (void)));
}

void	dabClient::setDisconnect (void) {
	if (connected) {		// close previous connection
	   QByteArray datagram;
	   datagram. resize (2);
	   datagram [0] = 0171;
	   datagram [1] = 2;
	   controlServer. write (datagram. data (), datagram. size ());
	   controlServer. close ();
	   soundServer. close ();
	}
	connected	= false;
	connectedLabel	-> setText (" ");
	state		-> setText ("disconnected");
}

void	dabClient::setStart	(void) {
QByteArray datagram;
int16_t i;

        datagram. resize (2);
        datagram [0] = 0170;
	datagram [1] = 2;
	if (connected)
           controlServer. write (datagram. data (), datagram. size ());
}

void	dabClient::setQuit	(void) {
	if (connected) {
	   clientSettings -> setValue ("dab-server",
                                        controlServer. peerAddress(). toString ());
	   clientSettings -> sync ();
	   controlServer. close ();
	}
	
	QApplication::quit ();
}

void	dabClient::setChannel	(const QString &s) {
int16_t	length	= s. length ();
QByteArray datagram;
int16_t	i;

	datagram. resize (18);

	datagram [0] = 0172;
	datagram [1] = 18;
	for (i = 2; i <= length + 1; i ++)
	   datagram [i] = (s. toLatin1 (). data ()) [i - 2];
	datagram [i] = 0;
	if (connected)
           controlServer. write (datagram. data (), datagram. size ());
}

void	dabClient::selectService	(QModelIndex ind) {
QString s = ensemble. data (ind, Qt::DisplayRole). toString ();
int16_t	length	= s. length ();
QByteArray datagram;
int16_t	i;

	datagram. resize (length + 3);

	datagram [0] = 0173;
	datagram [1] = length + 3;
	for (i = 2; i <= length + 1; i ++)
	   datagram [i] = (s. toLatin1 (). data ()) [i - 2];
	datagram [i] = 0;
	if (connected)
           controlServer. write (datagram. data (), datagram. size ());
}

void	dabClient::setAttenuation	(int gain) {
QByteArray datagram;
int16_t i;

        datagram. resize (3);
        datagram [0]    = 0174;
        datagram [1]    = 3;
        datagram [2]    = gain;
	if (connected)
           controlServer. write (datagram. data (), datagram. size ());
        gainDisplay     -> display (gain);
}

int16_t	getInteger (char *s, int l) {
uint16_t	res	= 0;

		res |= s [2] << 8;
	        res |= s [3];
	return (int16_t)res;
}


QString	stringFrom (QByteArray a) {
QString Result;
int16_t	i;

	for (i = 0; i < 16; i ++) {
	   if (a [i] == '\0')
	      return Result;
	   Result. append (char (a [i]));
	}
	return Result;
}

void	dabClient::readMessages	(void) {
QByteArray d;
int16_t		length;
int16_t		code;
uint8_t		magic;
int16_t		val;
QString		name;

	while (controlServer. bytesAvailable () > 0) {
	   while (true) {
	      controlServer. read ((char *)(&magic), 1);
	      if (magic == 0266)
	         break;
	   }
	   d. resize (2);
	   controlServer. read (d. data (), d. size ());

	   length 	= d [0] - 3;
	   code	= d [1];
	   if (length > 0) {
	      d. resize (length);
	      controlServer. read (d. data (), d. size ());
	   }

	   switch (code) {
	      case COARSE_CORRECTOR:
	         val	=  getInteger (d. data (), d. size ());
	         correctorDisplay	-> display (val);
	         break;
	      case CLEAR_ENSEMBLE:
	         Services	= QStringList ();
	         ensemble. setStringList (Services);
	         ensembleLabel	= QString ();
	         ensembleName	-> setText (ensembleLabel);
	         fprintf (stderr, "ensemble clean\n");
	         break;
	      case ENSEMBLE_NAME:
	         name	= stringFrom (d);
	         ensembleLabel	= name;
	         ensembleName	-> setText (ensembleLabel);
	         fprintf (stderr, "%s\n", name. toLatin1 (). data ());
	         break;
	      case PROGRAM_NAME:
	         name	= stringFrom (d);
	         Services << name;
	         Services. removeDuplicates ();
	         ensemble. setStringList (Services);
	         ensembleDisplay -> setModel (&ensemble);
	         fprintf (stderr, "%s\n", name. toLatin1 (). data ());
	         break;
	      case SYNC_FLAG:
	         val	= getInteger (d.data (), d. size ());
	         if (val != 0)
	            syncFlag -> 
	                  setStyleSheet ("QLabel {background-color : green}");
	         else
	            syncFlag ->
	                  setStyleSheet ("QLabel {background-color : red}");
	         break;
	      case STATION_TEXT:
	         break;
	      case FIC_FLAG:
	         val	= getInteger (d. data (), d. size ());
	         ficDisplay	-> display (val);
	         break;
	      case SUCCESS_RATE:
	         val	= getInteger (d. data (), d. size ());
	         mscDisplay	-> display (val);
	         break;
	      case SIGNAL_POWER:
	         val	= getInteger (d. data (), d. size ());
	         snrDisplay	-> display (val);
	         break;
	      case STEREO_FLAG:
	         val	= getInteger (d. data (), d. size ());
	         if (val != 0)
	            stereoFlag -> 
	                  setStyleSheet ("QLabel {background-color : green}");
	         else
	            stereoFlag ->
	                  setStyleSheet ("QLabel {background-color : red}");
	   }
	}
}


//	do not forget that ocnt starts with 1, due
//	to Qt list conventions
bool	dabClient::setupSoundOut (QComboBox	*streamOutSelector,
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

void	dabClient::setStreamOutSelector (int idx) {
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

//	These functions are typical for network use
void	dabClient::readSound	(void) {
QByteArray d;

	d. resize (4 * 512);
	while (soundServer. bytesAvailable () > 4 * 512) {
	   soundServer. read (d. data (), d. size ());
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
void	dabClient::toBuffer (QByteArray d) {
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

