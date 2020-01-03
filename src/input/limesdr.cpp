/*
 *    Copyright (C) 2020
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on Qt-DAB
 *    Copyright (C) 2014 .. 2017
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
#include "limesdr.h"

// For Qt translation if Qt is exisiting
#ifdef QT_CORE_LIB
#include <QtGlobal>
#else
#define QT_TRANSLATE_NOOP(x,y) (y)
#endif

#define FIFO_SIZE 32768
static
int16_t localBuffer [4 * FIFO_SIZE];
lms_info_str_t limedevices [10];

CLimeSDR::CLimeSDR(RadioControllerInterface &radioController) :
    radioController(radioController),
    SampleBuffer(256 * 1024),
    SpectrumSampleBuffer(8192)
{
    std::clog << "LimeSDR: " << "Open LimeSDR" << std::endl;

    //
    //      From here we have a library available

    int ndevs = LMS_GetDeviceList (limedevices);
    if (ndevs == 0) { // no devices found
        throw (21);
    }

    for (int i = 0; i < ndevs; i ++)
         std::clog << "LimeSDR: device " << limedevices [i] << std::endl;

    int res  = LMS_Open (&theDevice, nullptr, nullptr);
    if (res < 0) { // some error
        throw (22);
    }

    res  = LMS_Init (theDevice);
    if (res < 0) { // some error
        LMS_Close (&theDevice);
        throw (23);
    }

    res  = LMS_GetNumChannels (theDevice, LMS_CH_RX);
    if (res < 0) { // some error
        LMS_Close (&theDevice);
        throw (24);
    }

    std::clog << "LimeSDR: device " << limedevices [0] << "supports" << res << "channels" << std::endl;
    res  = LMS_EnableChannel (theDevice, LMS_CH_RX, 0, true);
    if (res < 0) { // some error
        LMS_Close (theDevice);
        throw (24);
    }

    res = LMS_SetSampleRate (theDevice, 2048000.0, 0);
    if (res < 0) {
        LMS_Close (theDevice);
        throw (25);
    }

    float_type host_Hz, rf_Hz;
    res = LMS_GetSampleRate (theDevice, LMS_CH_RX, 0,
                             &host_Hz, &rf_Hz);

    std::clog << "LimeSDR: samplerate = " << (float) host_Hz << " " << (float) rf_Hz << std::endl;

    //    res  = LMS_GetAntennaList (theDevice, LMS_CH_RX, 0, antennas);
    //    for (int i = 0; i < res; i ++)
    //       antennaList -> addItem (QString (antennas [i]));

    //    limeSettings -> beginGroup ("limeSettings");
    //    QString antenne = limeSettings -> value ("antenna", "default"). toString();
    //    limeSettings -> endGroup();

    //    int k       = antennaList -> findText (antenne);
    //        if (k != -1)
    //           antennaList -> setCurrentIndex (k);

    //    connect (antennaList, SIGNAL (activated (int)),
    //             this, SLOT (setAntenna (int)));

    // default antenna setting
    res  = LMS_SetAntenna (theDevice, LMS_CH_RX, 0, 3); // LNAW on LimSDR mini

    // default frequency
    res  = LMS_SetLOFrequency (theDevice, LMS_CH_RX,
                                  0, 220000000.0);
    if (res < 0) {
        LMS_Close (theDevice);
        throw (26);
    }

    res  = LMS_SetLPFBW (theDevice, LMS_CH_RX,
                            0, 1536000.0);
    if (res < 0) {
        LMS_Close (theDevice);
        throw (27);
    }

    LMS_SetGaindB (theDevice, LMS_CH_RX, 0, 50);

    LMS_Calibrate (theDevice, LMS_CH_RX, 0, 2500000.0, 0);

    return;
}

CLimeSDR::~CLimeSDR(void)
{
    stop();
    running = false;
    LMS_Close (theDevice);
}

void CLimeSDR::setVFOFrequency (int32_t f) {
    LMS_SetLOFrequency (theDevice, LMS_CH_RX, 0, f);
}

int32_t CLimeSDR::getVFOFrequency() {
    float_type freq;
    int res = LMS_GetLOFrequency (theDevice, LMS_CH_RX, 0, &freq);
    return (int)freq;
}


void CLimeSDR::setFrequency(int nf)
{
    freq = nf;
    LMS_SetLOFrequency (theDevice, LMS_CH_RX, 0, nf);
    LMS_SetGaindB (theDevice, LMS_CH_RX, 0, 50);
}

int CLimeSDR::getFrequency() const
{
    return freq;
}

bool CLimeSDR::restart(void)
{
    if (running)
        return true;
    LMS_SetLOFrequency (theDevice, LMS_CH_RX, 0, freq);
    stream. isTx            = false;
    stream. channel         = 0;
    stream. fifoSize        = FIFO_SIZE;
    stream. throughputVsLatency     = 0.1;  // ???
    stream. dataFmt         = lms_stream_t::LMS_FMT_I12;    // 12 bit ints
    int res     = LMS_SetupStream (theDevice, &stream);
    if (res < 0)
        return false;
    res     = LMS_StartStream (&stream);
    if (res < 0)
        return false;

    limesdr_thread = std::thread(&CLimeSDR::limesdr_thread_run, this);

    return true;
}

void CLimeSDR::limesdr_thread_run()
{
    std::clog << "LimeSDR: " << "Start limesdr_thread_run() thread" << std::endl;

    int res;
    lms_stream_status_t streamStatus;
    int underruns = 0;
    int overruns = 0;
    int dropped  = 0;
    int amountRead = 0;

    running = true;
    while (running) {
        res = LMS_RecvStream (&stream, localBuffer,
                              FIFO_SIZE,  &meta, 1000);
        if (res > 0) {
            std::vector<DSPCOMPLEX> temp(res);
            for (int i = 0; i < res; i ++) {
                temp[i] = DSPCOMPLEX(localBuffer[2*i] / 2048.0, localBuffer[2*i+1] / 2048.0);
            }
            SampleBuffer.putDataIntoBuffer (temp.data(), res);
            SpectrumSampleBuffer.putDataIntoBuffer (temp.data(), res);
            amountRead += res;
            res = LMS_GetStreamStatus (&stream, &streamStatus);
            underruns += streamStatus. underrun;
            overruns += streamStatus. overrun;
        }
        if (amountRead > 4 * 2048000) {
            amountRead = 0;
            //              showErrors (underruns, overruns);
            underruns = 0;
            overruns = 0;
        }
    }

    std::clog << "LimeSDR: " << "End limesdr_thread_run() thread" << std::endl;
}

bool CLimeSDR::is_ok()
{
    // ToDo

    return running;
}

void CLimeSDR::stop(void)
{
    if (!running)
        return;

    (void)LMS_StopStream (&stream);
    (void)LMS_DestroyStream (theDevice, &stream);
}

void CLimeSDR::reset(void)
{
    SampleBuffer.FlushRingBuffer();
}

int32_t CLimeSDR::getSamples(DSPCOMPLEX* Buffer, int32_t Size)
{
    return SampleBuffer.getDataFromBuffer(Buffer, Size);
}

std::vector<DSPCOMPLEX> CLimeSDR::getSpectrumSamples(int size)
{
    std::vector<DSPCOMPLEX> buf(size);
    int sizeRead = SpectrumSampleBuffer.getDataFromBuffer(buf.data(), size);
    if (sizeRead < size) {
        buf.resize(sizeRead);
    }
    return buf;
}

int32_t CLimeSDR::getSamplesToRead(void)
{
    return SampleBuffer.GetRingBufferReadAvailable();
}

int CLimeSDR::getGainCount()
{
    return 21; // ToDo
}

void CLimeSDR::setAgc(bool agc)
{
    // ToDo
}

std::string CLimeSDR::getDescription()
{
    std::string ver = "LimeSDR mini";
    return ver;
}

bool CLimeSDR::setDeviceParam(DeviceParam param, int value)
{
    // ToDo
}

CDeviceID CLimeSDR::getID()
{
    return CDeviceID::LIMESDR;
}

float CLimeSDR::getGain() const
{
    return currentLinearityGain;
}

float CLimeSDR::setGain(int gain)
{
    std::clog  << "LimeSDR: setgain: " << gain << std::endl;
    float_type gg;
    LMS_SetGaindB (theDevice, LMS_CH_RX, 0, gain);
    LMS_GetNormalizedGain (theDevice, LMS_CH_RX, 0, &gg); // ToDo convert to currentLinearityGain

    return currentLinearityGain;
}

