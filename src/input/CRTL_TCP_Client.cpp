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

#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	<QHostAddress>
#include	<QTcpSocket>
#include	"CRTL_TCP_Client.h"

//	commands are packed in 5 bytes, one "command byte"
//	and an integer parameter
struct command
{
    unsigned char cmd;
    unsigned int param;
}__attribute__((packed));

#define	ONE_BYTE	8

CRTL_TCP_Client::CRTL_TCP_Client(QSettings *settings)
{
    SampleBuffer	= new RingBuffer<uint8_t>(32 * 32768);
    SpectrumSampleBuffer	= new RingBuffer<uint8_t>(8192);

    connected = false;
    remoteSettings = settings;
    Frequency = Khz (220000);

    // Read settings, if not exist, use default values
    remoteSettings	-> beginGroup("rtl_tcp_client");
    theGain	= remoteSettings -> value("rtl_tcp_client-gain", 20).toInt();
    basePort = remoteSettings -> value("rtl_tcp_port", 1234).toInt();
    serverAddress = QHostAddress(remoteSettings -> value ("rtl_tcp_address", "127.0.0.1").toString());
    remoteSettings -> endGroup();

    connect(&TCPConnectionWatchDog, SIGNAL(timeout()), this, SLOT(TCPConnectionWatchDogTimeout()));
    connect(&TCPSocket, SIGNAL (readyRead (void)), this, SLOT (readData (void)));
}

CRTL_TCP_Client::~CRTL_TCP_Client	(void)
{
    // Save settings
    remoteSettings -> beginGroup ("rtl_tcp_client");
	remoteSettings -> setValue ("rtl_tcp_client-gain",   theGain);
	remoteSettings -> endGroup ();

    // Close connection
    TCPSocket. close();

    delete	SampleBuffer;
    delete 	SpectrumSampleBuffer;
}

void CRTL_TCP_Client::setFrequency(int32_t newFrequency)
{
    Frequency = newFrequency;
    sendVFO (newFrequency);
}

bool CRTL_TCP_Client::restart(void)
{
    TCPConnectionWatchDog.start(5000);
    TCPConnectionWatchDogTimeout(); // Call timout onces to start the connection

	if (!connected)
	   return false;
    else
        return true;
}

void CRTL_TCP_Client::stop	(void)
{
}

//	The brave old getSamples. For the dab stick, we get
//	size: still in I/Q pairs, but we have to convert the data from
//	uint8_t to DSPCOMPLEX *
int32_t	CRTL_TCP_Client::getSamples (DSPCOMPLEX *V, int32_t size)
{
    int32_t	amount, i;
    uint8_t	*tempBuffer = (uint8_t *)alloca (2 * size * sizeof (uint8_t));

    // Get data from the ring buffer
    amount = SampleBuffer	-> getDataFromBuffer (tempBuffer, 2 * size);
	for (i = 0; i < amount / 2; i ++)
	    V [i] = DSPCOMPLEX ((float (tempBuffer [2 * i] - 128)) / 128.0,
	                        (float (tempBuffer [2 * i + 1] - 128)) / 128.0);
	return amount / 2;
}

int32_t	CRTL_TCP_Client::getSpectrumSamples (DSPCOMPLEX *V, int32_t size)
{
    int32_t	amount, i;
    uint8_t	*tempBuffer = (uint8_t *)alloca (2 * size * sizeof (uint8_t));

    // Get data from the ring buffer
    amount = SpectrumSampleBuffer -> getDataFromBuffer(tempBuffer, 2 * size);
	for (i = 0; i < amount / 2; i ++)
	   V [i] = DSPCOMPLEX ((float (tempBuffer [2 * i] - 128)) / 128.0,
	                       (float (tempBuffer [2 * i + 1] - 128)) / 128.0);
	return amount / 2;
}

int32_t	CRTL_TCP_Client::getSamplesToRead	(void)
{
    return SampleBuffer->GetRingBufferReadAvailable () / 2;
}

void CRTL_TCP_Client::reset(void)
{
    SampleBuffer->FlushRingBuffer();
}

//	These functions are typical for network use
void CRTL_TCP_Client::readData(void)
{
    uint8_t	buffer[8192];

    while (TCPSocket. bytesAvailable () > 8192)
    {
       TCPSocket.read ((char *)buffer, 8192);
       SampleBuffer -> putDataIntoBuffer (buffer, 8192);
       SpectrumSampleBuffer -> putDataIntoBuffer (buffer, 8192);
	}
}

void CRTL_TCP_Client::sendCommand (uint8_t cmd, int32_t param)
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

void CRTL_TCP_Client::sendVFO(int32_t frequency)
{
	sendCommand (0x01, frequency);
}

void CRTL_TCP_Client::sendRate(int32_t theRate)
{
	sendCommand (0x02, theRate);
}

void CRTL_TCP_Client::setGainMode(int32_t gainMode)
{
    sendCommand (0x03, gainMode);
}

float CRTL_TCP_Client::setGain(int32_t gain)
{
    float gainValue = 0;

    switch(gain)
    {
        case 0: gainValue = 0.0; break;
        case 1: gainValue = 0.9; break;
        case 2: gainValue = 1.4; break;
        case 3: gainValue = 2.7; break;
        case 4: gainValue = 3.7; break;
        case 5: gainValue = 7.7; break;
        case 6: gainValue = 8.7; break;
        case 7: gainValue = 12.5; break;
        case 8: gainValue = 14.4; break;
        case 9: gainValue = 15.7; break;
        case 10: gainValue = 16.6; break;
        case 11: gainValue = 19.7; break;
        case 12: gainValue = 20.7; break;
        case 13: gainValue = 22.9; break;
        case 14: gainValue = 25.4; break;
        case 15: gainValue = 28.0; break;
        case 16: gainValue = 29.7; break;
        case 17: gainValue = 32.8; break;
        case 18: gainValue = 33.8; break;
        case 19: gainValue = 36.4; break;
        case 20: gainValue = 37.2; break;
        case 21: gainValue = 38.6; break;
        case 22: gainValue = 40.2; break;
        case 23: gainValue = 42.1; break;
        case 24: gainValue = 43.4; break;
        case 25: gainValue = 43.9; break;
        case 26: gainValue = 44.4; break;
        case 27: gainValue = 48.0; break;
        case 28: gainValue = 49.6; break;
        case 29: gainValue = 999; break; // Max gain
        default: gainValue = 0.0; fprintf(stderr, "Unknown gain count: %i\n.", gain);
    }

    sendCommand (0x04, (int) 10 * gainValue);
    theGain	= gainValue;

    return gainValue;
}

int32_t CRTL_TCP_Client::getGainCount()
{
    // rtl_tcp doesn't give us a gain count so use a hard code one from the RTL-SDR.com dongle
    return 29;
}

void CRTL_TCP_Client::setAgc(bool AGC)
{
    if (AGC)
	   setGainMode(0);
	else
        setGainMode(1);
}

void CRTL_TCP_Client::TCPConnectionWatchDogTimeout()
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
            sendRate(INPUT_RATE);
            sendVFO(Frequency);
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


