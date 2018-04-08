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

enum class AudioServiceComponentType { DAB, DABPlus, Unknown };

enum class TransportMode { Audio, StreamData, FIDC, PacketData };

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

extern const int ProtLevel[64][3];

class DABParams {
public:
    DABParams();
    DABParams(int mode);
    void setMode(int mode);

    // To access directly the members is ugly but it was the easiest for the existing code
    uint8_t dabMode;
    int16_t L; // symbols per transmission frame
    int16_t K; // Number of FFT carriers with power
    int16_t T_null; // null symbol length
    int32_t T_F; // samples per transmission frame
    int16_t T_s; // symbol length including cyclic prefix
    int16_t T_u; // Size of the FFT == symbol length without cyclic prefix
    int16_t guardLength;
    int16_t carrierDiff;
};

struct packetdata_t {
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
    bool valid;
};

struct audiodata_t {
    int16_t subchId;
    int16_t startAddr;
    bool shortForm;
    int16_t protLevel;
    int16_t length;
    int16_t bitRate;
    int16_t ASCTy;
    int16_t language;
    int16_t programType;
    bool valid;
};


struct dabLabel {
    std::string label; // UTF-8 encoded
    uint8_t     mask = 0x00;
};


//  from FIG1/2
struct Service {
    uint32_t serviceId = 0;
    dabLabel serviceLabel;
    bool     hasPNum = false;
    bool     hasLanguage = false;
    int16_t  language = -1;
    int16_t  programType = 0;
    uint16_t pNum = 0;
};

//      The service component describes the actual service
//      It really should be a union
struct ServiceComponent {
    int8_t       TMid;           // the transport mode
    uint32_t     SId;            // belongs to the service
    int16_t      componentNr;    // component

    int16_t      ASCTy;          // used for audio
    int16_t      PS_flag;        // use for both audio and packet
    int16_t      subchannelId;   // used in both audio and packet
    uint16_t     SCId;           // used in packet
    uint8_t      CAflag;         // used in packet (or not at all)
    int16_t      DSCTy;          // used in packet
    uint8_t      DGflag;         // used for TDC
    int16_t      packetAddress;  // used in packet

    TransportMode transportMode(void) const;
    AudioServiceComponentType audioType(void) const;
};

struct Subchannel {
    int32_t  subChId;
    int32_t  startAddr;
    int32_t  length;
    bool     shortForm;

    // when short-form, UEP:
    int16_t  tableIndex;

    // when long-form:
    // Option 0: EEP-A
    // Option 1: EEP-B
    int32_t  protOption;
    int32_t  protLevel;

    int16_t  language;

    // For subchannels carrying packet-mode service components
    int16_t  fecScheme; // 0=no FEC, 1=FEC, 2=Rfu, 3=Rfu

    // Calculate the effective subchannel bitrate
    int32_t bitrate(void) const;
};

#endif
