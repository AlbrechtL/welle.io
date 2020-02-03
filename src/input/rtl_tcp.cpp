/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
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

#include <iostream>
#include "rtl_tcp.h"

// For Qt translation if Qt is exisiting
#ifdef QT_CORE_LIB
    #include <QtGlobal>
#else
    #define QT_TRANSLATE_NOOP(x,y) (y)
#endif

// commands are packed in 5 bytes, one "command byte"
// and an integer parameter
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

#define ONE_BYTE 8

CRTL_TCP_Client::CRTL_TCP_Client(RadioControllerInterface& radioController) :
    radioController(radioController),
    sampleBuffer(32 * 32768),
    spectrumSampleBuffer(8192)
{
    memset(&dongleInfo, 0, sizeof(dongle_info_t));
    dongleInfo.tuner_type = RTLSDR_TUNER_UNKNOWN;
}

CRTL_TCP_Client::~CRTL_TCP_Client(void)
{
    stop();
}

void CRTL_TCP_Client::setFrequency(int newFrequency)
{
    frequency = newFrequency;
    sendVFO(newFrequency);
}

int CRTL_TCP_Client::getFrequency() const
{
    return frequency;
}

bool CRTL_TCP_Client::restart(void)
{
    if (rtlsdrRunning) {
        return true;
    }

    rtlsdrRunning = true;

    receiveThread = std::thread(&CRTL_TCP_Client::receiveAndReconnect, this);
    receiveThread.detach();

    // Wait so that the other thread has a chance to establish the connection
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::unique_lock<std::mutex> lock(mutex);
    return connected;
}

bool CRTL_TCP_Client::is_ok()
{
    return rtlsdrRunning;
}

void CRTL_TCP_Client::stop(void)
{
#ifdef __ANDROID__
    // Send TCP_ANDROID_EXIT cmd to explicitly cause the driver to turn off itself
    sendCommand(0x7e, 0);
#endif

    std::unique_lock<std::mutex> lock(mutex);

    agcRunning = false;
    rtlsdrRunning = false;

    // Close connection
    sock.close();

    lock.unlock();

    if (agcThread.joinable()) {
        agcThread.join();
    }

    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}

static int32_t read_convert_from_buffer(
        RingBuffer<uint8_t>& buffer,
        DSPCOMPLEX *v, int32_t size)
{
    int32_t amount, i;
    std::vector<uint8_t> tempBuffer(2 * size);

    // Get data from the ring buffer
    amount = buffer.getDataFromBuffer(tempBuffer.data(), 2 * size);
    for (i = 0; i < amount / 2; i ++)
        v[i] = DSPCOMPLEX(((float)tempBuffer[2 * i] - 128.0f) / 128.0f,
                          ((float)tempBuffer[2 * i + 1] - 128.0f) / 128.0f);
    return amount / 2;
}

int32_t CRTL_TCP_Client::getSamples(DSPCOMPLEX *v, int32_t size)
{
    return read_convert_from_buffer(sampleBuffer, v, size);
}

std::vector<DSPCOMPLEX> CRTL_TCP_Client::getSpectrumSamples(int size)
{
    std::vector<DSPCOMPLEX> buffer(size);
    int sizeRead = read_convert_from_buffer(spectrumSampleBuffer, buffer.data(), size);
    if (sizeRead < size) {
        buffer.resize(sizeRead);
    }
    return buffer;
}

int32_t CRTL_TCP_Client::getSamplesToRead(void)
{
    return sampleBuffer.GetRingBufferReadAvailable () / 2;
}

void CRTL_TCP_Client::reset(void)
{
    sampleBuffer.FlushRingBuffer();
}

void CRTL_TCP_Client::receiveData(void)
{
    std::vector<uint8_t> buffer(8192);

    size_t read = 0;

    while (sock.valid() && read < buffer.size()) {
        const size_t remain = buffer.size() - read;

        ssize_t ret = sock.recv(buffer.data() + read, remain, 0);

        if (ret == 0) {
            handleDisconnect();
        }
        else if (ret == -1) {
            if (errno == EAGAIN) {
                continue;
            }
            else if (errno == EINTR) {
                continue;
            }
            else if (errno == ECONNRESET || errno == EBADF) {
                handleDisconnect();
            }
            else {
                std::string errstr = strerror(errno);
                throw std::runtime_error("recv: " + errstr);
            }
        }
        else {
            read += ret;
        }

        if (not rtlsdrRunning) {
            break;
        }
    }

    if (firstData) {
        firstData = false;

        // Get dongle information
        ::memcpy(&dongleInfo, buffer.data(), sizeof(dongle_info_t));

        // Convert the byte order
        dongleInfo.tuner_type = ntohl(dongleInfo.tuner_type);
        dongleInfo.tuner_gain_count = ntohl(dongleInfo.tuner_gain_count);

        if(dongleInfo.magic[0] == 'R' &&
                dongleInfo.magic[1] == 'T' &&
                dongleInfo.magic[2] == 'L' &&
                dongleInfo.magic[3] == '0') {
            std::string TunerType;
            switch(dongleInfo.tuner_type)
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
            std::clog << "RTL_TCP_CLIENT: Tuner type: " <<
                dongleInfo.tuner_type << " " << TunerType << std::endl;
            std::clog << "RTL_TCP_CLIENT: Tuner gain count: " <<
                dongleInfo.tuner_gain_count << std::endl;
        }
        else {
            std::clog << "RTL_TCP_CLIENT: Didn't find the \"RTL0\" magic key." <<
                std::endl;
        }
    }

    sampleBuffer.putDataIntoBuffer(buffer.data(), buffer.size());
    spectrumSampleBuffer.putDataIntoBuffer(buffer.data(), buffer.size());

    // Check if device is overloaded
    minAmplitude = 255;
    maxAmplitude = 0;

    for (const auto b : buffer) {
        if (minAmplitude > b)
            minAmplitude = b;
        if (maxAmplitude < b)
            maxAmplitude = b;
    }
}

void CRTL_TCP_Client::handleDisconnect()
{
    connected = false;
    firstData = true;
    radioController.onMessage(message_level_t::Error,
            QT_TRANSLATE_NOOP("CRadioController", "RTL-TCP connection closed."));
    sock.close();
}

void CRTL_TCP_Client::sendCommand(uint8_t cmd, int32_t param)
{
    if (!connected || !sock.valid()) {
        return;
    }

    std::vector<uint8_t> datagram;

    datagram.resize(5);
    datagram[0] = cmd; // command to set rate
    datagram[4] = param & 0xFF;  //lsb last
    datagram[3] = (param >> ONE_BYTE) & 0xFF;
    datagram[2] = (param >> (2 * ONE_BYTE)) & 0xFF;
    datagram[1] = (param >> (3 * ONE_BYTE)) & 0xFF;
    sock.send(datagram.data(), datagram.size(), 0);
}

void CRTL_TCP_Client::sendVFO(int32_t frequency)
{
    sendCommand(0x01, frequency);
}

void CRTL_TCP_Client::sendRate(int32_t theRate)
{
    sendCommand(0x02, theRate);
}

void CRTL_TCP_Client::setGainMode(int32_t gainMode)
{
    sendCommand (0x03, gainMode);
}

float CRTL_TCP_Client::getGain() const
{
    return currentGain;
}

float CRTL_TCP_Client::setGain(int32_t gain)
{
    currentGainCount = gain;
    float gainValue = getGainValue(gain);

    sendCommand(0x04, (int)10 * gainValue);

    currentGain = gainValue;
    return gainValue;
}

int32_t CRTL_TCP_Client::getGainCount()
{
    int32_t MaxGainCount = 0;
    switch (dongleInfo.tuner_type) {
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

//void CRTL_TCP_Client::setHwAgc(bool hwAGC)
//{
//    isHwAGC = hwAGC;
//    sendCommand(0x08, hwAGC ? 1 : 0);
//}

std::string CRTL_TCP_Client::getDescription()
{
    return "rtl_tcp_client (server: " +
        serverAddress + ":" +
        std::to_string(serverPort) + ")";
}

CDeviceID CRTL_TCP_Client::getID()
{
    return CDeviceID::RTL_TCP;
}

void CRTL_TCP_Client::setServerAddress(const std::string& serverAddress)
{
    this->serverAddress = serverAddress;
}

void CRTL_TCP_Client::setPort(uint16_t Port)
{
    serverPort = Port;
}

void CRTL_TCP_Client::receiveAndReconnect()
{
    while (rtlsdrRunning) {
        std::unique_lock<std::mutex> lock(mutex);

        if (!connected) {
            std::clog << "RTL_TCP_CLIENT: Try to connect to server " <<
                serverAddress << ":" << serverPort << std::endl;

            try {
                connected = sock.connect(serverAddress, serverPort, 2);
            }
            catch(const std::runtime_error& e) {
                std::clog << "RTL_TCP_CLIENT: " << e.what() << std::endl;
            }

            if (connected) {
                std::clog << "RTL_TCP_CLIENT: Successful connected to server " <<
                    std::endl;

                // Always use manual gain, the AGC is implemented in software
                setGainMode(1);
                setGain(currentGainCount);
                sendRate(INPUT_RATE);
                sendVFO(frequency);

                if (!agcRunning) {
                    agcRunning = true;
                    agcThread = std::thread(&CRTL_TCP_Client::agcTimer, this);
                }
            }
            else {
                std::clog << "RTL_TCP_CLIENT: Could not connect to server" <<
                    std::endl;

                agcRunning = false;
                rtlsdrRunning = false;
                lock.unlock();

                radioController.onMessage(message_level_t::Error,
                        QT_TRANSLATE_NOOP("CRadioController", "Connection failed to server "),
                        serverAddress + ":" + std::to_string(serverPort));
            }
        }

        if (connected) {
            if (lock.owns_lock()) {
                lock.unlock();
            }

            receiveData();
        }
    }
}

void CRTL_TCP_Client::agcTimer(void)
{
    while (agcRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (isAGC && (dongleInfo.tuner_type != RTLSDR_TUNER_UNKNOWN)) {
            // Check for overloading
            if (minAmplitude == 0 || maxAmplitude == 255) {
                // We have to decrease the gain
                if(currentGainCount > 0) {
                    setGain(currentGainCount - 1);
                    //std::clog << "RTL_TCP_CLIENT: Decrease gain to " << (float)currentGain << std::endl;
                }
            }
            else {
                if (currentGainCount < (getGainCount() - 1)) {
                    // Calc if a gain increase overloads the device. Calc it
                    // from the gain values
                    const float newGain = getGainValue(currentGainCount + 1);
                    const float deltaGain = newGain - currentGain;
                    const float linGain = pow(10, deltaGain / 20);
                    const int newMaxValue = (float)maxAmplitude * linGain;
                    const int newMinValue = (float)minAmplitude / linGain;

                    // We have to increase the gain
                    if (newMinValue >=0 && newMaxValue <= 255) {
                        setGain(currentGainCount + 1);
                        //std::clog << "RTL_TCP_CLIENT: Increase gain to " << currentGain << std::endl;
                    }
                }
            }
        }
        else { // AGC is off or unknown tuner
            if (minAmplitude == 0 || maxAmplitude == 255) {
                std::string text = QT_TRANSLATE_NOOP("CRadioController", "ADC overload."
                    " Maybe you are using a too high gain.");
                std::clog << "RTL_TCP_CLIENT:" << text << std::endl;
                radioController.onMessage(message_level_t::Information, text);
            }
        }
    }
}

float CRTL_TCP_Client::getGainValue(uint16_t gainCount)
{
    float gainValue = 0;

    if (dongleInfo.tuner_type == RTLSDR_TUNER_UNKNOWN)
        return 0;

    // Get max gain count
    uint32_t maxGainCount = getGainCount();
    if (maxGainCount == 0)
        return 0;

    // Check if gainCount is valid
    if (gainCount < maxGainCount) {
        // Get gain
        switch(dongleInfo.tuner_type) {
            case RTLSDR_TUNER_E4000: gainValue = e4k_gains[gainCount]; break;
            case RTLSDR_TUNER_FC0012: gainValue = fc0012_gains[gainCount]; break;
            case RTLSDR_TUNER_FC0013: gainValue = fc0013_gains[gainCount]; break;
            case RTLSDR_TUNER_FC2580: gainValue = fc2580_gains[gainCount]; break;
            case RTLSDR_TUNER_R820T: gainValue = r82xx_gains[gainCount]; break;
            case RTLSDR_TUNER_R828D: gainValue = r82xx_gains[gainCount]; break;
            default: gainValue = 0;
        }
    }
    else {
        gainValue = 999.0; // Max gain
    }

    return gainValue;
}

