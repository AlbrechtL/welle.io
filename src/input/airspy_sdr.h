/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
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

#include "virtual_input.h"
#include "dab-constants.h"
#include "MathHelper.h"
#include "ringbuffer.h"
#include "rational_resampler.h"
#include "software_agc.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#ifndef __MINGW32__
#include "libairspy/airspy.h"
#else
#include "airspy.h"
#endif

enum class CAirspy_IOCTL {
    SET_BIAS_TEE
};

class CAirspy : public CVirtualInput {
public:
    // Which gain the software AGC and the manual gain control:
    // linearity or sensitivity gain (0..21, LNA, mixer and VGA together),
    // or hybrid: AGC of LNA and mixer in the tuner, VGA gain (0..15)
    enum class GainMode { Linearity = 0, Sensitivity = 1, Hybrid = 2 };

    CAirspy(RadioControllerInterface& radioController);
    ~CAirspy(void);
    CAirspy(const CAirspy&) = delete;
    CAirspy& operator=(const CAirspy&) = delete;

    void setFrequency(int nf);
    int getFrequency(void) const;
    bool restart(void);
    bool is_ok(void);
    void stop(void);
    void reset(void);
    int32_t getSamples(DSPCOMPLEX* Buffer, int32_t Size);
    std::vector<DSPCOMPLEX> getSpectrumSamples(int size);
    int32_t getSamplesToRead(void);
    float getGain(void) const;
    int getGainIndex(void) const;
    float setGain(int gain);
    int getGainCount(void);
    void setAgc(bool agc);
    std::string getDescription(void);
    bool setDeviceParam(DeviceParam param, int value);

    CDeviceID getID(void);
    CRecordFormat recordFormat(void) { return CRecordFormat::CF32; }

private:
    RadioControllerInterface& radioController;

    int sampleRate = 4096000;

    std::atomic<bool> running{false};
    int freq = 0;

    std::atomic<bool> sw_agc{false};
    std::atomic<GainMode> gainMode{GainMode::Linearity};
    std::atomic<int> currentGain{10};   // index in the gain of gainMode
    std::mutex gainMutex;
    std::unique_ptr<SoftwareAgc> agc;
    std::thread agcThread;
    std::unique_ptr<RationalResampler> resampler;
    std::vector<DSPCOMPLEX> resampled;
    RingBuffer<DSPCOMPLEX> SampleBuffer;
    RingBuffer<DSPCOMPLEX> SpectrumSampleBuffer;
    struct airspy_device *device;

    static int callback(airspy_transfer_t*);
    int data_available(const DSPCOMPLEX* buf, size_t num_samples);

    int maxGainIndex() const;
    std::vector<float> gainTableDb() const;
    void applyGain(int index);
    void agcTimerThread();
};

#endif
