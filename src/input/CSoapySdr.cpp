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

#include <vector>
#include <sstream>
#include <iostream>
#include <cassert>
#include <SoapySDR/Errors.hpp>
#include "CSoapySdr.h"
#include "DabConstants.h"
#include "unistd.h"

using namespace std;

CSoapySdr::CSoapySdr() :
    m_sampleBuffer(1024 * 1024),
    m_spectrumSampleBuffer(8192)
{
    m_running = false;
    std::clog << "SoapySdr";
}

CSoapySdr::~CSoapySdr()
{
    stop();
}


void CSoapySdr::setDriverArgs(const std::string& args)
{
    m_driver_args = args;
}

void CSoapySdr::setAntenna(const std::string& antenna)
{
    m_antenna = antenna;
}

void CSoapySdr::setClockSource(const std::string& clock_source)
{
    m_clock_source = clock_source;
}

void CSoapySdr::setFrequency(int32_t Frequency)
{
    m_freq = Frequency;
    if (m_device != nullptr) {
        m_device->setFrequency(SOAPY_SDR_RX, 0, Frequency);
        Frequency = m_device->getFrequency(SOAPY_SDR_RX, 0);
        std::clog << "OutputSoapySDR:Actual frequency: " <<
            Frequency / 1000.0 <<
            " kHz.";
    }
}

bool CSoapySdr::restart()
{
    if (m_running) {
        return true;
    }

    m_sampleBuffer.FlushRingBuffer();
    m_spectrumSampleBuffer.FlushRingBuffer();

    m_device = SoapySDR::Device::make(m_driver_args);
    stringstream ss;
    ss << "SoapySDR driver=" << m_device->getDriverKey();
    ss << " hardware=" << m_device->getHardwareKey();
    for (const auto &it : m_device->getHardwareInfo())
    {
        ss << "  " << it.first << "=" << it.second;
    }
    std::clog << ss.str().c_str();

    m_device->setMasterClockRate(INPUT_RATE*16);
    std::clog << "SoapySDR master clock rate set to " <<
        m_device->getMasterClockRate()/1000.0 << " kHz";

    m_device->setSampleRate(SOAPY_SDR_RX, 0, INPUT_RATE);
    std::clog << "OutputSoapySDR:Actual RX rate: " <<
        m_device->getSampleRate(SOAPY_SDR_RX, 0) / 1000.0 <<
        " ksps.";

    if (!m_antenna.empty())
        m_device->setAntenna(SOAPY_SDR_RX, 0, m_antenna);

    if (!m_clock_source.empty())
        m_device->setClockSource(m_clock_source);

    if (m_freq > 0) {
        setFrequency(m_freq);
    }

    const bool automatic = true;
    m_device->setGainMode(SOAPY_SDR_RX, 0, automatic);

    m_running = true;
    m_thread = std::thread(&CSoapySdr::workerthread, this);

    return true;
}

void CSoapySdr::stop()
{
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }

    if (m_device != nullptr) {
        SoapySDR::Device::unmake(m_device);
        m_device = nullptr;
    }
}

void CSoapySdr::reset()
{
    m_sampleBuffer.FlushRingBuffer();
}

int32_t CSoapySdr::getSamples(DSPCOMPLEX *Buffer, int32_t Size)
{
    int32_t amount = m_sampleBuffer.getDataFromBuffer(Buffer, Size);
    return amount;
}

int32_t CSoapySdr::getSpectrumSamples(DSPCOMPLEX *Buffer, int32_t Size)
{
    int32_t amount = m_spectrumSampleBuffer.getDataFromBuffer(Buffer, Size);

    return amount;
}

int32_t CSoapySdr::getSamplesToRead()
{
    return m_sampleBuffer.GetRingBufferReadAvailable();
}

float CSoapySdr::setGain(int32_t Gain)
{
    if (m_device != nullptr) {
        m_device->setGain(SOAPY_SDR_RX, 0, Gain);
        float g = m_device->getGain(SOAPY_SDR_RX, 0);
        std::clog << "Soapy gain is " << g;
        return g;
    }
    return 0;
}

int32_t CSoapySdr::getGainCount()
{
    return 100;
}

void CSoapySdr::setAgc(bool AGC)
{
    (void) AGC;
}

void CSoapySdr::setHwAgc(bool hwAGC)
{
    (void) hwAGC;
}

std::string CSoapySdr::getName()
{
    return "SoapySDR";
}

CDeviceID CSoapySdr::getID()
{
    return CDeviceID::SOAPYSDR;
}

void CSoapySdr::workerthread()
{
    std::vector<size_t> channels;
    channels.push_back(0);
    auto device = m_device;
    std::clog << " *************** Setup soapy stream";
    auto stream = device->setupStream(SOAPY_SDR_RX, "CF32", channels);
    assert(stream != nullptr);

    device->activateStream(stream);
    try {
        process(stream);
    }
    catch (std::exception& e) {
        std::clog << " *************** Exception caught in soapy: " << e.what();
    }

    std::clog << " *************** Close soapy stream";
    device->closeStream(stream);
    m_running = false;
}

void CSoapySdr::process(SoapySDR::Stream *stream)
{
    while (m_running) {
        // Stream MTU is in samples, not bytes.
        const size_t mtu = m_device->getStreamMTU(stream);

        const size_t samps_to_read = mtu; // Always read MTU samples
        std::vector<DSPCOMPLEX> buf(samps_to_read);

        void *buffs[1];
        buffs[0] = buf.data();

        int flags = 0;
        long long timeNs = 0;
        assert(m_device != nullptr);
        int ret = m_device->readStream(
                stream, buffs, samps_to_read, flags, timeNs);

        if (ret == SOAPY_SDR_TIMEOUT) {
            continue;
        }
        else if (ret == SOAPY_SDR_OVERFLOW) {
            continue;
        }
        else if (ret == SOAPY_SDR_UNDERFLOW) {
            continue;
        }

        if (ret < 0) {
            std::clog << " *************** Unexpected stream error " <<
                SoapySDR::errToStr(ret);
            m_running = false;
        }
        else {
            buf.resize(ret);

            m_sampleBuffer.putDataIntoBuffer(buf.data(), ret);
            m_spectrumSampleBuffer.putDataIntoBuffer(buf.data(), ret);
        }
    }
}
