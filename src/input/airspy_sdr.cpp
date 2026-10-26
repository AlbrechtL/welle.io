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

#include <algorithm>
#include <chrono>
#include <iostream>
#include "airspy_sdr.h"

// For Qt translation if Qt is existing
#ifdef QT_CORE_LIB
    #include <QtGlobal>
#else
    #define QT_TRANSLATE_NOOP(x,y) (y)
#endif

static const int EXTIO_NS = 8192;
static const int EXTIO_BASE_TYPE_SIZE = sizeof(float);

CAirspy::CAirspy(RadioControllerInterface &radioController) :
    radioController(radioController),
    SampleBuffer(256 * 1024),
    SpectrumSampleBuffer(8192)
{
    std::clog << "Airspy: " << "Open airspy" << std::endl;

    device = {};

    int result = airspy_init();
    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: " << "airspy_init () failed: " << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        throw 0;
    }

    result = airspy_open(&device);
    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: " << "airpsy_open () failed: " << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        throw 0;
    }

    airspy_set_sample_type(device, AIRSPY_SAMPLE_FLOAT32_IQ);

    // Use the lowest native sample rate not below 2.048 MS/s (Mini: 3 MS/s,
    // R2: 2.5 MS/s). Other rates are possible, but give a clearly worse
    // signal quality. The resampler converts to 2.048 MS/s.
    uint32_t rateCount = 0;
    airspy_get_samplerates(device, &rateCount, 0);
    std::vector<uint32_t> rates(rateCount);
    if (rateCount > 0) {
        airspy_get_samplerates(device, rates.data(), rateCount);
    }
    sampleRate = 0;
    for (const uint32_t rate : rates) {
        if (rate >= (uint32_t)INPUT_RATE && (sampleRate == 0 || (int)rate < sampleRate)) {
            sampleRate = rate;
        }
    }
    if (sampleRate == 0) {
        sampleRate = 4096000;
    }
    std::clog << "Airspy: sample rate " << sampleRate << std::endl;
    resampler = std::make_unique<RationalResampler>(sampleRate, INPUT_RATE);

    // The level is measured on the samples of the ADC. Measured with an
    // Airspy Mini, the SNR doesn't improve any more above about -40 dBFS
    // in all gain modes, and nothing clips up to the maximum gain; -25 dBFS
    // keeps the gain in that range with about 20 dB headroom.
    agc = std::make_unique<SoftwareAgc>(200, sampleRate, -25.0f);

    result = airspy_set_samplerate(device, sampleRate);
    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: " <<"airspy_set_samplerate() failed: " << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        throw 0;
    }

    // Start in the middle of the gain range
    currentGain = maxGainIndex() / 2;
    applyGain(currentGain);

    running = false;

    return;
}

CAirspy::~CAirspy(void)
{
    if (device) {
        stop();

        int result = airspy_close(device);
        if (result != AIRSPY_SUCCESS) {
            std::clog  << "Airspy: airspy_close () failed: " << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        }
    }

    airspy_exit();
}

void CAirspy::setFrequency(int nf)
{
    freq = nf;
    int result = airspy_set_freq(device, nf);

    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: airspy_set_freq() failed: " << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
    }
    agc->reset();
}

int CAirspy::getFrequency() const
{
    return freq;
}

bool CAirspy::restart(void)
{
    int result;
    if (running)
        return true;

    SampleBuffer.FlushRingBuffer();
    SpectrumSampleBuffer.FlushRingBuffer();
    resampler->reset();
    result = airspy_set_sample_type(device, AIRSPY_SAMPLE_FLOAT32_IQ);
    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: airspy_set_sample_type () failed: " << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        return false;
    }

    result = airspy_start_rx(device, (airspy_sample_block_cb_fn)callback, this);
    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: airspy_start_rx () failed: " << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        return false;
    }

    running = true;
    agcThread = std::thread(&CAirspy::agcTimerThread, this);
    return true;
}

bool CAirspy::is_ok()
{
    // Check if airspy is still connected
    airspy_error status =  (airspy_error) airspy_is_streaming(device);
    if(status != AIRSPY_TRUE && running == true) {
        std::clog << "Airspy: airspy is not working. Maybe it is unplugged. Code: " << status <<  "running" << running << std::endl;
        radioController.onMessage(message_level_t::Error, QT_TRANSLATE_NOOP("CRadioController", "airspy is unplugged."));

        stop();
    }

    return running;
}

void CAirspy::stop(void)
{
    if (!running)
        return;
    running = false;
    int result = airspy_stop_rx(device);

    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: airspy_stop_rx() failed: " << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
    }

    if (agcThread.joinable()) {
        agcThread.join();
    }
}

int CAirspy::callback(airspy_transfer* transfer)
{
    if (!transfer) {
        throw std::logic_error("AIRSPY: no transfer");
    }
    auto *p = static_cast<CAirspy*>(transfer->ctx);

    // AIRSPY_SAMPLE_FLOAT32_IQ:
    p->data_available(reinterpret_cast<const DSPCOMPLEX*>(transfer->samples),
            transfer->sample_count);
    return 0;
}

// Called from AirSpy data callback which gives us interleaved float32
// I and Q according to setting given to airspy_set_sample_type() above.
// The AirSpy runs at a native sample rate, the resampler converts it to
// 2.048 MS/s and keeps the adjacent DAB blocks from aliasing into the
// wanted block.
int CAirspy::data_available(const DSPCOMPLEX* buf, size_t num_samples)
{
    // Level measurement of the software AGC, on the samples of the ADC
    agc->process(buf, num_samples);

    resampled.clear();
    const size_t count = resampler->process(buf, num_samples, resampled);

    SampleBuffer.putDataIntoBuffer(resampled.data(), count);
    SpectrumSampleBuffer.putDataIntoBuffer(resampled.data(), count);
    putIntoRecordBuffer(resampled.data(), count);

    return 0;
}

void CAirspy::reset(void)
{
    SampleBuffer.FlushRingBuffer();
    SpectrumSampleBuffer.FlushRingBuffer();
}

int32_t CAirspy::getSamples(DSPCOMPLEX* Buffer, int32_t Size)
{
    return SampleBuffer.getDataFromBuffer(Buffer, Size);
}

std::vector<DSPCOMPLEX> CAirspy::getSpectrumSamples(int size)
{
    std::vector<DSPCOMPLEX> buf(size);
    int sizeRead = SpectrumSampleBuffer.getDataFromBuffer(buf.data(), size);
    if (sizeRead < size) {
        buf.resize(sizeRead);
    }
    return buf;
}

int32_t CAirspy::getSamplesToRead(void)
{
    return SampleBuffer.GetRingBufferReadAvailable();
}

int CAirspy::getGainCount()
{
    return maxGainIndex();
}

void CAirspy::setAgc(bool agc)
{
    sw_agc = agc;
    if (not agc) {
        applyGain(currentGain);
    }
}

std::string CAirspy::getDescription()
{
    // Get airspy device name and version
    char Version[255] = {0};
    airspy_version_string_read(device, Version, 20);

    // Get airspy library version
    airspy_lib_version_t lib_version;
    airspy_lib_version(&lib_version);

    std::string ver = Version;

    ver += "AirSpy, lib. v" +
        std::to_string(lib_version.major_version) + "." +
        std::to_string(lib_version.minor_version) + "." +
        std::to_string(lib_version.revision);
    return ver;
}

bool CAirspy::setDeviceParam(DeviceParam param, int value)
{
    switch (param) {
        case DeviceParam::BiasTee:
            std::clog << "Airspy: Set bias tee to " << value << std::endl;
            airspy_set_rf_bias(device, value);
            return true;
        case DeviceParam::AirspyGainMode:
        {
            const GainMode mode = value == 1 ? GainMode::Sensitivity :
                value == 2 ? GainMode::Hybrid : GainMode::Linearity;
            std::clog << "Airspy: Set gain mode to " << value << std::endl;
            {
                std::lock_guard<std::mutex> lock(gainMutex);
                gainMode = mode;
                // The linearity and sensitivity gains switch the AGC of LNA
                // and mixer off themselves
                if (mode == GainMode::Hybrid) {
                    airspy_set_lna_agc(device, 1);
                    airspy_set_mixer_agc(device, 1);
                }
            }
            currentGain = maxGainIndex() / 2;
            applyGain(currentGain);
            agc->reset();
            return true;
        }
        default:
            return false;
    }
}

CDeviceID CAirspy::getID()
{
    return CDeviceID::AIRSPY;
}

float CAirspy::getGain() const
{
    return currentGain;
}

int CAirspy::getGainIndex() const
{
    return currentGain;
}

float CAirspy::setGain(int gain)
{
    currentGain = std::max(0, std::min(gain, maxGainIndex()));
    applyGain(currentGain);
    return currentGain;
}

int CAirspy::maxGainIndex() const
{
    return gainMode == GainMode::Hybrid ? 15 : 21;
}

std::vector<float> CAirspy::gainTableDb() const
{
    // Nominal steps; measured with an Airspy Mini 1.4 to 4.8 dB (linearity,
    // sensitivity) and 3.3 to 3.7 dB (VGA). The AGC learns the real steps.
    const float step = gainMode == GainMode::Hybrid ? 3.4f : 3.5f;
    std::vector<float> table(maxGainIndex() + 1);
    for (size_t i = 0; i < table.size(); i++) {
        table[i] = i * step;
    }
    return table;
}

void CAirspy::applyGain(int index)
{
    std::lock_guard<std::mutex> lock(gainMutex);
    index = std::max(0, std::min(index, maxGainIndex()));

    int result;
    switch (gainMode) {
        case GainMode::Sensitivity:
            result = airspy_set_sensitivity_gain(device, index);
            break;
        case GainMode::Hybrid:
            result = airspy_set_vga_gain(device, index);
            break;
        default:
            result = airspy_set_linearity_gain(device, index);
    }
    std::clog << "Airspy: Set gain to " << index << std::endl;
    if (result != AIRSPY_SUCCESS) {
        std::clog << "Airspy: Setting gain failed: " << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
    }

    if (agc) {
        agc->gainChanged();
    }
}

// The gain is changed here and not in the USB callback: the USB control
// transfers would block the reception
void CAirspy::agcTimerThread()
{
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (sw_agc) {
            const int index = currentGain;
            const int next = agc->nextGainIndex(gainTableDb(), index);
            if (next != index) {
                currentGain = next;
                applyGain(next);
            }
        }
        else if (agc->overloaded()) {
            std::string text = QT_TRANSLATE_NOOP("CRadioController", "ADC overload. Maybe you are using a too high gain.");
            std::clog << "Airspy: " << text << std::endl;
            radioController.onMessage(message_level_t::Information, text);
        }
    }
}
