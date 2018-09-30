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

#include <iostream>
#include "airspy_sdr.h"

static const int EXTIO_NS = 8192;
static const int EXTIO_BASE_TYPE_SIZE = sizeof(float);

CAirspy::CAirspy() :
    SampleBuffer(256 * 1024),
    SpectrumSampleBuffer(8192)
{
    std::clog << "Airspy:" << "Open airspy" << std::endl;

    device = {};

    int result = airspy_init();
    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy:" << "airspy_init () failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        throw 0;
    }

    result = airspy_open(&device);
    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy:" << "airpsy_open () failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        throw 0;
    }

    airspy_set_sample_type(device, AIRSPY_SAMPLE_FLOAT32_IQ);

    result = airspy_set_samplerate(device, AIRSPY_SAMPLERATE);
    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy:" <<"airspy_set_samplerate() failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        throw 0;
    }

    if (sw_agc) {
        setAgc(true);
    }
    else {
        setAgc(false);
        setGain(currentLinearityGain);
    }

    running = false;

    return;
}

CAirspy::~CAirspy(void)
{
    if (device) {
        int result = airspy_stop_rx(device);
        if (result != AIRSPY_SUCCESS) {
            std::clog  << "Airspy: airspy_stop_rx () failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        }

        result = airspy_close(device);
        if (result != AIRSPY_SUCCESS) {
            std::clog  << "Airspy: airspy_close () failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        }
    }

    airspy_exit();
}

void CAirspy::setFrequency(int nf)
{
    freq = nf;
    int result = airspy_set_freq(device, nf);

    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: airspy_set_freq() failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
    }
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
    result = airspy_set_sample_type(device, AIRSPY_SAMPLE_FLOAT32_IQ);
    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: airspy_set_sample_type () failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        return false;
    }

    result = airspy_start_rx(device, (airspy_sample_block_cb_fn)callback, this);
    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: airspy_start_rx () failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
        return false;
    }

    running = true;
    return true;
}

void CAirspy::stop(void)
{
    if (!running)
        return;
    int result = airspy_stop_rx(device);

    if (result != AIRSPY_SUCCESS) {
        std::clog  << "Airspy: airspy_stop_rx() failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;
    }
    running = false;
}

int CAirspy::callback(airspy_transfer* transfer)
{
    if (!transfer)
        return 0; // should not happen
    auto *p = static_cast<CAirspy*>(transfer->ctx);

    // AIRSPY_SAMPLE_FLOAT32_IQ:
    p->data_available(reinterpret_cast<const DSPCOMPLEX*>(transfer->samples),
            transfer->sample_count);
    return 0;
}

// Called from AirSpy data callback which gives us interleaved float32
// I and Q according to setting given to airspy_set_sample_type() above.
// The AirSpy runs at 4096ksps, we need to decimate by two.
int CAirspy::data_available(const DSPCOMPLEX* buf, size_t num_samples)
{
    if (num_samples % 2 != 0) {
        throw std::runtime_error("CAirspy::data_available() needs an even number of IQ samples to be able to decimate");
    }

    const DSPCOMPLEX* sbuf = reinterpret_cast<const DSPCOMPLEX*>(buf);

    std::vector<DSPCOMPLEX> temp(num_samples/2);

    float maxnorm = 0;

    for (size_t i = 0; i < num_samples/2; i++) {
        const auto z = 0.5f * (sbuf[2*i] + sbuf[2*i+1]);
        temp[i] = z;

        if (sw_agc and (num_frames % 200) == 0) {
            if (norm(z) > maxnorm) {
                maxnorm = norm(z);
            }
        }
    }

    if (sw_agc and (num_frames % 200) == 0) {
        const float maxampl = sqrt(maxnorm);

        if (maxampl > 0.2f) {
            const int newgain = currentLinearityGain--;
            if (newgain >= AIRSPY_GAIN_MIN) {
                setGain(newgain);
            }
        }
        else if (maxampl < 0.02f) {
            const int newgain = currentLinearityGain++;
            if (newgain <= AIRSPY_GAIN_MAX) {
                setGain(newgain);
            }
        }
    }

    num_frames++;

    SampleBuffer.putDataIntoBuffer(temp.data(), num_samples/2);
    SpectrumSampleBuffer.putDataIntoBuffer(temp.data(), num_samples/2);

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
    return 21;
}

void CAirspy::setAgc(bool agc)
{
    if (not agc) {
        int result = airspy_set_linearity_gain(device, currentLinearityGain);
        if (result != AIRSPY_SUCCESS)
            std::clog  << "Airspy: airspy_set_mixer_agc() failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;

    }

    sw_agc = agc;
}

void CAirspy::setHwAgc(bool hwAGC)
{
    (void)hwAGC;
}

std::string CAirspy::getName()
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

CDeviceID CAirspy::getID()
{
    return CDeviceID::AIRSPY;
}

float CAirspy::getGain() const
{
    return currentLinearityGain;
}

float CAirspy::setGain(int gain)
{
    currentLinearityGain = gain;

    int result = airspy_set_linearity_gain(device, currentLinearityGain);
    if (result != AIRSPY_SUCCESS)
        std::clog  << "Airspy: airspy_set_mixer_agc() failed:" << airspy_error_name((airspy_error)result) << "(" << result << ")" << std::endl;

    return currentLinearityGain;
}
