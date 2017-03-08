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

#include <QFrame>
#include <QObject>
#include <QSettings>

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
    Q_OBJECT
public:
    CAirspy();
    ~CAirspy(void);

    void setFrequency(int32_t nf);
    bool restart(void);
    void stop(void);
    void reset(void);
    int32_t getSamples(DSPCOMPLEX* Buffer, int32_t Size);
    int32_t getSpectrumSamples(DSPCOMPLEX* Buffer, int32_t Size);
    int32_t getSamplesToRead(void);
    float setGain(int32_t gain);
    int32_t getGainCount(void);
    void setAgc(bool AGC);
    QString getName(void);
    CDeviceID getID(void);

private:
    bool libraryLoaded;
    bool success;
    bool running;

    bool isAGC;
    int8_t currentLinearityGain;
    int32_t selectedRate;
    DSPCOMPLEX* convBuffer;
    int16_t convBufferSize;
    int16_t convIndex;
    int16_t mapTable_int[4 * 512];
    float mapTable_float[4 * 512];
    RingBuffer<DSPCOMPLEX>* SampleBuffer;
    RingBuffer<DSPCOMPLEX>* SpectrumSampleBuffer;
    int32_t inputRate;
    struct airspy_device* device;
    uint64_t serialNumber;
    char serial[128];
    // callback buffer
    int bs_;
    uint8_t* buffer;
    int bl_;
    static int callback(airspy_transfer_t*);
    int data_available(void* buf, int buf_size);
};

#endif
