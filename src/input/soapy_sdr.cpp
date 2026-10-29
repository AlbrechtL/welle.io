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
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <sstream>
#include <iostream>
#include <cassert>
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

namespace { // AGC helpers: no device access, independently regression tested.
// Keep the public/manual gain indices unchanged. AGC uses the actual device
// step instead: e.g. SoapyOsmo miri's 0.1 means one native tuner-gain unit.
double agcNextGain(double current, double minimum, double maximum, double step, bool up)
{
    if (!std::isfinite(current) || !std::isfinite(minimum) ||
        !std::isfinite(maximum) || maximum < minimum) return current;
    if (!std::isfinite(step) || step <= 0) step = 1.0;
    if (current < minimum) return minimum;
    if (current > maximum) return maximum;
    const double position = (current - minimum) / step;
    // getGain() may have passed through float. Avoid choosing the same
    // hardware step again because of a tiny rounding difference.
    const double index = up ? std::floor(position + 1e-5) + 1
                            : std::ceil(position - 1e-5) - 1;
    return std::max(minimum, std::min(maximum, minimum + index * step));
}

struct AgcWindow {
    std::array<size_t, 1024> histogram{};
    size_t count = 0, invalid = 0, clipped = 0;
    double power = 0;
    void add(double i, double q, double fullScale) {
        if (!std::isfinite(i) || !std::isfinite(q)) { ++invalid; return; }
        i = std::abs(i) / fullScale;
        q = std::abs(q) / fullScale;
        const double a = std::max(i, q);
        ++histogram[std::min<size_t>(1023, size_t(std::min(a, 1.0) * 1024))];
        ++count;
        power += (i*i + q*q) * 0.5;
        if (a >= 0.995) ++clipped;
    }
    double level999() const {
        if (!count) return 0;
        size_t sum = 0;
        for (size_t b = 0; b < histogram.size(); ++b) {
            sum += histogram[b];
            if (sum >= 0.999 * count) return (b + 1.0) / 1024.0;
        }
        return 1;
    }
    // Return -1/0/+1. A single spike must not pull down an entire DAB
    // ensemble. Persistent peaks/clipping lower gain promptly; raising it
    // needs two quiet windows. The wide deadband prevents gain hunting.
    int direction(unsigned& lowWindows) const {
        if (!count || invalid || power == 0) { lowWindows = 0; return 0; }
        const double level = level999();
        const double clipping = double(clipped) / count;
        if (level > 0.85 || clipping > 0.001) { lowWindows = 0; return -1; }
        if (level < 0.55 && clipping < 0.0001) {
            if (++lowWindows >= 2) { lowWindows = 0; return 1; }
        } else lowWindows = 0;
        return 0;
    }
};
} // AGC helpers


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

    // Join a finished worker and release its device before replacing either.
    stop();
    m_gains.clear();
    m_sampleBuffer.FlushRingBuffer();
    m_spectrumSampleBuffer.FlushRingBuffer();

    try {
        m_device = SoapySDR::Device::make(m_driver_args);
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
        if (!m_device) throw runtime_error("SoapySDR returned no device");
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

        auto range = m_device->getGainRange(SOAPY_SDR_RX, 0);
        // Preserve existing manual gain indices; AGC uses range.step() below.
        m_gains.clear();
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
    catch (const std::exception& e) {
        std::clog << "SoapySDR configuration failed: " << e.what() << std::endl;
        stop();
        return false;
    }
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
        auto* device = m_device;
        m_device = nullptr;
        try { SoapySDR::Device::unmake(device); }
        catch (...) { std::clog << "SoapySDR device cleanup failed" << std::endl; }
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


void CSoapySdr::increaseGain()
{
    if (m_device != nullptr) {
        const auto range = m_device->getGainRange(SOAPY_SDR_RX, 0);
        const double current = m_device->getGain(SOAPY_SDR_RX, 0);
        const double target = agcNextGain(current, range.minimum(), range.maximum(), range.step(), true);
        if (std::isfinite(target) && std::abs(target - current) > 1e-6)
            m_device->setGain(SOAPY_SDR_RX, 0, target);
    }
}

void CSoapySdr::decreaseGain()
{
    if (m_device != nullptr) {
        const auto range = m_device->getGainRange(SOAPY_SDR_RX, 0);
        const double current = m_device->getGain(SOAPY_SDR_RX, 0);
        const double target = agcNextGain(current, range.minimum(), range.maximum(), range.step(), false);
        if (std::isfinite(target) && std::abs(target - current) > 1e-6)
            m_device->setGain(SOAPY_SDR_RX, 0, target);
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
    bool active = false;
    try {
        const auto args = SoapySDR::KwargsFromString(m_driver_args);
        stream = m_device->setupStream(SOAPY_SDR_RX, "CF32", {0}, args);
        if (!stream) throw runtime_error("setupStream returned no stream");
        const int result = m_device->activateStream(stream);
        if (result != 0) throw runtime_error(SoapySDR::errToStr(result));
        active = true;
        process(stream);
    }
    catch (const std::exception& e) {
        std::clog << "SoapySDR worker failed: " << e.what() << std::endl;
    }
    catch (...) { std::clog << "SoapySDR worker failed" << std::endl; }
    // Cleanup must not let driver exceptions escape a std::thread.
    if (active) {
        try { m_device->deactivateStream(stream); }
        catch (...) { std::clog << "SoapySDR deactivate failed" << std::endl; }
    }
    if (stream) {
        try { m_device->closeStream(stream); }
        catch (...) { std::clog << "SoapySDR close failed" << std::endl; }
    }
    m_running = false;
}

void CSoapySdr::process(SoapySDR::Stream *stream)
{
    m_iqCounter.begin();
    // CF32 normally uses +/-1. The legacy SoapyOsmo miri adapter divides
    // signed 16-bit samples by 4096 instead, so its component full scale is
    // 8. Restrict this compatibility case to the verified legacy gain API;
    // do not apply it to other Soapy devices or modern Miri gain stages.
    const auto range = m_device->getGainRange(SOAPY_SDR_RX, 0);
    const auto names = m_device->listGains(SOAPY_SDR_RX, 0);
    const bool legacyMiri = m_device->getDriverKey() == "miri" &&
        names == std::vector<std::string>{"LNA"} &&
        std::abs(range.minimum()) < 1e-6 && std::abs(range.maximum() - 10.2) < 1e-6 &&
        std::abs(range.step() - 0.1) < 1e-6;
    const double fullScale = legacyMiri ? 8.0 : 1.0;
    std::clog << "Soapy AGC component full scale=" << fullScale
              << " gain step=" << range.step() << std::endl;
    const bool diagnose = std::getenv("WELLE_AGC_DIAGNOSTICS") != nullptr;
    const size_t windowSamples = INPUT_RATE / 4; // 250 ms of valid IQ
    AgcWindow window;
    unsigned lowWindows = 0;
    size_t settleSamples = 0;
    bool agcFault = false;
    using clock = std::chrono::steady_clock;
    auto mark = clock::now(), lastReadEnd = mark;
    uint64_t received = 0, dropped = 0, overflow = 0, underflow = 0, timeout = 0;
    double maxReadMs = 0, maxWorkMs = 0;
    const char* fixed = std::getenv("WELLE_DIAG_FIXED_GAIN");
    if (fixed) {
        m_sw_agc = false;
        m_device->setGain(SOAPY_SDR_RX, 0, std::stod(fixed));
        std::clog << "IQ_DIAG fixed_gain=" << m_device->getGain(SOAPY_SDR_RX, 0) << std::endl;
    }
    const size_t mtu = m_device->getStreamMTU(stream);
    if (mtu == 0 || mtu > 16 * 1024 * 1024) throw runtime_error("Invalid stream MTU");
    std::vector<DSPCOMPLEX> buf(mtu);
    while (m_running) {

        // Stream MTU is in samples, not bytes.
        const size_t samps_to_read = mtu;
        buf.resize(mtu);

        void *buffs[1];
        buffs[0] = buf.data();

        int flags = 0;
        long long timeNs = 0;
        assert(m_device != nullptr);
        auto beginRead = clock::now();
        maxWorkMs = std::max(maxWorkMs, std::chrono::duration<double, std::milli>(beginRead-lastReadEnd).count());
        int ret = m_device->readStream(
                stream, buffs, samps_to_read, flags, timeNs);
        // Count delivered complex samples before any Welle queue drops.
        // CF32 is eight bytes per I/Q pair here, not the USB transport format.
        if (ret > 0 && static_cast<size_t>(ret) <= samps_to_read)
            m_iqCounter.record(static_cast<size_t>(ret));
        lastReadEnd = clock::now();
        maxReadMs = std::max(maxReadMs, std::chrono::duration<double, std::milli>(lastReadEnd-beginRead).count());
        if (ret > 0) received += ret;
        if (ret == SOAPY_SDR_TIMEOUT) ++timeout;
        if (ret == SOAPY_SDR_OVERFLOW) ++overflow;
        if (ret == SOAPY_SDR_UNDERFLOW) ++underflow;
        double elapsed = std::chrono::duration<double>(lastReadEnd-mark).count();
        if (elapsed >= 5) {
            std::clog << "IQ_DIAG epoch=" << std::time(nullptr) << " seconds=" << elapsed
                      << " samples=" << received << " dropped=" << dropped
                      << " overflow=" << overflow << " underflow=" << underflow
                      << " timeout=" << timeout << " max_read_ms=" << maxReadMs
                      << " max_work_ms=" << maxWorkMs
                      << " queued=" << m_sampleBuffer.GetRingBufferReadAvailable()
                      << " mtu=" << mtu << std::endl;
            mark=lastReadEnd; received=dropped=overflow=underflow=timeout=0; maxReadMs=maxWorkMs=0;
        }

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
            if (static_cast<size_t>(ret) > mtu) throw runtime_error("Invalid stream sample count");
            buf.resize(ret);

            if (m_sw_agc.load() && !agcFault) {
                if (settleSamples) {
                    settleSamples -= std::min(settleSamples, buf.size());
                } else {
                    for (const auto& z : buf) window.add(z.real(), z.imag(), fullScale);
                    if (window.count + window.invalid >= windowSamples) {
                        const int direction = window.direction(lowWindows);
                        try {
                            const double before = m_device->getGain(SOAPY_SDR_RX, 0);
                            if (direction < 0) decreaseGain();
                            else if (direction > 0) increaseGain();
                            const double after = m_device->getGain(SOAPY_SDR_RX, 0);
                            const bool changed = std::abs(after - before) > 1e-6;
                            if (changed || diagnose) {
                                std::clog << "SOAPY_AGC gain=" << before << "->" << after
                                          << " q999=" << window.level999()
                                          << " clip=" << (window.count ? double(window.clipped)/window.count : 0)
                                          << " invalid=" << window.invalid << std::endl;
                            }
                            if (changed) {
                                settleSamples = windowSamples;
                                lowWindows = 0;
                            }
                        } catch (const std::exception& error) {
                            // Keep reception alive at its last gain; do not retry
                            // a broken control every block or terminate the thread.
                            std::clog << "Soapy AGC stopped after control error: " << error.what() << std::endl;
                            agcFault = true;
                        }
                        window = AgcWindow{};
                    }
                }
            } else {
                window = AgcWindow{};
                lowWindows = 0;
                settleSamples = 0;
            }

            const auto written = m_sampleBuffer.putDataIntoBuffer(buf.data(), ret);
            if (written < ret) dropped += ret-written;
            m_spectrumSampleBuffer.putDataIntoBuffer(buf.data(), ret);
        }
    }
}
