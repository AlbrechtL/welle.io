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
#include "soapy_sdr.h"
#include "dab-constants.h"
#include "unistd.h"

using namespace std;

CSoapySdr::CSoapySdr() :
    m_sampleBuffer(1024 * 1024),
    m_spectrumSampleBuffer(8192)
{
    m_running = false;
    std::clog << "SoapySdr" << std::endl;

    restart();
}

CSoapySdr::~CSoapySdr()
{
    stop();
}

void CSoapySdr::setFrequency(int Frequency)
{
    m_freq = Frequency;
    if (m_device != nullptr) {
        m_device->setFrequency(SOAPY_SDR_RX, 0, Frequency);
        m_freq = m_device->getFrequency(SOAPY_SDR_RX, 0);
        std::clog << "OutputSoapySDR:Actual frequency: " <<
            m_freq / 1000.0 <<
            " kHz." << std::endl;
    }
}

int CSoapySdr::getFrequency() const
{
    return m_freq;
}

bool CSoapySdr::restart()
{
    if (m_running) {
        return true;
    }

    m_sampleBuffer.FlushRingBuffer();
    m_spectrumSampleBuffer.FlushRingBuffer();

    m_device = SoapySDR::Device::make("driver=uhd");
    stringstream ss;
    ss << "SoapySDR driver=" << m_device->getDriverKey();
    ss << " hardware=" << m_device->getHardwareKey();
    for (const auto &it : m_device->getHardwareInfo())
    {
        ss << "  " << it.first << "=" << it.second;
    }
    std::clog << ss.str().c_str() << std::endl;

    m_device->setMasterClockRate(INPUT_RATE*16);
    std::clog << "SoapySDR master clock rate set to " <<
        m_device->getMasterClockRate()/1000.0 << " kHz" << std::endl;

    m_device->setSampleRate(SOAPY_SDR_RX, 0, INPUT_RATE);
    std::clog << "OutputSoapySDR:Actual RX rate: " <<
        m_device->getSampleRate(SOAPY_SDR_RX, 0) / 1000.0 <<
        " ksps." << std::endl;


    clog << "Supported antenna: ";
    for (const auto& ant : m_device->listAntennas(SOAPY_SDR_RX, 0)) {
        clog << " " << ant;
    }
    clog << endl;

    if (!m_antenna.empty()) {
        clog << "Select antenna " << m_antenna << endl;
        m_device->setAntenna(SOAPY_SDR_RX, 0, m_antenna);
    }
    else {
        clog << "Not selecting antenna" << endl;
    }

    if (!m_clock_source.empty()) {
        m_device->setClockSource(m_clock_source);
    }

    if (m_freq > 0) {
        setFrequency(m_freq);
    }

    auto range = m_device->getGainRange(SOAPY_SDR_RX, 0);
    // Ignore step size, as it could be 0. 1dB steps are ok
    for (double g = range.minimum(); g < range.maximum(); g++) {
        m_gains.push_back(g);
    }

    const bool automatic = false;
    m_device->setGainMode(SOAPY_SDR_RX, 0, automatic);

    if (m_device->hasDCOffsetMode(SOAPY_SDR_RX, 0)) {
        m_device->setDCOffsetMode(SOAPY_SDR_RX, 0, true);
    }
    else {
        clog << "DC offset compensation not supported" << endl;
    }

    m_running = true;
    m_thread = std::thread(&CSoapySdr::workerthread, this);

    return true;
}

bool CSoapySdr::is_ok()
{
    return m_running;
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

std::vector<DSPCOMPLEX> CSoapySdr::getSpectrumSamples(int size)
{
    std::vector<DSPCOMPLEX> sampleBuffer(size);
    int32_t amount = m_spectrumSampleBuffer.getDataFromBuffer(sampleBuffer.data(), size);
    if (amount < size) {
        sampleBuffer.resize(amount);
    }
    return sampleBuffer;
}

int32_t CSoapySdr::getSamplesToRead()
{
    return m_sampleBuffer.GetRingBufferReadAvailable();
}

float CSoapySdr::getGain() const
{
    if (m_device != nullptr) {
        float g = m_device->getGain(SOAPY_SDR_RX, 0);
        return g;
    }
    else {
        return 0;
    }
}

float CSoapySdr::setGain(int32_t gainIndex)
{
    if (m_device != nullptr) {
        try {
            m_device->setGain(SOAPY_SDR_RX, 0, m_gains.at(gainIndex));
        }
        catch (const out_of_range&) {
            std::clog << "Soapy gain " << gainIndex << " is out of range" << std::endl;
        }
        float g = m_device->getGain(SOAPY_SDR_RX, 0);
        std::clog << "Soapy gain is " << g << std::endl;
        return g;
    }
    return 0;
}

void CSoapySdr::setDriverArgs(const std::string& args)
{
    m_driver_args = args;
}

void CSoapySdr::setAntenna(const std::string& antenna)
{
    m_antenna = antenna;
    if (!m_antenna.empty() && m_device != nullptr) {
        clog << "Select antenna " << m_antenna << ", supported: ";
        for (const auto& ant : m_device->listAntennas(SOAPY_SDR_RX, 0)) {
            clog << " " << ant;
        }
        clog << endl;

        m_device->setAntenna(SOAPY_SDR_RX, 0, m_antenna);
    }
}

void CSoapySdr::setClockSource(const std::string& clock_source)
{
    m_clock_source = clock_source;
}


void CSoapySdr::increaseGain()
{
    if (m_device != nullptr) {
        float current_gain = m_device->getGain(SOAPY_SDR_RX, 0);
        for (const float g : m_gains) {
            if (g > current_gain) {
                m_device->setGain(SOAPY_SDR_RX, 0, g);
                break;
            }
        }
    }
}

void CSoapySdr::decreaseGain()
{
    if (m_device != nullptr) {
        float current_gain = m_device->getGain(SOAPY_SDR_RX, 0);
        for (auto it = m_gains.rbegin(); it != m_gains.rend(); ++it) {
            if (*it > current_gain) {
                m_device->setGain(SOAPY_SDR_RX, 0, *it);
                break;
            }
        }
    }
}

int32_t CSoapySdr::getGainCount()
{
    return m_gains.size();
}

void CSoapySdr::setAgc(bool AGC)
{
    m_sw_agc = AGC;
}

//void CSoapySdr::setHwAgc(bool hwAGC)
//{
//    if (m_device != nullptr) {
//        m_device->setGainMode(SOAPY_SDR_RX, 0, hwAGC);
//    }
//}

std::string CSoapySdr::getDescription()
{
    return "SoapySDR (" + m_device->getDriverKey() + ")";
}

CDeviceID CSoapySdr::getID()
{
    return CDeviceID::SOAPYSDR;
}

bool CSoapySdr::setDeviceParam(DeviceParam param, std::string &value)
{
    switch(param) {
        case DeviceParam::SoapySDRAntenna: setAntenna(value); return true;
        case DeviceParam::SoapySDRClockSource: setClockSource(value); return true;
        case DeviceParam::SoapySDRDriverArgs: setDriverArgs(value); return true;
        default: std::runtime_error("Unsupported device parameter");
    }

    return false;
}

void CSoapySdr::workerthread()
{
    std::vector<size_t> channels;
    channels.push_back(0);
    auto device = m_device;
    std::clog << " *************** Setup soapy stream" << std::endl;
    auto stream = device->setupStream(SOAPY_SDR_RX, "CF32", channels);
//    assert(stream != nullptr);

    device->activateStream(stream);
    try {
        process(stream);
    }
    catch (std::exception& e) {
        std::clog << " *************** Exception caught in soapy: " << e.what() << std::endl;
    }

    std::clog << " *************** Close soapy stream" << std::endl;
    device->closeStream(stream);
    m_running = false;
}

void CSoapySdr::process(SoapySDR::Stream *stream)
{
    size_t frames = 0;
    while (m_running) {
        frames++;

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
                SoapySDR::errToStr(ret) << std::endl;
            m_running = false;
        }
        else {
            buf.resize(ret);

            if (m_sw_agc and (frames % 200) == 0) {
                float maxnorm = 0;
                for (const DSPCOMPLEX& z : buf) {
                    if (norm(z) > maxnorm) {
                        maxnorm = norm(z);
                    }
                }

                const float maxampl = sqrt(maxnorm);

                if (maxampl > 0.5f) {
                    decreaseGain();
                }
                else if (maxampl < 0.1f) {
                    increaseGain();
                }
            }

            m_sampleBuffer.putDataIntoBuffer(buf.data(), ret);
            m_spectrumSampleBuffer.putDataIntoBuffer(buf.data(), ret);
        }
    }
}
