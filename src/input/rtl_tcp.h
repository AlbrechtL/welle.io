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

#ifndef __RTL_TCP_CLIENT
#define __RTL_TCP_CLIENT

#include <array>
#include <string>
#include <thread>
#include <mutex>
#include "Socket.h"
#include "virtual_input.h"
#include "dab-constants.h"
#include "MathHelper.h"
#include "ringbuffer.h"
#include "radio-controller.h"

struct dongle_info_t { /* structure size must be multiple of 2 bytes */
    char magic[4];
    uint32_t tuner_type;
    uint32_t tuner_gain_count;
};

class CRTL_TCP_Client : public CVirtualInput {
public:
    CRTL_TCP_Client(RadioControllerInterface& radioController);
    ~CRTL_TCP_Client(void);

    // Interface methods
    void setFrequency(int);
    int getFrequency(void) const;
    bool restart(void);
    bool is_ok(void);
    int32_t getSamples(DSPCOMPLEX* V, int32_t size);
    std::vector<DSPCOMPLEX> getSpectrumSamples(int size);
    int32_t getSamplesToRead(void);
    void reset(void);
    float getGain(void) const;
    float setGain(int gain);
    int getGainCount(void);
    void setAgc(bool AGC);
    std::string getDescription(void);
    CDeviceID getID(void);

    // Specific methods
    void setServerAddress(const std::string& serverAddress);
    void setPort(uint16_t Port);

    RadioControllerInterface& radioController;

private:
    void stop(void);
    void agcTimer(void);
    void receiveData(void);
    void receiveAndReconnect(void);
    void handleDisconnect(void);

    std::mutex mutex;
    Socket sock;
    std::thread receiveThread;
    bool agcRunning = false;
    std::thread agcThread;

    float currentGain = 0;
    uint16_t currentGainCount = 0;
    uint8_t minAmplitude = 255;
    uint8_t maxAmplitude = 0;
    bool isAGC = true;
    bool isHwAGC = false;
    int frequency = kHz(220000);
    RingBuffer<uint8_t> sampleBuffer;
    RingBuffer<uint8_t> spectrumSampleBuffer;
    bool connected = false;
    bool rtlsdrRunning = false;
    std::string serverAddress = "127.0.0.1";
    uint16_t serverPort = 1234;

    bool firstData = true;
    dongle_info_t dongleInfo;

    // Gain values for the different tuners
    const std::array<float, 14> e4k_gains{{-1.0, 1.5, 4.0, 6.5, 9.0, 11.5,
        14.0, 16.5, 19.0, 21.5, 24.0, 29.0, 34.0, 42.0}};

    const std::array<float, 5> fc0012_gains{{ -9.9F, -4.0F, 7.1F, 17.9F, 19.2F }};

    const std::array<float, 23>  fc0013_gains{{ -9.9F, -7.3F, -6.5F, -6.3F, -6.0F,
                       -5.8F, -5.4F, 5.8F, 6.1F, 6.3F, 6.5F, 6.7F, 6.8F, 7.0F, 7.1F, 17.9F, 18.1F, 18.2F,
                       18.4F, 18.6F, 18.8F, 19.1F, 19.7F }};

    const std::array<float, 1>  fc2580_gains{{ 0 /* no gain values */ }};

    const std::array<float, 29>  r82xx_gains{{ 0.0F, 0.9F, 1.4F, 2.7F, 3.7F, 7.7F,
        8.7F, 12.5F, 14.4F, 15.7F, 16.6F, 19.7F, 20.7F, 22.9F, 25.4F, 28.0F, 29.7F, 32.8F,
        33.8F, 36.4F, 37.2F, 38.6F, 40.2F, 42.1F, 43.4F, 43.9F, 44.5F, 48.0F, 49.6F }};

    void sendVFO(int32_t frequency);
    void sendRate(int32_t theRate);
    void setGainMode(int32_t gainMode);
    void sendCommand(uint8_t cmd, int32_t param);
    float getGainValue(uint16_t gainCount);
};

#endif
