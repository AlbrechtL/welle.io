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
#include    <QtEndian>

#include	"CRTL_TCP_Client.h"

//	commands are packed in 5 bytes, one "command byte"
//	and an integer parameter
struct command
{
    unsigned char cmd;
    unsigned int param;
}__attribute__((packed));

enum rtlsdr_tuner {
    RTLSDR_TUNER_UNKNOWN = 0,
    RTLSDR_TUNER_E4000,
    RTLSDR_TUNER_FC0012,
    RTLSDR_TUNER_FC0013,
    RTLSDR_TUNER_FC2580,
    RTLSDR_TUNER_R820T,
    RTLSDR_TUNER_R828D
};

#define	ONE_BYTE	8

CRTL_TCP_Client::CRTL_TCP_Client(RadioControllerInterface& radioController) :
    radioController(radioController)
{
    SampleBuffer	= new RingBuffer<uint8_t>(32 * 32768);
    SpectrumSampleBuffer	= new RingBuffer<uint8_t>(8192);

    connected = false;
    stopped = false;
    Frequency = kHz(220000);

    // Use default values
    isAGC = true;
    isHwAGC = false;
    CurrentGain	= 0;
    CurrentGainCount = 0;
    serverPort = 1234;
    serverAddress = QHostAddress("127.0.0.1");
    MinValue = 255;
    MaxValue = 0;

    FirstData = true;
    memset(&DongleInfo, 0, sizeof(dongle_info_t));

    connect(&TCPConnectionWatchDog, &QTimer::timeout, this, &CRTL_TCP_Client::TCPConnectionWatchDogTimeout);
    connect(&TCPSocket, &QTcpSocket::readyRead, this, &CRTL_TCP_Client::readData);
    connect(&TCPSocket, &QTcpSocket::disconnected, this, &CRTL_TCP_Client::disconnected);
    connect(&AGCTimer, &QTimer::timeout, this, &CRTL_TCP_Client::AGCTimerTimeout);
}

CRTL_TCP_Client::~CRTL_TCP_Client	(void)
{
#ifdef Q_OS_ANDROID
    // Send TCP_ANDROID_EXIT cmd to explicitly cause the driver to turn off itself
    sendCommand (0x7e, 0);
    TCPSocket. flush();
#endif

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

       if(FirstData)
       {
           FirstData = false;

           // Get dongle information
           memcpy(&DongleInfo, buffer, sizeof(dongle_info_t));

           // Convert the byte order
           DongleInfo.tuner_type = qFromBigEndian(DongleInfo.tuner_type);
           DongleInfo.tuner_gain_count = qFromBigEndian(DongleInfo.tuner_gain_count);

           if(DongleInfo.magic[0] == 'R' &&
              DongleInfo.magic[1] == 'T' &&
              DongleInfo.magic[2] == 'L' &&
              DongleInfo.magic[3] == '0')
           {
               QString TunerType;
               switch(DongleInfo.tuner_type)
               {
               case RTLSDR_TUNER_UNKNOWN: TunerType = "Unknown"; break;
               case RTLSDR_TUNER_E4000: TunerType = "E4000"; break;
               case RTLSDR_TUNER_FC0012: TunerType = "FC0012"; break;
               case RTLSDR_TUNER_FC0013: TunerType = "FC0013"; break;
               case RTLSDR_TUNER_FC2580: TunerType = "FC2580"; break;
               case RTLSDR_TUNER_R820T: TunerType = "R820T"; break;
               case RTLSDR_TUNER_R828D: TunerType = "R828D"; break;
               default: TunerType = "Unknown";
               }
               qDebug() << "RTL_TCP_CLIENT:" << "Tuner type:" << DongleInfo.tuner_type << TunerType ;
               qDebug() << "RTL_TCP_CLIENT:" << "Tuner gain count:" << DongleInfo.tuner_gain_count;
           }
           else
           {
               qDebug() << "RTL_TCP_CLIENT:" << "Doesn't found the \"RTL0\" magic key.";
           }
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
    FirstData = true;
    radioController.onMessage(message_level_t::Error, "RTL-TCP connection closed.");
#ifdef Q_OS_ANDROID
    QTimer::singleShot(0, RadioController, SLOT(closeDevice()));
#endif
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
    int32_t MaxGainCount = 0;
    switch(DongleInfo.tuner_type)
    {
    case RTLSDR_TUNER_E4000: MaxGainCount = e4k_gains.size(); break;
    case RTLSDR_TUNER_FC0012: MaxGainCount = fc0012_gains.size(); break;
    case RTLSDR_TUNER_FC0013: MaxGainCount = fc0013_gains.size(); break;
    case RTLSDR_TUNER_FC2580: MaxGainCount = fc2580_gains.size(); break;
    case RTLSDR_TUNER_R820T: MaxGainCount = r82xx_gains.size(); break;
    case RTLSDR_TUNER_R828D: MaxGainCount = r82xx_gains.size(); break;
    default: MaxGainCount = 29; // Most likely it is the R820T tuner
    }

    return MaxGainCount;
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

std::string CRTL_TCP_Client::getName()
{
    return "rtl_tcp_client (server: " +
        serverAddress.toString().toStdString() + ":" +
        QString::number(serverPort).toStdString() + ")";
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

            // Always use manual gain, the AGC is implemented in software
            setGainMode(1);

            setHwAgc(isHwAGC);

            if(isAGC)
            {
                setAgc(true);
            }
            else
            {
                setAgc(false);
            }

            setGain(CurrentGainCount);

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
        radioController.onMessage(message_level_t::Error,
                "Connection failed to server " +
                serverAddress.toString().toStdString() + ":" +
                std::to_string(serverPort));
        connected	= false;
        AGCTimer.stop();
    }
}

void CRTL_TCP_Client::AGCTimerTimeout(void)
{
    if(isAGC && (DongleInfo.tuner_type != RTLSDR_TUNER_UNKNOWN))
    {
        // Check for overloading
        if(MinValue == 0 || MaxValue == 255)
        {
            // We have to decrease the gain
            if(CurrentGainCount > 0)
            {
                setGain(CurrentGainCount - 1);
                qDebug() << "RTL_TCP_CLIENT:" << "Decrease gain to" << (float) CurrentGain;
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
                    qDebug() << "RTL_TCP_CLIENT:" << "Increase gain to" << CurrentGain;
                }
            }
        }
    }
    else // AGC is off or unknown tuner
    {
        if(MinValue == 0 || MaxValue == 255)
        {
            QString Text = QObject::tr("ADC overload. Maybe you are using a to high gain.");
            qDebug().noquote() << "RTL_TCP_CLIENT:" << Text;
            radioController.onMessage(message_level_t::Information,
                    "ADC overload. Maybe you are using a to high gain.");
        }
    }
}

float CRTL_TCP_Client::getGainValue(uint16_t GainCount)
{
    float gainValue = 0;

    if(DongleInfo.tuner_type == RTLSDR_TUNER_UNKNOWN)
        return 0;

    // Get max gain count
    uint32_t MaxGainCount = getGainCount();
    if(MaxGainCount == 0)
        return 0;

    // Check if GainCount is valid
    if(GainCount < MaxGainCount)
    {
        // Get gain
        switch(DongleInfo.tuner_type)
        {
        case RTLSDR_TUNER_E4000: gainValue = e4k_gains[GainCount]; break;
        case RTLSDR_TUNER_FC0012: gainValue = fc0012_gains[GainCount]; break;
        case RTLSDR_TUNER_FC0013: gainValue = fc0013_gains[GainCount]; break;
        case RTLSDR_TUNER_FC2580: gainValue = fc2580_gains[GainCount]; break;
        case RTLSDR_TUNER_R820T: gainValue = r82xx_gains[GainCount]; break;
        case RTLSDR_TUNER_R828D: gainValue = r82xx_gains[GainCount]; break;
        default: gainValue = 0;
        }
    }
    else
    {
        gainValue = 999.0; // Max gain
    }

    return gainValue;
}
