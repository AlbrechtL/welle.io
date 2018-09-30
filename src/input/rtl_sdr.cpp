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

#include "rtl_sdr.h"

#define READLEN_DEFAULT 8192

CRTL_SDR::CRTL_SDR(RadioControllerInterface& radioController) :
    radioController(radioController),
    sampleBuffer(1024 * 1024),
    spectrumSampleBuffer(8192)
{
    int ret = 0;

    std::clog << "RTL_SDR:" << "Open rtl-sdr" << std::endl;

    // Get all devices
    uint32_t deviceCount = rtlsdr_get_device_count();
    if (deviceCount == 0) {
        std::clog << "RTL_SDR:" << "No devices found" << std::endl;
        throw 0;
    }
    else {
        std::clog << "RTL_SDR:" << "Found " << deviceCount << " devices. Uses the first working one" << std::endl;
    }

    //	Iterate over all found rtl-sdr devices and try to open it. Stops if one device is successfull opened.
    for(uint32_t i=0; i<deviceCount; i++) {
        ret = rtlsdr_open(&device, i);
        if (ret >= 0) {
            std::clog << "RTL_SDR:" << " Opening rtl-sdr device" << i << std::endl;
            break;
        }
    }

    if (ret < 0) {
        std::clog << "RTL_SDR:" << " Opening rtl-sdr failed" << std::endl;
        throw 0;
    }

    open = true;

    // Set sample rate
    ret = rtlsdr_set_sample_rate(device, INPUT_RATE);
    if (ret < 0) {
        std::clog << "RTL_SDR:" << " Setting sample rate failed" << std::endl;
        throw 0;
    }

    // Get tuner gains
    uint32_t gainsCount = rtlsdr_get_tuner_gains(device, NULL);
    std::clog << "RTL_SDR:" << " Supported gain values" << gainsCount << std::endl;
    gains.resize(gainsCount);
    gainsCount = rtlsdr_get_tuner_gains(device, gains.data());

    for (int i = gainsCount; i > 0; i--) {
        std::clog << "RTL_SDR:" << " gain " << (gains[i - 1] / 10.0) << std::endl;
    }

    // Always use manual gain, the AGC is implemented in software
    rtlsdr_set_tuner_gain_mode(device, 1);

    // Disable hardware AGC by default
    setHwAgc(false);

    // Enable AGC by default
    setAgc(true);
}

CRTL_SDR::~CRTL_SDR(void)
{
    stop();

    if (open)
        rtlsdr_close(device);

    open = false;
}

void CRTL_SDR::setFrequency(int frequency)
{
    lastFrequency = frequency;
    (void)(rtlsdr_set_center_freq(device, frequency + frequencyOffset));
}

int CRTL_SDR::getFrequency(void) const
{
    return lastFrequency;
}

bool CRTL_SDR::restart(void)
{
    int ret;

    if (rtlsdrRunning) {
        return true;
    }

    sampleBuffer.FlushRingBuffer();
    spectrumSampleBuffer.FlushRingBuffer();
    ret = rtlsdr_reset_buffer(device);
    if (ret < 0)
        return false;

    rtlsdr_set_center_freq(device, lastFrequency + frequencyOffset);
    rtlsdrRunning = true;

    rtlsdrThread = std::thread(&CRTL_SDR::rtlsdr_read_async_wrapper, this);
    agcThread = std::thread(&CRTL_SDR::AGCTimer, this);

    return true;
}

void CRTL_SDR::stop(void)
{
    if (not rtlsdrRunning)
        return;

    rtlsdr_cancel_async(device);
    if (rtlsdrThread.joinable()) {
        rtlsdrThread.join();
    }

    if (agcThread.joinable()) {
        agcThread.join();
    }
}

float CRTL_SDR::getGain() const
{
    return currentGain / 10.0;
}

float CRTL_SDR::setGain(int gain_index)
{
    if ((size_t)gain_index >= gains.size()) {
        std::clog << "RTL_SDR:" << "Unknown gain count" << gain_index << std::endl;
        return 0;
    }

    currentGainIndex = gain_index;
    currentGain = gains[gain_index];

    //std::clog << "RTL_SDR:" << "Set gain to" << currentGain / 10.0 << "db" << std::endl;

    int ret = rtlsdr_set_tuner_gain(device, currentGain);
    if (ret != 0) {
        std::clog << "RTL_SDR:" << "Setting gain failed" << std::endl;
    }

    return currentGain / 10.0;
}

int CRTL_SDR::getGainCount()
{
    return gains.size() - 1;
}

void CRTL_SDR::setAgc(bool AGC)
{
    if (AGC == true) {
        isAGC = true;
    }
    else {
        isAGC = false;
        setGain(currentGainIndex);
    }
}

void CRTL_SDR::setHwAgc(bool hwAGC)
{
    isHwAGC = hwAGC;
    rtlsdr_set_agc_mode(device, hwAGC ? 1 : 0);
}

bool CRTL_SDR::isHwAgcSupported() const
{
    return true;
}

std::string CRTL_SDR::getName()
{
    char manufact[256] = {0};
    char product[256] = {0};
    char serial[256] = {0};

    rtlsdr_get_usb_strings(device, manufact, product, serial);

    std::string name;
    name += manufact;
    name += ", ";
    name += product;
    name += ", ";
    name += serial;
    return name;
}

CDeviceID CRTL_SDR::getID()
{
    return CDeviceID::RTL_SDR;
}

void CRTL_SDR::AGCTimer(void)
{
    while (rtlsdrRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (isAGC) {
            // Check for overloading
            if (minAmplitude == 0 || maxAmplitude == 255) {
                // We have to decrease the gain
                if (currentGainIndex > 0) {
                    setGain(currentGainIndex - 1);
                    //std::clog << "RTL_SDR:" << "Decreased gain to " << (float)currentGain / 10.0f << std::endl;
                }
            }
            else {
                if (currentGainIndex < ((ssize_t)gains.size() - 1)) {
                    // Calc if a gain increase overloads the device. Calc it from the gain values
                    int NewGain = gains[currentGainIndex + 1];
                    float DeltaGain = ((float) NewGain / 10) - ((float) currentGain / 10);
                    float LinGain = pow(10, DeltaGain / 20);

                    int NewMaxValue = (float) maxAmplitude * LinGain;
                    int NewMinValue = (float) minAmplitude / LinGain;

                    // We have to increase the gain
                    if(NewMinValue >= 0 && NewMaxValue <= 255) {
                        setGain(currentGainIndex + 1);
                        //std::clog << "RTL_SDR:" << "Increased gain to " << (float) currentGain / 10 << std::endl;
                    }
                }
            }
        }
        else { // AGC is off
            if (minAmplitude == 0 || maxAmplitude == 255) {
                std::string Text = "ADC overload. Maybe you are using a to high gain.";
                std::clog << "RTL_SDR:" << Text << std::endl;
                radioController.onMessage(message_level_t::Information, Text);
            }
        }
    }
}

int32_t CRTL_SDR::getSamples(DSPCOMPLEX *buffer, int32_t size)
{
    std::vector<uint8_t> tempBuffer(2 * size);

    int32_t amount = sampleBuffer.getDataFromBuffer(tempBuffer.data(), 2 * size);

    // Normalise samples
    for (int i = 0; i < amount / 2; i++) {
        buffer[i] = DSPCOMPLEX(
                (float(tempBuffer[2 * i] - 128)) / 128.0,
                (float(tempBuffer[2 * i + 1] - 128)) / 128.0);
    }

    return amount / 2;
}

std::vector<DSPCOMPLEX> CRTL_SDR::getSpectrumSamples(int size)
{
    std::vector<uint8_t> tempBuffer(2 * size);

    // Get samples
    int32_t amount = spectrumSampleBuffer.getDataFromBuffer(
            tempBuffer.data(), 2 * size);

    std::vector<DSPCOMPLEX> buffer(amount / 2);

    // Convert samples into generic format
    for (int i = 0; i < amount / 2; i++) {
        buffer[i] = DSPCOMPLEX(
                (float(tempBuffer[2 * i] - 128)) / 128.0,
                (float(tempBuffer[2 * i + 1] - 128)) / 128.0);
    }

    return buffer;
}

int32_t CRTL_SDR::getSamplesToRead(void)
{
    return sampleBuffer.GetRingBufferReadAvailable() / 2;
}

void CRTL_SDR::reset(void)
{
    sampleBuffer.FlushRingBuffer();
}

void CRTL_SDR::RTLSDRCallBack(uint8_t* buf, uint32_t len, void* ctx)
{
    if (ctx) {
        CRTL_SDR *rtlsdr = (CRTL_SDR*)ctx;

        if (len != READLEN_DEFAULT) {
            std::clog << "Short read" << std::endl;
            return;
        }

        int32_t tmp = rtlsdr->sampleBuffer.putDataIntoBuffer(buf, len);
        if ((len - tmp) > 0)
            rtlsdr->sampleCounter += len - tmp;

        rtlsdr->spectrumSampleBuffer.putDataIntoBuffer(buf, len);

        // Check if device is overloaded
        rtlsdr->minAmplitude = 255;
        rtlsdr->maxAmplitude = 0;

        for (uint32_t i=0;i<len;i++) {
            if (rtlsdr->minAmplitude > buf[i])
                rtlsdr->minAmplitude = buf[i];

            if (rtlsdr->maxAmplitude < buf[i])
                rtlsdr->maxAmplitude = buf[i];
        }
    }
    else {
        std::clog << "ERROR no ctx in RTLSDR callback" << std::endl;
    }
}

void CRTL_SDR::rtlsdr_read_async_wrapper()
{
    std::clog << "Start RTLSDR thread" << std::endl;
    rtlsdr_read_async(device,
                      (rtlsdr_read_async_cb_t)&CRTL_SDR::RTLSDRCallBack,
                      (void*)this, 0, READLEN_DEFAULT);

    radioController.onMessage(message_level_t::Error, "RTL-SDR is unplugged.");

    rtlsdrRunning = false;
}

