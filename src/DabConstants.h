/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012
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
//
//	Common definitions and includes for
//	the DAB decoder

#ifndef __DAB_CONSTANTS
#define __DAB_CONSTANTS

#include <complex>
#include <cstring>
#include <limits>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>

#include <QString>
#include <QObject>

typedef float DSPFLOAT;
typedef std::complex<DSPFLOAT> DSPCOMPLEX;

// Fallback if git hash macro is not defined
#ifndef GITHASH
#pragma message "Git hash is not defined! Set it to \"unknown\""
#define GITHASH "unknown"
#endif

#define CURRENT_VERSION "1.0-rc2"

#define DAB 0100
#define DAB_PLUS 0101

#define AUDIO_SERVICE 0101
#define PACKET_SERVICE 0102
#define UNKNOWN_SERVICE 0100

#define INPUT_RATE 2048000
#define BANDWIDTH 1536000

#define SYNCED 01
#define LONG_HIGH 02
#define LONG_LOW 03
#define UNSYNCED 04

// Static class to hold constant values
class CDABConstants: public QObject {
    Q_OBJECT
public:
    static QString getProgramTypeName(int Type);
    static QString getLanguageName(int Language);
};

class CDABParams {
public:
    CDABParams();
    CDABParams(int Mode);
    void setMode(int Mode);

    // To access directly the members is ugly but it was the easiest for the existing code
    uint8_t dabMode;
    int16_t L; // blocks per frame
    int16_t K; // carriers
    int16_t T_null; // null length
    int32_t T_F; // samples per frame
    int16_t T_s; // block length
    int16_t T_u; // useful part
    int16_t guardLength;
    int16_t carrierDiff;

private:
    void setMode1(void);
    void setMode2(void);
    void setMode3(void);
    void setMode4(void);
};

typedef struct {
    int16_t subchId;
    int16_t startAddr;
    bool shortForm;
    int16_t protLevel;
    int16_t DSCTy;
    int16_t length;
    int16_t bitRate;
    int16_t FEC_scheme;
    int16_t DGflag;
    int16_t packetAddress;
} packetdata;

typedef struct {
    int16_t subchId;
    int16_t startAddr;
    bool shortForm;
    int16_t protLevel;
    int16_t length;
    int16_t bitRate;
    int16_t ASCTy;
    int16_t language;
    int16_t programType;
    bool defined;
} audiodata;

#endif
