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

#include <algorithm>
#include <iostream>
#include <chrono>

#include "rtl_tcp.h"
#include "iq_transfer.h"
#include "iq_buffer_pacer.h"

// For Qt translation if Qt is existing
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
    sampleNetworkBuffer(256 * 32768),
    spectrumSampleBuffer(8192)
{
    memset(&dongleInfo, 0, sizeof(dongle_info_t));
    dongleInfo.tuner_type = RTLSDR_TUNER_UNKNOWN;
}

CRTL_TCP_Client::~CRTL_TCP_Client(void)
{
    std::clog << "RTL_TCP_CLIENT: deleting ..." << std::endl;
    stop();
}

void CRTL_TCP_Client::setFrequency(int newFrequency)
{
    frequency = newFrequency;
    {
        std::lock_guard<std::mutex> lock(bufferTransferMutex);
        discardBytes = size_t(settleMs) * INPUT_RATE * 2 / 1000;
    }
    sendVFO(newFrequency);
    agc.reset();
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

    // Threads of a previous run that ended by a failed connection
    joinThreads();

    rtlsdrRunning = true;

    networkBufferThread = std::thread(&CRTL_TCP_Client::networkBufferCopy, this);
    receiveThread = std::thread(&CRTL_TCP_Client::receiveAndReconnect, this);

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

    // Wake up the receive thread, the socket is closed after it is joined
    if (sock.valid()) {
        sock.shutdown();
    }

    lock.unlock();

    // The threads use this object, so they have to end before it is deleted
    joinThreads();

    if (sock.valid()) {
        sock.close();
    }

    connected = false;
}

void CRTL_TCP_Client::joinThreads(void)
{
    // The receive thread starts the AGC thread, so it is joined first
    if (receiveThread.joinable()) {
        receiveThread.join();
    }

    if (agcThread.joinable()) {
        agcThread.join();
    }

    if (networkBufferThread.joinable()) {
        networkBufferThread.join();
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
    std::lock_guard<std::mutex> lock(bufferTransferMutex);
    return read_convert_from_buffer(sampleBuffer, v, size);
}

std::vector<DSPCOMPLEX> CRTL_TCP_Client::getSpectrumSamples(int size)
{
    std::lock_guard<std::mutex> lock(bufferTransferMutex);
    std::vector<DSPCOMPLEX> buffer(size);
    int sizeRead = read_convert_from_buffer(spectrumSampleBuffer, buffer.data(), size);
    if (sizeRead < size) {
        buffer.resize(sizeRead);
    }
    return buffer;
}

int32_t CRTL_TCP_Client::getSamplesToRead(void)
{
    std::lock_guard<std::mutex> lock(bufferTransferMutex);
    return sampleBuffer.GetRingBufferReadAvailable() / 2;
}

void CRTL_TCP_Client::reset(void)
{
    std::lock_guard<std::mutex> lock(bufferTransferMutex);
    ++bufferGeneration;
    sampleBuffer.FlushRingBuffer();
    sampleNetworkBuffer.FlushRingBuffer();
    spectrumSampleBuffer.FlushRingBuffer();
    firstFilledNetworkBuffer = false;
}

void CRTL_TCP_Client::receiveData(void)
{
    std::vector<uint8_t> buffer(8192);

    uint64_t generation;
    {
        std::lock_guard<std::mutex> lock(bufferTransferMutex);
        generation = bufferGeneration;
    }
    size_t read = 0;

    while (sock.valid() && read < buffer.size()) {
        const size_t remain = buffer.size() - read;

        ssize_t ret = sock.recv(buffer.data() + read, remain, 0);

        if (ret == 0) {
            handleDisconnect();
            return;
        }
        else if (ret == -1) {
#if defined(_WIN32)
            if (WSAGetLastError() == WSAEINTR ||
                    WSAGetLastError() == WSAECONNABORTED ||
                    WSAGetLastError() == WSAENOTSOCK) {
                continue;
            }
            else if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAEBADF) {
                handleDisconnect();
            return;
            }
            else {
                std::wstring s;
                int error = WSAGetLastError();
                throw std::runtime_error("RTL_TCP_CLIENT recv error: " +  std::to_string(error));
            }
#else
            if (errno == EAGAIN) {
                continue;
            }
            else if (errno == EINTR) {
                continue;
            }
            else if (errno == ECONNRESET || errno == EBADF) {
                handleDisconnect();
            return;
            }
            else {
                std::string errstr = strerror(errno);
                throw std::runtime_error("RTL_TCP_CLIENT recv error: " + errstr);
            }
#endif
        }
        else {
            read += ret;
        }

        if (not rtlsdrRunning) {
            break;
        }
    }

    if (!connected || !rtlsdrRunning) return;
    // The blocking loop accumulates split reads; disconnect/stop return above.
    // This resize is defensive, not a promise that short blocks are published.
    buffer.resize(read);
    if (firstData && buffer.size() < sizeof(dongle_info_t)) {
        // Fail closed if a future receive-loop change allows an incomplete header.
        handleDisconnect();
        return;
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
            
            // Always use manual gain, the AGC is implemented in software
            setGainMode(1);
            // The digital AGC of the RTL2832 (maybe turned on by a previous
            // client) would change the level behind the software AGC
            sendCommand(0x08, 0);
            // The software AGC starts in the middle of the gain table
            setGain(isAGC ? getGainCount() / 2 : currentGainCount.load());
            agc.reset();
            sendRate(INPUT_RATE);
            sendVFO(frequency);
            // The recognized dongle header is metadata, not six I/Q samples.
            buffer.erase(buffer.begin(), buffer.begin() + sizeof(dongle_info_t));
        }
        else {
            std::clog << "RTL_TCP_CLIENT: Didn't find the \"RTL0\" magic key." <<
                std::endl;
        }
    }

    // Keep received data pending while the network reservoir is full. TCP
    // backpressure is bounded by the sender's own buffers; a reset cancels
    // this old-generation block instead of injecting it after the retune.
    while (rtlsdrRunning && connected) {
        {
            std::lock_guard<std::mutex> lock(bufferTransferMutex);
            if (generation != bufferGeneration) return;
            if (sampleNetworkBuffer.GetRingBufferWriteAvailable() >= int(buffer.size())) {
                // Preserve next's intentional discard of old-frequency samples.
                const size_t skip = std::min(discardBytes.load(), buffer.size());
                discardBytes -= skip;
                sampleNetworkBuffer.putDataIntoBuffer(buffer.data() + skip, buffer.size() - skip);
                if (sampleNetworkBuffer.GetRingBufferReadAvailable() >=
                        sampleNetworkBuffer.GetBufferSize() / 2)
                    firstFilledNetworkBuffer = true;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Keep the upstream software AGC and retune behaviour.
    if (rtlsdrRunning && connected) agc.process(buffer.data(), buffer.size());
}

void CRTL_TCP_Client::handleDisconnect()
{
    connected = false;
    firstData = true;

    // No message if the connection is closed by stop()
    if (rtlsdrRunning) {
        radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController", "RTL-TCP connection closed."));
    }

    std::unique_lock<std::mutex> lock(mutex);
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
    // No SIGPIPE if the socket is shut down by stop() meanwhile
    sock.send(datagram.data(), datagram.size(), MSG_NOSIGNAL);
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

int CRTL_TCP_Client::getGainIndex() const
{
    if (dongleInfo.tuner_type == RTLSDR_TUNER_UNKNOWN)
        return -1;
    return currentGainCount;
}

float CRTL_TCP_Client::setGain(int32_t gain)
{
    currentGainCount = gain;
    float gainValue = getGainValue(gain);

    sendCommand(0x04, (int)10 * gainValue);
    agc.gainChanged();

    currentGain = gainValue;
    return gainValue;
}

std::vector<float> CRTL_TCP_Client::gainTable() const
{
    // The gain count of the server tells which rtl-sdr library it uses
    const uint32_t count = dongleInfo.tuner_gain_count;

    switch (dongleInfo.tuner_type) {
        case RTLSDR_TUNER_E4000:
            if (count == e4k_gains_olddab.size())
                return {e4k_gains_olddab.begin(), e4k_gains_olddab.end()};
            return {e4k_gains.begin(), e4k_gains.end()};
        case RTLSDR_TUNER_FC0012: return {fc0012_gains.begin(), fc0012_gains.end()};
        case RTLSDR_TUNER_FC0013: return {fc0013_gains.begin(), fc0013_gains.end()};
        case RTLSDR_TUNER_FC2580: return {fc2580_gains.begin(), fc2580_gains.end()};
        case RTLSDR_TUNER_R820T:
        case RTLSDR_TUNER_R828D:
            if (count == r82xx_gains_olddab.size())
                return {r82xx_gains_olddab.begin(), r82xx_gains_olddab.end()};
            return {r82xx_gains.begin(), r82xx_gains.end()};
        default: return {};
    }
}

int32_t CRTL_TCP_Client::getGainCount()
{
    const size_t count = gainTable().size();
    // Without a connection it is most likely the R820T tuner
    return count > 0 ? count : 29;
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

        // stop() may have been called while waiting for the lock
        if (!rtlsdrRunning) {
            break;
        }

        if (!connected) {
            int attempt = 0;
            const int maxAttempts = 5;
            while (!connected && rtlsdrRunning && attempt < maxAttempts) {
                attempt++;
                std::clog << "RTL_TCP_CLIENT: Try to connect to server " <<
                    serverAddress << ":" << serverPort;
                if (maxAttempts > 1) {
                    std::clog << " (attempt " << attempt << "/" << maxAttempts << ")";
                }
                std::clog << std::endl;

                try {
                    connected = sock.connect(serverAddress, serverPort, 2);
                }
                catch(const std::runtime_error& e) {
                    std::clog << "RTL_TCP_CLIENT: " << e.what() << std::endl;
                }

                if (connected) {
                    break;
                }

                if (attempt < maxAttempts && rtlsdrRunning) {
                    lock.unlock();
                    for (int s = 0; s < 10 && rtlsdrRunning; ++s) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    lock.lock();
                }
            }

            if (connected) {
                std::clog << "RTL_TCP_CLIENT: Successful connected to server " <<
                    std::endl;

                if (!agcRunning) {
                    if (agcThread.joinable()) {
                        agcThread.join();
                    }
                    agcRunning = true;
                    agcThread = std::thread(&CRTL_TCP_Client::agcTimer, this);
                }
                firstData = true;
                reset(); // Clear buffers
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

#define NETWORK_BUFFER_READ_SAMPLES 32768
void CRTL_TCP_Client::networkBufferCopy()
{
    std::vector<uint8_t> tempBuffer(NETWORK_BUFFER_READ_SAMPLES * 2);
    using Clock = std::chrono::steady_clock;
    IQBufferPacer pacer(2.0 * INPUT_RATE, sampleNetworkBuffer.GetBufferSize());
    auto nextCopy = Clock::now();

    while (rtlsdrRunning) {
        int amount = 0;
        int queuedBytes = 0;
        bool ready = false;
        {
            std::lock_guard<std::mutex> lock(bufferTransferMutex);
            ready = firstFilledNetworkBuffer;
            if (ready) {
                amount = transferIQ(sampleNetworkBuffer, sampleBuffer,
                                    tempBuffer.data(), tempBuffer.size());
                queuedBytes = sampleNetworkBuffer.GetRingBufferReadAvailable();
                if (amount > 0) {
                    spectrumSampleBuffer.putDataIntoBuffer(tempBuffer.data(), amount);
                    putIntoRecordBuffer(*tempBuffer.data(), amount);
                }
            }
        }
        if (!ready || (amount == 0 && queuedBytes < 2)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            nextCopy = Clock::now();
            continue;
        }
        if (amount == 0) {
            // Decoder full: leave the bytes upstream and recheck stop/reset.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            // Do not accumulate an unbounded catch-up burst during a stall.
            nextCopy = Clock::now();
            continue;
        }
        nextCopy += std::chrono::duration_cast<Clock::duration>(
            std::chrono::duration<double>(pacer.secondsFor(amount, queuedBytes)));
        std::this_thread::sleep_until(nextCopy);
    }
}

void CRTL_TCP_Client::agcTimer(void)
{
    while (agcRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (isAGC && (dongleInfo.tuner_type != RTLSDR_TUNER_UNKNOWN)) {
            const int gainCount = currentGainCount;
            const int newGainCount = agc.nextGainIndex(gainTable(), gainCount);
            if (newGainCount != gainCount) {
                setGain(newGainCount);
            }
        }
        else { // AGC is off or unknown tuner
            if (agc.overloaded()) {
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
    if (dongleInfo.tuner_type == RTLSDR_TUNER_UNKNOWN)
        return 0;

    const std::vector<float> gains = gainTable();
    if (gains.empty())
        return 0;

    // Check if gainCount is valid
    if (gainCount < gains.size())
        return gains[gainCount];

    return 999.0; // Max gain
}

