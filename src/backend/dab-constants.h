/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
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
//  Common definitions and includes for the DAB decoder

#ifndef __DAB_CONSTANTS
#define __DAB_CONSTANTS

#include <complex>
#include <limits>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

using DSPFLOAT = float;
using DSPCOMPLEX = std::complex<DSPFLOAT>;

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

namespace DABConstants {
    const char* getProgramTypeName(int type);
    const char* getLanguageName(int language);
}

class DABParams {
public:
    DABParams();
    DABParams(int mode);
    void setMode(int mode);

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
};

struct packetdata {
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
};

struct audiodata {
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
};

#endif
