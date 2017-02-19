/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    Bases on SDR-J
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

#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	<QHostAddress>
#include	<QTcpSocket>
#include	"rtl_tcp_client.h"

//	commands are packed in 5 bytes, one "command byte"
//	and an integer parameter
struct command
{
    unsigned char cmd;
    unsigned int param;
}__attribute__((packed));

#define	ONE_BYTE	8

rtl_tcp_client::rtl_tcp_client	(QSettings *settings, bool *success)
{
    theBuffer	= new RingBuffer<uint8_t>(32 * 32768);
    theShadowBuffer	= new RingBuffer<uint8_t>(8192);

    connected = false;
    remoteSettings = settings;
    theRate	= 2048000;
    vfoFrequency = Khz (220000);

    // Read settings, if not exist, use default values
    remoteSettings	-> beginGroup("rtl_tcp_client");
    theGain	= remoteSettings -> value("rtl_tcp_client-gain", 20).toInt();
    thePpm = remoteSettings -> value("rtl_tcp_client-ppm", 0).toInt();
    vfoOffset = remoteSettings -> value("rtl_tcp_client-offset", 0).toInt();
    basePort = remoteSettings -> value("rtl_tcp_port", 1234).toInt();
    serverAddress = QHostAddress(remoteSettings -> value ("rtl_tcp_address", "127.0.0.1").toString());
    remoteSettings -> endGroup();

    connect(&TCPConnectionWatchDog, SIGNAL(timeout()), this, SLOT(TCPConnectionWatchDogTimeout()));
    connect(&TCPSocket, SIGNAL (readyRead (void)), this, SLOT (readData (void)));

	*success	= true;
}

rtl_tcp_client::~rtl_tcp_client	(void)
{
    remoteSettings -> beginGroup ("rtl_tcp_client");
    if (connected)
    {		// close previous connection
	   stopReader	();
//	   streamer. close ();
       remoteSettings -> setValue ("remote-server", TCPSocket. peerAddress (). toString ());
	}
	remoteSettings -> setValue ("rtl_tcp_client-gain",   theGain);
	remoteSettings -> setValue ("rtl_tcp_client-ppm",    thePpm);
	remoteSettings -> setValue ("rtl_tcp_client-offset", vfoOffset);
	remoteSettings -> endGroup ();
    TCPSocket. close ();
	delete	theBuffer;
	delete 	theShadowBuffer;
}

void rtl_tcp_client::setVFOFrequency(int32_t newFrequency)
{
	vfoFrequency	= newFrequency;
    sendVFO (newFrequency);
}

int32_t	rtl_tcp_client::getVFOFrequency(void)
{
	return vfoFrequency;
}

bool rtl_tcp_client::restartReader(void)
{
    TCPConnectionWatchDog.start(5000);
    TCPConnectionWatchDogTimeout(); // Call timout onces to start the connection

	if (!connected)
	   return false;
    else
        return true;
}

void rtl_tcp_client::stopReader	(void)
{
}

//	The brave old getSamples. For the dab stick, we get
//	size: still in I/Q pairs, but we have to convert the data from
//	uint8_t to DSPCOMPLEX *
int32_t	rtl_tcp_client::getSamples (DSPCOMPLEX *V, int32_t size)
{
    int32_t	amount, i;
    uint8_t	*tempBuffer = (uint8_t *)alloca (2 * size * sizeof (uint8_t));

    // Get data from the ring buffer
	amount = theBuffer	-> getDataFromBuffer (tempBuffer, 2 * size);
	for (i = 0; i < amount / 2; i ++)
	    V [i] = DSPCOMPLEX ((float (tempBuffer [2 * i] - 128)) / 128.0,
	                        (float (tempBuffer [2 * i + 1] - 128)) / 128.0);
	return amount / 2;
}

int32_t	rtl_tcp_client::getSamplesFromShadowBuffer (DSPCOMPLEX *V, int32_t size)
{
    int32_t	amount, i;
    uint8_t	*tempBuffer = (uint8_t *)alloca (2 * size * sizeof (uint8_t));

    // Get data from the ring buffer
	amount = theShadowBuffer -> getDataFromBuffer(tempBuffer, 2 * size);
	for (i = 0; i < amount / 2; i ++)
	   V [i] = DSPCOMPLEX ((float (tempBuffer [2 * i] - 128)) / 128.0,
	                       (float (tempBuffer [2 * i + 1] - 128)) / 128.0);
	return amount / 2;
}

int32_t	rtl_tcp_client::Samples	(void)
{
    return theBuffer->GetRingBufferReadAvailable () / 2;
}

uint8_t	rtl_tcp_client::myIdentity	(void)
{
	return DAB_STICK;
}

//	These functions are typical for network use
void rtl_tcp_client::readData(void)
{
    uint8_t	buffer[8192];

    while (TCPSocket. bytesAvailable () > 8192)
    {
       TCPSocket.read ((char *)buffer, 8192);
	   theBuffer -> putDataIntoBuffer (buffer, 8192);
       theShadowBuffer -> putDataIntoBuffer (buffer, 8192);
	}
}

void rtl_tcp_client::sendCommand (uint8_t cmd, int32_t param)
{
    if(!connected)
        return;

    QByteArray datagram;

	datagram. resize (5);
	datagram [0] = cmd;		// command to set rate
	datagram [4] = param & 0xFF;  //lsb last
	datagram [3] = (param >> ONE_BYTE) & 0xFF;
	datagram [2] = (param >> (2 * ONE_BYTE)) & 0xFF;
	datagram [1] = (param >> (3 * ONE_BYTE)) & 0xFF;
    TCPSocket. write (datagram. data (), datagram. size ());
}

void rtl_tcp_client::sendVFO(int32_t frequency)
{
	sendCommand (0x01, frequency);
}

void rtl_tcp_client::sendRate(int32_t theRate)
{
	sendCommand (0x02, theRate);
}

void rtl_tcp_client::setGainMode(int32_t gainMode)
{
    sendCommand (0x03, gainMode);
}

void rtl_tcp_client::sendGain(int gain)
{
	sendCommand (0x04, 10 * gain);
	theGain		= gain;
}

void rtl_tcp_client::set_fCorrection(int32_t ppm)
{
    sendCommand (0x05, ppm);
    thePpm		= ppm;
}

void rtl_tcp_client::set_Offset(int32_t o)
{
    sendCommand (0x0a, Khz (o));
    vfoOffset	= o;
}

void rtl_tcp_client::setGain(int32_t g)
{
	sendGain (g);
}

void rtl_tcp_client::setAgc(bool b)
{
	if (b)
	   setGainMode(0);
	else
	   setGainMode(1);
}

void rtl_tcp_client::TCPConnectionWatchDogTimeout()
{
    // Check the connection to the server
    if(!connected)
    {
        fprintf(stderr, "Try to connect to server %s:%lli\n", serverAddress.toString().toStdString().c_str(), basePort);

        // Try to connect
        TCPSocket.connectToHost(serverAddress, basePort);

        if(TCPSocket.waitForConnected(2000)) // Timeout 2 s
        {
            fprintf(stderr, "Successful connected to server\n");
            connected	= true;

            setAgc(true);
            sendRate(theRate);
            sendVFO(vfoFrequency);
            TCPSocket.waitForBytesWritten();
        }
        else
        {
            fprintf(stderr, "Timeout while connecting to server\n");
            connected	= false;
        }
    }

    if(TCPSocket.state() != QTcpSocket::ConnectedState)
    {
        fprintf(stderr, "Connection failed to server %s:%lli\n", serverAddress.toString().toStdString().c_str(), basePort);
        connected	= false;
    }
}


