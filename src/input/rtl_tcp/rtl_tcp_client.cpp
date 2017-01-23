#
/*
 *    Copyright (C) 2010, 2011, 2012
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
 *    A simple client for rtl_tcp
 */

#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	<QHostAddress>
#include	<QTcpSocket>
#include	"rtl_tcp_client.h"
//
#define	DEFAULT_FREQUENCY	(Khz (220000))

	rtl_tcp_client::rtl_tcp_client	(QSettings *s,
	                                 bool *success, bool visible) {
	remoteSettings		= s;
	*success		= false;

	theFrame		= new QFrame;
	setupUi (theFrame);
	if (visible)
	   this	-> theFrame	-> show ();

    //	setting the defaults and constants
	theRate		= 2048000;
	remoteSettings	-> beginGroup ("rtl_tcp_client");
	theGain		= remoteSettings ->
	                          value ("rtl_tcp_client-gain", 20). toInt ();
	thePpm		= remoteSettings ->
	                          value ("rtl_tcp_client-ppm", 0). toInt ();
	vfoOffset	= remoteSettings ->
	                          value ("rtl_tcp_client-offset", 0). toInt ();
    basePort = remoteSettings ->
                    value ("rtl_tcp_port", 1234).toInt();
	remoteSettings	-> endGroup ();
	tcp_gain	-> setValue (theGain);
	tcp_ppm		-> setValue (thePpm);
	vfoFrequency	= DEFAULT_FREQUENCY;
	theBuffer	= new RingBuffer<uint8_t>(32 * 32768);
#ifdef	GUI_3
	theShadowBuffer	= new RingBuffer<uint8_t>(8192);
#endif
	connected	= false;
	hostLineEdit 	= new QLineEdit (NULL);

	if (visible) {
	   connect (tcp_connect, SIGNAL (clicked (void)),
	            this, SLOT (wantConnect (void)));
	   connect (tcp_disconnect, SIGNAL (clicked (void)),
	            this, SLOT (setDisconnect (void)));
	   connect (tcp_gain, SIGNAL (valueChanged (int)),
	            this, SLOT (sendGain (int)));
	   connect (tcp_ppm, SIGNAL (valueChanged (int)),
	            this, SLOT (set_fCorrection (int)));
	   connect (khzOffset, SIGNAL (valueChanged (int)),
	            this, SLOT (set_Offset (int)));
	   state	-> setText ("waiting to start");
	   *success	= true;
	   return;
	}
//
//	If we do not make the widget visible, we assume there is an
//	ipAddress in the remoteSettings, and we connect (try to connect)
//	to that

	remoteSettings -> beginGroup ("rtl_tcp_client");
	QString ipAddress = remoteSettings ->
	                value ("rtl_tcp_address", "127.0.0.1"). toString ();
	remoteSettings -> endGroup ();
	serverAddress	= QHostAddress (ipAddress);
	toServer. connectToHost (serverAddress, basePort);
	if (!toServer. waitForConnected (2000)) {
	   *success	= false;
	   return;
	}

	setAgc (true);
	sendRate (theRate);
	sendVFO	(DEFAULT_FREQUENCY);
	toServer. waitForBytesWritten ();
	connected	= true;
	*success	= true;
}

	rtl_tcp_client::~rtl_tcp_client	(void) {
	remoteSettings ->  beginGroup ("rtl_tcp_client");
	if (connected) {		// close previous connection
	   stopReader	();
//	   streamer. close ();
	   remoteSettings -> setValue ("remote-server",
	                               toServer. peerAddress (). toString ());
	   QByteArray datagram;
	}
	remoteSettings -> setValue ("rtl_tcp_client-gain",   theGain);
	remoteSettings -> setValue ("rtl_tcp_client-ppm",    thePpm);
	remoteSettings -> setValue ("rtl_tcp_client-offset", vfoOffset);
	remoteSettings -> endGroup ();
	toServer. close ();
	delete	theBuffer;
#ifdef	GUI_3
	delete 	theShadowBuffer;
#endif
	delete	hostLineEdit;
	delete	theFrame;
}
//
void	rtl_tcp_client::wantConnect (void) {
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
	remoteSettings -> beginGroup ("rtl_tcp_client");
	ipAddress = remoteSettings ->
	                value ("remote-server", ipAddress). toString ();
	remoteSettings -> endGroup ();
	hostLineEdit -> setText (ipAddress);

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
void	rtl_tcp_client::setConnection (void) {
QString s	= hostLineEdit -> text ();
QHostAddress theAddress	= QHostAddress (s);

	serverAddress	= QHostAddress (s);
	disconnect (hostLineEdit, SIGNAL (returnPressed (void)),
	            this, SLOT (setConnection (void)));
	toServer. connectToHost (serverAddress, basePort);
	if (!toServer. waitForConnected (2000)) {
	   QMessageBox::warning (theFrame, tr ("sdr"),
	                                   tr ("connection failed\n"));
	   return;
	}

	sendGain (theGain);
	sendRate (theRate);
	sendVFO	(DEFAULT_FREQUENCY - theRate / 4);
	toServer. waitForBytesWritten ();
	state -> setText ("Connected");
	connected	= true;
}

int32_t	rtl_tcp_client::getRate	(void) {
	return theRate;
}

bool	rtl_tcp_client::legalFrequency (int32_t f) {
	(void)f;
	return true;
}

int32_t	rtl_tcp_client::defaultFrequency (void) {
	return DEFAULT_FREQUENCY;	// choose any legal frequency here
}

void	rtl_tcp_client::setVFOFrequency	(int32_t newFrequency) {
	if (!connected)
	   return;
	vfoFrequency	= newFrequency;
//	here the command to set the frequency
	sendVFO (newFrequency);
}

int32_t	rtl_tcp_client::getVFOFrequency	(void) {
	return vfoFrequency;
}

bool	rtl_tcp_client::restartReader	(void) {
	if (!connected)
	   return false;
	connect (&toServer, SIGNAL (readyRead (void)),
	         this, SLOT (readData (void)));
	return true;
}

void	rtl_tcp_client::stopReader	(void) {
	if (connected)
	   disconnect (&toServer, SIGNAL (readyRead (void)),
	               this, SLOT (readData (void)));
}
//
//
//	The brave old getSamples. For the dab stick, we get
//	size: still in I/Q pairs, but we have to convert the data from
//	uint8_t to DSPCOMPLEX *
int32_t	rtl_tcp_client::getSamples (DSPCOMPLEX *V, int32_t size) { 
int32_t	amount, i;
uint8_t	*tempBuffer = (uint8_t *)alloca (2 * size * sizeof (uint8_t));
//
	amount = theBuffer	-> getDataFromBuffer (tempBuffer, 2 * size);
	for (i = 0; i < amount / 2; i ++)
	    V [i] = DSPCOMPLEX ((float (tempBuffer [2 * i] - 128)) / 128.0,
	                        (float (tempBuffer [2 * i + 1] - 128)) / 128.0);
	return amount / 2;
}

#ifdef	GUI_3
int32_t	rtl_tcp_client::getSamplesFromShadowBuffer (DSPCOMPLEX *V,
	                                            int32_t size) {
int32_t	amount, i;
uint8_t	*tempBuffer = (uint8_t *)alloca (2 * size * sizeof (uint8_t));
//
	amount = theShadowBuffer -> getDataFromBuffer(tempBuffer, 2 * size);
	for (i = 0; i < amount / 2; i ++)
	   V [i] = DSPCOMPLEX ((float (tempBuffer [2 * i] - 128)) / 128.0,
	                       (float (tempBuffer [2 * i + 1] - 128)) / 128.0);
	return amount / 2;
}
#endif

int32_t	rtl_tcp_client::Samples	(void) {
	return  theBuffer	-> GetRingBufferReadAvailable () / 2;
}
//
uint8_t	rtl_tcp_client::myIdentity	(void) {
	return DAB_STICK;
}

//
//	bitDepth is required in the forthcoming 6.1 release
//	In 6.0 it is not called
//	It is used to set the scale for the spectrum
int16_t	rtl_tcp_client::bitDepth	(void) {
	return 8;
}

//	These functions are typical for network use
void	rtl_tcp_client::readData	(void) {
uint8_t	buffer [8192];
	while (toServer. bytesAvailable () > 8192) {
	   toServer. read ((char *)buffer, 8192);
	   theBuffer -> putDataIntoBuffer (buffer, 8192);
#ifdef	GUI_3
       theShadowBuffer -> putDataIntoBuffer (buffer, 8192);
#endif
	}
}
//
//
//	commands are packed in 5 bytes, one "command byte" 
//	and an integer parameter
struct command {
	unsigned char cmd;
	unsigned int param;
}__attribute__((packed));

#define	ONE_BYTE	8

void	rtl_tcp_client::sendCommand (uint8_t cmd, int32_t param) {
QByteArray datagram;

	datagram. resize (5);
	datagram [0] = cmd;		// command to set rate
	datagram [4] = param & 0xFF;  //lsb last
	datagram [3] = (param >> ONE_BYTE) & 0xFF;
	datagram [2] = (param >> (2 * ONE_BYTE)) & 0xFF;
	datagram [1] = (param >> (3 * ONE_BYTE)) & 0xFF;
	toServer. write (datagram. data (), datagram. size ());
}

void rtl_tcp_client::sendVFO (int32_t frequency) {
	sendCommand (0x01, frequency);
}

void	rtl_tcp_client::sendRate (int32_t theRate) {
	sendCommand (0x02, theRate);
}

void	rtl_tcp_client::setGainMode (int32_t gainMode) {
    sendCommand (0x03, gainMode);
}

void	rtl_tcp_client::sendGain (int gain) {
	sendCommand (0x04, 10 * gain);
	theGain		= gain;
}

//
//	the "setGain" function is to accomodate gui_3.
void	rtl_tcp_client::setGain	(int32_t g) {
	sendGain (g);
}

void	rtl_tcp_client::setAgc		(bool b) {
	if (b)
	   setGainMode(0);
	else
	   setGainMode(1);
}

//	correction is in ppm
void	rtl_tcp_client::set_fCorrection	(int32_t ppm) {
	sendCommand (0x05, ppm);
	thePpm		= ppm;
}

void	rtl_tcp_client::setDisconnect (void) {
	if (connected) {		// close previous connection
	   stopReader	();
	   remoteSettings -> beginGroup ("rtl_tcp_client");
	   remoteSettings -> setValue ("remote-server",
	                               toServer. peerAddress (). toString ());
	   remoteSettings -> setValue ("rtl_tcp_client-gain", theGain);
	   remoteSettings -> setValue ("rtl_tcp_client-ppm", thePpm);
	   remoteSettings -> endGroup ();
	   toServer. close ();
	}
	connected	= false;
	connectedLabel	-> setText (" ");
	state		-> setText ("disconnected");
}

void	rtl_tcp_client::set_Offset	(int32_t o) {
	sendCommand (0x0a, Khz (o));
	vfoOffset	= o;
}

