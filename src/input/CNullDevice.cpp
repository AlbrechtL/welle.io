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

#include "CNullDevice.h"

CNullDevice::CNullDevice()
{

}

void CNullDevice::setFrequency(int32_t Frequency)
{
    (void) Frequency;
}

bool CNullDevice::restart()
{
    return false;
}

void CNullDevice::stop()
{

}

void CNullDevice::reset()
{

}

int32_t CNullDevice::getSamples(DSPCOMPLEX *Buffer, int32_t Size)
{
    memset(Buffer, 0, Size * sizeof(DSPCOMPLEX));

    return Size;
}

int32_t CNullDevice::getSpectrumSamples(DSPCOMPLEX *Buffer, int32_t Size)
{
    memset(Buffer, 0, Size * sizeof(DSPCOMPLEX));

    return Size;
}

int32_t CNullDevice::getSamplesToRead()
{
    return 0;
}

float CNullDevice::setGain(int32_t Gain)
{
    (void) Gain;

    return 0;
}

int32_t CNullDevice::getGainCount()
{
    return 0;
}

void CNullDevice::setAgc(bool AGC)
{
    (void) AGC;
}

QString CNullDevice::getName()
{
    return "Null device";
}
