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

#ifndef _DABSTICK
#define _DABSTICK

#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QThread>
#include <rtl-sdr.h>

#include "CVirtualInput.h"
#include "DabConstants.h"
#include "MathHelper.h"
#include "ringbuffer.h"
#include "CRadioController.h"

class CRTL_SDR_Thread;

//	This class is a simple wrapper around the
//	rtlsdr library that is read is as dll
//	It does not do any processing
class CRTL_SDR : public CVirtualInput {
    Q_OBJECT
public:
    CRTL_SDR(CRadioController &RadioController, int fd = -1, QString path = "");
    ~CRTL_SDR(void);

    // Interface methods
    bool restart(void);
    void stop(void);
    void reset(void);
    int32_t getSamples(DSPCOMPLEX* SampleBuffer, int32_t Size);
    int32_t getSpectrumSamples(DSPCOMPLEX* SampleBuffer, int32_t Size);
    int32_t getSamplesToRead(void);
    void setFrequency(int32_t Frequency);
    float setGain(int32_t Gain);
    int32_t getGainCount(void);
    void setAgc(bool AGC);
    void setHwAgc(bool hwAGC);
    QString getName(void);
    CDeviceID getID(void);

    // Specific methods
    void setMinMaxValue(uint8_t MinValue, uint8_t MaxValue);
    CRadioController *getRadioController(void);

    //	These need to be visible for the separate usb handling thread
    RingBuffer<uint8_t>* SampleBuffer;
    RingBuffer<uint8_t>* SpectrumSampleBuffer;
    struct rtlsdr_dev* device;
    int32_t sampleCounter;

private:
    QTimer AGCTimer;
    CRadioController *RadioController;
    int32_t lastFrequency;
    int32_t FrequencyOffset;
    int CurrentGain;
    bool isAGC;
    bool isHwAGC;
    int32_t DeviceCount;
    CRTL_SDR_Thread* RTL_SDR_Thread;
    bool open;
    int* gains;
    uint16_t GainsCount;
    uint16_t CurrentGainCount;
    uint8_t MinValue;
    uint8_t MaxValue;

private slots:
    void AGCTimerTimeout(void);

};

//	for handling the events in libusb, we need a controlthread
//	whose sole purpose is to process the rtlsdr_read_async function
//	from the lib.
class CRTL_SDR_Thread : public QThread {
public:
    CRTL_SDR_Thread(CRTL_SDR* RTL_SDR);
    ~CRTL_SDR_Thread(void);

private:
    virtual void run(void);

    CRTL_SDR* RTL_SDR;
};

#endif
