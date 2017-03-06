/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
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
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __VIRTUAL_INPUT
#define __VIRTUAL_INPUT

#include "dab-constants.h"
#include <QObject>
#include <QString>
#include <stdint.h>

// Enum of available input device
enum class CDeviceID {AIRSPY, NULLDEVICE, RAWFILE, RTL_SDR, RTL_TCP};

// Device interface
class CVirtualInput : public QObject {
public:
    virtual void setFrequency(int32_t Frequency) = 0;
    virtual bool restart(void) = 0;
    virtual void stop(void) = 0;
    virtual void reset(void) = 0;
    virtual int32_t getSamples(DSPCOMPLEX* Buffer, int32_t Size) = 0;
    virtual int32_t getSpectrumSamples(DSPCOMPLEX* Buffer, int32_t Size) = 0;
    virtual int32_t getSamplesToRead(void) = 0;
    virtual float setGain(int32_t Gain) = 0;
    virtual int32_t getGainCount(void) = 0;
    virtual void setAgc(bool AGC) = 0;
    virtual QString getName(void) = 0;
    virtual CDeviceID getID(void) = 0;
};

#endif
