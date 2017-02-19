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
#include <QDialog>
#include <QObject>
#include <stdint.h>

#define NIX 0100
#define FILEREADER 0200
#define DAB_STICK 0101
#define AIRSPY 0102
#define ELAD 0104
#define SDRPLAY 0110

#define someStick(x) (x & 017)
class virtualInput : public QObject {
public:
    virtualInput(void);
    virtual ~virtualInput(void);
    virtual void setVFOFrequency(int32_t);
    virtual int32_t getVFOFrequency(void);
    virtual uint8_t myIdentity(void);
    virtual bool restartReader(void);
    virtual void stopReader(void);
    virtual int32_t getSamples(DSPCOMPLEX*, int32_t);
    virtual int32_t getSamplesFromShadowBuffer(DSPCOMPLEX* V, int32_t size);
    virtual int32_t Samples(void);
    virtual void resetBuffer(void);
    virtual int16_t bitDepth(void) { return 10; }
    //
    //	To accomodate gui_3 without a separate control for the device
    virtual void setGain(int32_t);
    virtual void setAgc(bool);
    //
protected:
    int32_t lastFrequency;
    int32_t vfoOffset;
    int theGain;
};
#endif
