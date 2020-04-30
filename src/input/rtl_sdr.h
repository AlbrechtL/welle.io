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

#ifndef _CRTLSDR_H
#define _CRTLSDR_H

#include <vector>
#include <thread>
#include <string>
#include <atomic>
#include <rtl-sdr.h>

#include "virtual_input.h"
#include "dab-constants.h"
#include "MathHelper.h"
#include "ringbuffer.h"
#include "radio-controller.h"

// This class is a simple wrapper around the
// rtlsdr library that is read is as dll
// It does not do any processing
class CRTL_SDR : public CVirtualInput {
public:
    CRTL_SDR(RadioControllerInterface& radioController);
    ~CRTL_SDR(void);
    CRTL_SDR(const CRTL_SDR&) = delete;
    void operator=(const CRTL_SDR&) = delete;

    // Interface methods
    bool restart(void);
    bool is_ok(void);
    void stop(void);
    void reset(void);
    int32_t getSamples(DSPCOMPLEX *buffer, int32_t size);
    std::vector<DSPCOMPLEX> getSpectrumSamples(int size);
    int32_t getSamplesToRead(void);
    void setFrequency(int Frequency);
    int getFrequency(void) const;
    float getGain(void) const;
    float setGain(int gain_index);
    int getGainCount(void);
    void setAgc(bool AGC);
    std::string getDescription(void);
    bool setDeviceParam(DeviceParam param, int value);

    CDeviceID getID(void);

private:
    std::thread agcThread;
    RadioControllerInterface& radioController;
    int frequency = kHz(174928);
    int frequencyOffset = 0;
    int currentGain = 0;
    bool isAGC = false;
    bool isHwAGC = false;
    std::thread rtlsdrThread;
    void rtlsdr_read_async_wrapper(void);
    std::atomic<bool> rtlsdrRunning = ATOMIC_VAR_INIT(false);
    std::atomic<bool> rtlsdrUnplugged = ATOMIC_VAR_INIT(false);

    std::vector<int> gains;
    int currentGainIndex = 0;
    uint8_t minAmplitude = 255;
    uint8_t maxAmplitude = 0;

    void agc_timer_thread(void);

    RingBuffer<uint8_t> sampleBuffer;
    RingBuffer<uint8_t> spectrumSampleBuffer;
    struct rtlsdr_dev *device = nullptr;
    int32_t sampleCounter = 0;

    static void rtlsdr_read_callback(uint8_t* buf, uint32_t len, void *ctx);
    void open_device();
};



#endif
