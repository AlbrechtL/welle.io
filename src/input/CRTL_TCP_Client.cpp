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

CRTL_TCP_Client::CRTL_TCP_Client(CRadioController &RadioController)
{
    this->RadioController = &RadioController;
    SampleBuffer	= new RingBuffer<uint8_t>(32 * 32768);
    SpectrumSampleBuffer	= new RingBuffer<uint8_t>(8192);

    connected = false;
    stopped = false;
    Frequency = Khz (220000);

    // Use default values
    isAGC = true;
    isHwAGC = false;
    CurrentGain	= 0;
    CurrentGainCount = 0;
    serverPort = 1234;
    serverAddress = QHostAddress("127.0.0.1");
    MinValue = 255;
    MaxValue = 0;

    connect(&TCPConnectionWatchDog, &QTimer::timeout, this, &CRTL_TCP_Client::TCPConnectionWatchDogTimeout);
    connect(&TCPSocket, &QTcpSocket::readyRead, this, &CRTL_TCP_Client::readData);
    connect(&TCPSocket, &QTcpSocket::disconnected, this, &CRTL_TCP_Client::disconnected);
    connect(&AGCTimer, &QTimer::timeout, this, &CRTL_TCP_Client::AGCTimerTimeout);
}

CRTL_TCP_Client::~CRTL_TCP_Client	(void)
{
    // Close connection
    TCPSocket. close();

    delete	SampleBuffer;
    delete 	SpectrumSampleBuffer;
}

void CRTL_TCP_Client::setFrequency(int32_t newFrequency)
{
    stopped = false;
    Frequency = newFrequency;
    sendVFO (newFrequency);
}

bool CRTL_TCP_Client::restart(void)
{
    stopped = false;
    TCPConnectionWatchDog.start(5000);
    TCPConnectionWatchDogTimeout(); // Call timout onces to start the connection

	if (!connected)
	   return false;
    else
        return true;
}

void CRTL_TCP_Client::stop	(void)
{
    // Fake stopped due to missing RTL-TCP command
    stopped = true;
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
    stopped = false;
    SampleBuffer->FlushRingBuffer();
}

//	These functions are typical for network use
void CRTL_TCP_Client::readData(void)
{
    uint8_t	buffer[8192];

    while (TCPSocket. bytesAvailable () > 8192)
    {
       TCPSocket.read ((char *)buffer, 8192);
       if (stopped) {
           continue;
       }

       SampleBuffer -> putDataIntoBuffer (buffer, 8192);
       SpectrumSampleBuffer -> putDataIntoBuffer (buffer, 8192);

       // Check if device is overloaded
       MinValue = 255;
       MaxValue = 0;

       for(uint32_t i=0;i<8192;i++)
       {
           if(MinValue > buffer[i])
               MinValue = buffer[i];
           if(MaxValue < buffer[i])
               MaxValue = buffer[i];
       }
    }
}

void CRTL_TCP_Client::disconnected(void)
{
    if(RadioController)
        RadioController->setErrorMessage(QObject::tr("RTL-TCP connection closed."));
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
    CurrentGainCount = gain;
    float gainValue = getGainValue(gain);

    sendCommand (0x04, (int) 10 * gainValue);

    CurrentGain = gainValue;
    return gainValue;
}

int32_t CRTL_TCP_Client::getGainCount()
{
    // rtl_tcp doesn't give us a gain count so use a hard code one from the RTL-SDR.com dongle
    return 29;
}

void CRTL_TCP_Client::setAgc(bool AGC)
{
    isAGC = AGC;
}

void CRTL_TCP_Client::setHwAgc(bool hwAGC)
{
    isHwAGC = hwAGC;
    sendCommand (0x08, hwAGC ? 1 : 0);
}

bool CRTL_TCP_Client::isHwAgcSupported()
{
    return true;
}

QString CRTL_TCP_Client::getName()
{
    return "rtl_tcp_client (server: " + serverAddress.toString() + ":" + QString::number(serverPort) + ")";
}

CDeviceID CRTL_TCP_Client::getID()
{
    return CDeviceID::RTL_TCP;
}

void CRTL_TCP_Client::setIP(QString IPAddress)
{
    serverAddress = QHostAddress(IPAddress);
}

void CRTL_TCP_Client::setPort(uint16_t Port)
{
    serverPort = Port;
}

void CRTL_TCP_Client::TCPConnectionWatchDogTimeout()
{
    // Check the connection to the server
    if(!connected)
    {
        qDebug() << "RTL_TCP_CLIENT:" << "Try to connect to server" << serverAddress.toString() << ":" << serverPort;

        // Try to connect
        TCPSocket.connectToHost(serverAddress, serverPort);

        if(TCPSocket.waitForConnected(2000)) // Timeout 2 s
        {
            qDebug() << "RTL_TCP_CLIENT:" << "Successful connected to server";
            connected	= true;

            setHwAgc(isHwAGC);

            if(isAGC)
            {
                setAgc(true);
            }
            else
            {
                setAgc(false);
                setGain(CurrentGainCount);
            }

            sendRate(INPUT_RATE);
            sendVFO(Frequency);
            TCPSocket.waitForBytesWritten();

            AGCTimer.start(50);
        }
        else
        {
            qDebug() << "RTL_TCP_CLIENT:" << "Timeout while connecting to server";
            connected	= false;
        }
    }

    if(TCPSocket.state() != QTcpSocket::ConnectedState)
    {
        QString Text = QObject::tr("Connection failed to server ") + serverAddress.toString() + ":" + QString::number(serverPort);
        qDebug().noquote() << "RTL_TCP_CLIENT:" << Text;
        RadioController->setErrorMessage(Text);
        connected	= false;
        AGCTimer.stop();
    }
}

void CRTL_TCP_Client::AGCTimerTimeout(void)
{
    if(isAGC)
    {
        // Check for overloading
        if(MinValue == 0 || MaxValue == 255)
        {
            // We have to decrease the gain
            if(CurrentGainCount > 0)
            {
                setGain(CurrentGainCount - 1);
                //qDebug() << "RTL_TCP_CLIENT:" << "Decrease gain to" << (float) CurrentGain;
            }
        }
        else
        {
            if(CurrentGainCount < (getGainCount() - 1))
            {
                // Calc if a gain increase overloads the device. Calc it from the gain values
                float NewGain = getGainValue(CurrentGainCount + 1);
                float DeltaGain = NewGain - CurrentGain;
                float LinGain = pow(10, DeltaGain / 20);

                int NewMaxValue = (float) MaxValue * LinGain;
                int NewMinValue = (float) MinValue / LinGain;

                // We have to increase the gain
                if(NewMinValue >=0 && NewMaxValue <= 255)
                {
                    setGain(CurrentGainCount + 1);
                    //qDebug() << "RTL_TCP_CLIENT:" << "Increase gain to" << CurrentGain;
                }
            }
        }
    }
    else // AGC is off
    {
        if(MinValue == 0 || MaxValue == 255)
        {
            QString Text = QObject::tr("ADC overload. Maybe you are using a to high gain.");
            qDebug().noquote() << "RTL_TCP_CLIENT:" << Text;
            RadioController->setInfoMessage(Text);
        }
    }
}

float CRTL_TCP_Client::getGainValue(uint16_t GainCount)
{
    float gainValue = 0;

    // The rtl_tcp server doesn't delivers the possible gain values.
    // Instead, using gain values from the Rafael Micro R820T tuner
    switch(GainCount)
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
        default: gainValue = 0.0; qDebug() << "RTL_TCP_CLIENT:" << "Unknown gain count:" << GainCount;
    }

    return gainValue;
}
