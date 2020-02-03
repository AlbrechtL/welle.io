/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *    Albrecht Lohofener (albrechtloh@gmx.de)
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

#pragma once

#include <atomic>
#include <thread>
#include "virtual_input.h"
#include "ringbuffer.h"
#include <SoapySDR/Version.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Device.hpp>

class CSoapySdr_Thread;

class CSoapySdr : public CVirtualInput
{
public:
    CSoapySdr(RadioControllerInterface& radioController);
    ~CSoapySdr();
    CSoapySdr(const CSoapySdr&) = delete;
    CSoapySdr operator=(const CSoapySdr&) = delete;

    virtual void setFrequency(int Frequency);
    virtual int getFrequency(void) const;
    virtual bool restart(void);
    virtual bool is_ok(void);
    virtual void stop(void);
    virtual void reset(void);
    virtual int32_t getSamples(DSPCOMPLEX* Buffer, int32_t Size);
    virtual std::vector<DSPCOMPLEX> getSpectrumSamples(int size);
    virtual int32_t getSamplesToRead(void);
    virtual float setGain(int gainIndex);
    virtual float getGain(void) const;
    virtual int getGainCount(void);
    virtual void setAgc(bool AGC);
    virtual std::string getDescription(void);
    virtual CDeviceID getID(void);
    virtual bool setDeviceParam(DeviceParam param, const std::string& value);

private:
    void setDriverArgs(const std::string& args);
    void setAntenna(const std::string& antenna);
    void setClockSource(const std::string& clock_source);
    void decreaseGain();
    void increaseGain();

    RadioControllerInterface& radioController;
    int m_freq = 0;
    std::string m_driver_args;
    std::string m_antenna;
    std::string m_clock_source;
    SoapySDR::Device *m_device = nullptr;
    std::atomic<bool> m_running = ATOMIC_VAR_INIT(false);
    bool m_sw_agc = false;

    RingBuffer<DSPCOMPLEX> m_sampleBuffer;
    RingBuffer<DSPCOMPLEX> m_spectrumSampleBuffer;

    std::vector<double> m_gains;

    std::thread m_thread;
    void workerthread(void);
    void process(SoapySDR::Stream *stream);
};

