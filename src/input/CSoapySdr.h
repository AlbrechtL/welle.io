/*
 *    Copyright (C) 2017
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

#include <QThread>
#include <atomic>
#include <vector>
#include <deque>
#include <mutex>
#include "CVirtualInput.h"
#include "ringbuffer.h"
#include <SoapySDR/Version.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Device.hpp>

class CSoapySdr_Thread;

class CSoapySdr : public CVirtualInput
{
public:
    CSoapySdr();
    ~CSoapySdr();
    CSoapySdr(const CSoapySdr&) = delete;
    CSoapySdr operator=(const CSoapySdr&) = delete;

    virtual void setFrequency(int32_t Frequency);
    virtual bool restart(void);
    virtual void stop(void);
    virtual void reset(void);
    virtual int32_t getSamples(DSPCOMPLEX* Buffer, int32_t Size);
    virtual int32_t getSpectrumSamples(DSPCOMPLEX* Buffer, int32_t Size);
    virtual int32_t getSamplesToRead(void);
    virtual float setGain(int32_t Gain);
    virtual int32_t getGainCount(void);
    virtual void setAgc(bool AGC);
    virtual void setHwAgc(bool hwAGC);
    virtual QString getName(void);
    virtual CDeviceID getID(void);
    virtual void setDriverArgs(QString args);
    virtual void setAntenna(QString antenna);
    virtual void setClockSource(QString clock_source);
private:
    friend class CSoapySdr_Thread;

    int32_t m_freq = 0;
    QString m_driver_args;
    QString m_antenna;
    QString m_clock_source;
    SoapySDR::Device *m_device = nullptr;
    std::atomic<bool> m_running;
    CSoapySdr_Thread *m_thread = nullptr;

    RingBuffer<DSPCOMPLEX> m_sampleBuffer;
    RingBuffer<DSPCOMPLEX> m_spectrumSampleBuffer;

    void workerthread(void);
};

class CSoapySdr_Thread : public QThread {
public:
    CSoapySdr_Thread(CSoapySdr *soapySdr);
    ~CSoapySdr_Thread(void);

private:
    virtual void run(void);
    virtual void process(SoapySDR::Stream *stream);

    CSoapySdr* soapySdr;
};

