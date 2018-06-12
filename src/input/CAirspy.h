/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright 2015 by Andrea Montefusco IW0HDV
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
 *    You should have received a copy of the GNU General Public License 3.0+
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
*/
#ifndef __AIRSPY_RADIO__
#define __AIRSPY_RADIO__

#include "CVirtualInput.h"
#include "dab-constants.h"
#include "MathHelper.h"
#include "ringbuffer.h"

#ifndef __MINGW32__
#include "libairspy/airspy.h"
#else
#include "airspy.h"
#endif

class CAirspy : public CVirtualInput {
public:
    CAirspy();
    ~CAirspy(void);

    void setFrequency(int nf);
    int getFrequency(void) const;
    bool restart(void);
    void stop(void);
    void reset(void);
    int32_t getSamples(DSPCOMPLEX* Buffer, int32_t Size);
    std::vector<DSPCOMPLEX> getSpectrumSamples(int size);
    int32_t getSamplesToRead(void);
    float setGain(int gain);
    int getGainCount(void);
    void setAgc(bool AGC);
    void setHwAgc(bool hwAGC);
    std::string getName(void);
    CDeviceID getID(void);

private:
    bool running = false;
    int freq = 0;

    bool isAGC = true;
    int8_t currentLinearityGain = 0;
    int32_t selectedRate = 0;
    DSPCOMPLEX* convBuffer;
    int16_t convBufferSize = 0;
    int16_t convIndex = 0;
    int16_t mapTable_int[4 * 512];
    float mapTable_float[4 * 512];
    RingBuffer<DSPCOMPLEX> SampleBuffer;
    RingBuffer<DSPCOMPLEX> SpectrumSampleBuffer;
    int32_t inputRate;
    struct airspy_device *device;
    uint64_t serialNumber = 0;
    char serial[128];
    // callback buffer
    int bs_ = 0;
    uint8_t *buffer;
    int bl_ = 0;

    static int callback(airspy_transfer_t*);
    int data_available(void* buf, int buf_size);
};

#endif
