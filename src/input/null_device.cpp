/*
 *    Copyright (C) 2017
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

#include "null_device.h"

CNullDevice::CNullDevice()
{

}

void CNullDevice::setFrequency(int Frequency)
{
    (void) Frequency;
}

int CNullDevice::getFrequency(void) const
{
    return 0;
}

bool CNullDevice::restart()
{
    return false;
}

bool CNullDevice::is_ok()
{
    return true;
}

void CNullDevice::stop()
{

}

void CNullDevice::reset()
{

}

int32_t CNullDevice::getSamples(DSPCOMPLEX *Buffer, int32_t Size)
{
    memset((void*)Buffer, 0, Size * sizeof(DSPCOMPLEX));

    return Size;
}

std::vector<DSPCOMPLEX> CNullDevice::getSpectrumSamples(int size)
{
    std::vector<DSPCOMPLEX> sampleBuffer(size);
    std::fill(sampleBuffer.begin(), sampleBuffer.end(), 0);
    return sampleBuffer;
}

int32_t CNullDevice::getSamplesToRead()
{
    return 0;
}

float CNullDevice::getGain() const
{
    return 0;
}

float CNullDevice::setGain(int Gain)
{
    (void) Gain;

    return 0;
}

int CNullDevice::getGainCount()
{
    return 0;
}

void CNullDevice::setAgc(bool AGC)
{
    (void) AGC;
}

std::string CNullDevice::getDescription()
{
    return "Null device";
}

CDeviceID CNullDevice::getID()
{
    return CDeviceID::NULLDEVICE;
}
