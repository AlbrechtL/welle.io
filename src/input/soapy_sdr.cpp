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

#include <cmath>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iostream>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <SoapySDR/Errors.hpp>
#include "soapy_sdr.h"
#include "dab-constants.h"
#include "unistd.h"

// For Qt translation if Qt is existing
#ifdef QT_CORE_LIB
    #include <QtGlobal>
#else
    #define QT_TRANSLATE_NOOP(x,y) (y)
#endif

using namespace std;

CSoapySdr::CSoapySdr(RadioControllerInterface& radioController) :
    radioController(radioController),
    m_sampleBuffer(1024 * 1024),
    m_spectrumSampleBuffer(8192)
{
    //enumerate devices
    const std::string args ="";
    const auto foundDevices = SoapySDR::Device::enumerate("");

    int nonAudioDeviceCount = 0;
    for(auto found: foundDevices)
    {
        std::clog << "SoapySDR: Found device \"" << found.at("driver") << " \'" << found.at("label") << "\' \"" << std::endl;

        if(found.at("driver").compare("audio") != 0)
            nonAudioDeviceCount++;
    }

    if(nonAudioDeviceCount == 0)
    {
        std::clog << "SoapySDR: " << "No usable SDR device found" << std::endl;
        throw 0;
    }
}

CSoapySdr::~CSoapySdr()
{
    stop();
}

void CSoapySdr::setFrequency(int Frequency)
{
    m_agc.reset();
    m_freq = Frequency;
    if (m_device != nullptr) {
        m_device->setFrequency(SOAPY_SDR_RX, 0, Frequency);
        m_freq = m_device->getFrequency(SOAPY_SDR_RX, 0);
        std::clog << "SoapySDR:Actual frequency: " <<
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

    // An ended worker remains joinable, and still owns its device.
    stop();
    reset();

    try {
        m_device = SoapySDR::Device::make(m_driver_args);
        if (m_device == nullptr)
            throw std::runtime_error("SoapySDR returned no device");
    }
    catch (std::exception& e) {
        std::clog << "Exception caught in SoapySDR::Device::make with \"" << m_driver_args << "\" :\n" <<
            "   " << e.what() << std::endl;
        stop();
        radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController",
                    "Could not load SoapySDR with provided device arguments."));
        return false;
    }
    try {
        return configureAndStart();
    }
    catch (const std::exception& e) {
        std::clog << "SoapySDR device initialization failed: " << e.what() << std::endl;
        stop();
        m_gains.clear();
        radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController",
                    "Could not load SoapySDR with provided device arguments."));
        return false;
    }
}

bool CSoapySdr::configureAndStart()
{
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
    std::clog << "SoapySDR:Actual RX rate: " <<
        m_device->getSampleRate(SOAPY_SDR_RX, 0) / 1000.0 <<
        " ksps." << std::endl;


    clog << "Supported antenna: ";
    for (const auto& ant : m_device->listAntennas(SOAPY_SDR_RX, 0)) {
        clog << " " << ant;
    }
    clog << endl;

    if (!m_antenna.empty()) {
        clog << "Select antenna " << m_antenna << endl;
        try {
            m_device->setAntenna(SOAPY_SDR_RX, 0, m_antenna);
        }
        catch (const runtime_error& e) {
            clog << "ERROR! could not select antenna:" << e.what() << endl;
        }
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

    try {
        updateGains();
    }
    catch (const std::exception& e) {
        std::clog << "SoapySDR gain range rejected: " << e.what() << std::endl;
        stop();
        radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController",
                    "The SoapySDR device reported an invalid gain range."));
        return false;
    }

    try {
        const bool automatic = false;
        m_device->setGainMode(SOAPY_SDR_RX, 0, automatic);
        if (m_requestedGain >= 0 && !m_sw_agc) {
            // Validate the stored index before calling the driver. Its own
            // out_of_range exception must be treated as a startup failure.
            if (static_cast<size_t>(m_requestedGain) < m_gains.size()) {
                m_device->setGain(SOAPY_SDR_RX, 0, m_gains.at(m_requestedGain));
                std::clog << "Soapy gain is " <<
                    m_device->getGain(SOAPY_SDR_RX, 0) << std::endl;
            }
            else {
                std::clog << "Soapy gain " << m_requestedGain << " is out of range" << std::endl;
                m_requestedGain = -1;
            }
        }
    }
    catch (const std::exception& e) {
        std::clog << "SoapySDR gain setup failed: " << e.what() << std::endl;
        stop();
        m_gains.clear();
        radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController",
                    "The SoapySDR device could not apply its gain settings."));
        return false;
    }

    if (m_device->hasDCOffsetMode(SOAPY_SDR_RX, 0)) {
        m_device->setDCOffsetMode(SOAPY_SDR_RX, 0, true);
    }
    else {
        clog << "DC offset compensation not supported" << endl;
    }

    m_agc.reset();
    m_running = true;
    try {
        m_thread = std::thread(&CSoapySdr::workerthread, this);
    }
    catch (const std::exception& e) {
        std::clog << "SoapySDR worker startup failed: " << e.what() << std::endl;
        stop();
        return false;
    }

    return true;
}

void CSoapySdr::updateGains()
{
    m_gains.clear();
    const auto range = m_device->getGainRange(SOAPY_SDR_RX, 0);
    const double minimum = range.minimum();
    const double maximum = range.maximum();
    const double step = range.step() == 0.0 ? 1.0 : range.step();
    if (!std::isfinite(minimum) || !std::isfinite(maximum) ||
        !std::isfinite(step) || step <= 0.0 || minimum > maximum) {
        throw std::runtime_error("Invalid SoapySDR gain range");
    }

    // Bound memory and work even for a tiny but representable driver step.
    // Build privately so a rejected range leaves no partial gain table.
    constexpr size_t maxGainEntries = 4096;
    std::vector<double> gains;

    // A zero step denotes an unspecified step size: retain the 1 dB policy.
    // Calculate from the minimum to avoid accumulating rounding error, and
    // include the driver's upper endpoint without issuing an out-of-range gain.
    const double tolerance = 8 * std::numeric_limits<double>::epsilon() *
        std::max(1.0, std::max(std::abs(minimum), std::abs(maximum)));
    for (size_t i = 0; ; ++i) {
        const double gain = std::fma(static_cast<double>(i), step, minimum);
        if (gain >= maximum || (i > 0 && maximum - gain <= tolerance))
            break;
        // Reserve one entry for the maximum, including off-grid endpoints.
        if (gains.size() >= maxGainEntries - 1)
            throw std::runtime_error("SoapySDR gain table is too large");
        if (!gains.empty() && gain <= gains.back())
            throw std::runtime_error("SoapySDR gain step is too small");
        gains.push_back(gain);
    }
    gains.push_back(maximum);
    m_gains.swap(gains);
}

bool CSoapySdr::is_ok()
{
    return m_running;
}

void CSoapySdr::stop()
{
    {
        std::lock_guard<std::mutex> lock(m_sampleMutex);
        m_running = false;
        ++m_sampleGeneration;
    }
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
    std::lock_guard<std::mutex> lock(m_sampleMutex);
    ++m_sampleGeneration;
    m_sampleBuffer.FlushRingBuffer();
    m_spectrumSampleBuffer.FlushRingBuffer();
}

int32_t CSoapySdr::getSamples(DSPCOMPLEX *Buffer, int32_t Size)
{
    std::lock_guard<std::mutex> lock(m_sampleMutex);
    int32_t amount = m_sampleBuffer.getDataFromBuffer(Buffer, Size);
    return amount;
}

std::vector<DSPCOMPLEX> CSoapySdr::getSpectrumSamples(int size)
{
    std::vector<DSPCOMPLEX> sampleBuffer(size);
    std::lock_guard<std::mutex> lock(m_sampleMutex);
    int32_t amount = m_spectrumSampleBuffer.getDataFromBuffer(sampleBuffer.data(), size);
    if (amount < size) {
        sampleBuffer.resize(amount);
    }
    return sampleBuffer;
}

int32_t CSoapySdr::getSamplesToRead()
{
    std::lock_guard<std::mutex> lock(m_sampleMutex);
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

int CSoapySdr::getGainIndex() const
{
    if (m_device == nullptr || m_gains.empty())
        return -1;

    // The AGC sets the gains of m_gains, the device may round them
    const double gain = m_device->getGain(SOAPY_SDR_RX, 0);
    size_t index = 0;
    for (size_t i = 1; i < m_gains.size(); i++) {
        if (std::abs(m_gains[i] - gain) < std::abs(m_gains[index] - gain))
            index = i;
    }
    return int(index);
}

float CSoapySdr::setGain(int32_t gainIndex)
{
    if (m_device != nullptr) {
        try {
            m_device->setGain(SOAPY_SDR_RX, 0, m_gains.at(gainIndex));
            m_requestedGain = gainIndex;
            m_agc.gainChanged();
        }
        catch (const out_of_range&) {
            std::clog << "Soapy gain " << gainIndex << " is out of range" << std::endl;
        }
        float g = m_device->getGain(SOAPY_SDR_RX, 0);
        std::clog << "Soapy gain is " << g << std::endl;
        return g;
    }
    if (gainIndex >= 0)
        m_requestedGain = gainIndex;
    return 0;
}

void CSoapySdr::setDriverArgs(const std::string& args)
{
    if (m_driver_args != args) {
        m_driver_args = args;
        m_running = false;
        stop();
        restart();
    }
}

void CSoapySdr::setAntenna(const std::string& antenna)
{
    if (!antenna.empty() && m_device != nullptr) {
        clog << "Select antenna " << antenna << ", supported: ";
        for (const auto& ant : m_device->listAntennas(SOAPY_SDR_RX, 0)) {
            clog << " " << ant;
        }
        clog << endl;

        try {
            m_device->setAntenna(SOAPY_SDR_RX, 0, antenna);
            m_antenna = antenna;
        }
        catch (...) {
            clog << "Could not set antenna to " << antenna << endl;
        }
    }
    else if (m_device == nullptr) {
        m_antenna = antenna;
    }
}

void CSoapySdr::setClockSource(const std::string& clock_source)
{
    m_clock_source = clock_source;
}


int32_t CSoapySdr::getGainCount()
{
    return m_gains.size();
}

void CSoapySdr::setAgc(bool AGC)
{
    if (m_sw_agc.exchange(AGC) != AGC) {
        m_agc.reset();
        // Preserve a GUI off/on transition even if both happen between reads.
        m_agcRetryRequested = true;
    }
}

//void CSoapySdr::setHwAgc(bool hwAGC)
//{
//    if (m_device != nullptr) {
//        m_device->setGainMode(SOAPY_SDR_RX, 0, hwAGC);
//    }
//}

std::string CSoapySdr::getDescription()
{
    if (m_device != nullptr) {
        return "SoapySDR (" + m_device->getDriverKey() + ")";
    }
    else {
        return "SoapySDR (uninitialised)";
    }
}

CDeviceID CSoapySdr::getID()
{
    return CDeviceID::SOAPYSDR;
}

bool CSoapySdr::setDeviceParam(DeviceParam param, const std::string& value)
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
    SoapySDR::Stream* stream = nullptr;
    try {
        const std::vector<size_t> channels{0};
        const auto args = SoapySDR::KwargsFromString(m_driver_args);
        stream = m_device->setupStream(SOAPY_SDR_RX, "CF32", channels, args);
        if (stream == nullptr)
            throw std::runtime_error("SoapySDR returned no stream");
        const int result = m_device->activateStream(stream);
        if (result != 0)
            throw std::runtime_error(std::string("SoapySDR activate: ") + SoapySDR::errToStr(result));
        process(stream);
    }
    catch (const std::exception& e) {
        std::clog << "SoapySDR stream failed: " << e.what() << std::endl;
    }

    if (stream != nullptr) {
        try {
            m_device->closeStream(stream);
        }
        catch (const std::exception& e) {
            std::clog << "SoapySDR stream cleanup failed: " << e.what() << std::endl;
        }
    }
    m_running = false;
}

void CSoapySdr::process(SoapySDR::Stream *stream)
{
    // Use the same windowed, hysteretic controller as the other SDR inputs.
    // Floats describe the nominal step sizes only; commands use m_gains' exact
    // double values so a fractional endpoint is never rounded out of range.
    const std::vector<float> gainsDb(m_gains.begin(), m_gains.end());
    bool agcFault = false;
    while (m_running) {

        uint64_t generation;
        {
            std::lock_guard<std::mutex> lock(m_sampleMutex);
            generation = m_sampleGeneration;
        }

        // Stream MTU is in samples, not bytes.
        const size_t mtu = m_device->getStreamMTU(stream);
        if (mtu == 0 || mtu > static_cast<size_t>(std::numeric_limits<int>::max()))
            throw std::runtime_error("Invalid SoapySDR stream MTU");

        // MTU is the driver's preferred maximum, not a required allocation.
        // A single read must fit the bounded decoder queue.
        const size_t samps_to_read = std::min(mtu,
                static_cast<size_t>(m_sampleBuffer.GetBufferSize()));
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
            if (static_cast<size_t>(ret) > samps_to_read)
                throw std::runtime_error("SoapySDR returned more samples than requested");
            {
                std::lock_guard<std::mutex> lock(m_sampleMutex);
                if (!m_running || generation != m_sampleGeneration) continue;
            }
            buf.resize(ret);

            if (m_agcRetryRequested.exchange(false)) agcFault = false;
            if (!m_sw_agc) {
                agcFault = false;
            }
            else if (!agcFault && !buf.empty()) {
                // Invalid or disconnected input must not drive gain to its
                // maximum. CF32 samples have component full scale 1.
                const bool valid = std::all_of(buf.begin(), buf.end(), [](const DSPCOMPLEX& z) {
                    return std::isfinite(z.real()) && std::isfinite(z.imag());
                });
                const bool signal = std::any_of(buf.begin(), buf.end(), [](const DSPCOMPLEX& z) {
                    return z.real() != 0 || z.imag() != 0;
                });
                if (!valid || !signal) {
                    m_agc.reset();
                }
                else {
                    m_agc.process(buf.data(), buf.size());
                    try {
                        const int current = getGainIndex();
                        const int next = m_agc.nextGainIndex(gainsDb, current);
                        if (next != current && next >= 0) {
                            m_device->setGain(SOAPY_SDR_RX, 0, m_gains.at(next));
                            m_agc.gainChanged();
                        }
                    }
                    catch (const std::exception& error) {
                        std::clog << "Soapy AGC disabled after gain error: " << error.what()
                                  << "; restart or toggle AGC to retry" << std::endl;
                        agcFault = true;
                    }
                }
            }

            // A read can complete after reset() or stop(). Publish both queues
            // under the reset lock, and never reintroduce the previous channel.
            std::lock_guard<std::mutex> lock(m_sampleMutex);
            if (!m_running || generation != m_sampleGeneration) continue;
            m_sampleBuffer.putDataIntoBuffer(buf.data(), ret);
            m_spectrumSampleBuffer.putDataIntoBuffer(buf.data(), ret);
            putIntoRecordBuffer(buf.data(), ret);
        }
    }
}
