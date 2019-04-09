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

#include "charsets.h"
#include <complex>
#include <limits>
#include <map>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

using DSPFLOAT = float;
using DSPCOMPLEX = std::complex<DSPFLOAT>;
using softbit_t = int8_t;

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

struct DabLabel {
    // Label from FIG 1
    /* FIG 1 labels are usually in EBU Latin encoded */
    CharacterSet charset = CharacterSet::EbuLatin;
    std::string fig1_label; // encoded according to charset
    uint16_t    fig1_flag = 0x0000; // describes the short label

    /* If necessary, convert the label to UTF8 */
    std::string fig1_label_utf8() const;
    std::string fig1_shortlabel_utf8() const;

    void setCharset(uint8_t charset_id);

    // Extended Label from FIG 2
    /* FIG 2 labels are either in UTF-8 or UCS2. We store them as segments and build
     * a UTF-8 string when needed. */

    std::map<int, std::vector<uint8_t> > segments;
    size_t segment_count = 0; // number if actual segments (not segment count as in spec)
    CharacterSet extended_label_charset = CharacterSet::Undefined;
    uint8_t toggle_flag = 0;
    bool fig2_rfu = false; // draftETSI TS 103 176 v2.2.1 gives this a new meaning

    // Assemble all segments into a UTF-8 string. Returns an
    // empty string if not all segments received.
    std::string fig2_label() const;

    // Common to FIG 1 and FIG 2
    /* If FIG 2 label available, use that one, otherwise take the FIG 1 label */
    std::string utf8_label() const;
};

struct Service {
    Service(uint32_t sid) : serviceId(sid) {}

    uint32_t serviceId = 0;

    DabLabel serviceLabel;
    int16_t  language = 0;
    int16_t  programType = 0; // PTy, FIG0/17
};

//      The service component describes the actual service
//      It really should be a union
struct ServiceComponent {
    int8_t       TMid = 0;           // the transport mode
    uint32_t     SId = 0;            // belongs to the service
    int16_t      componentNr = 0;    // component

    DabLabel     componentLabel;

    int16_t      ASCTy = 0;          // used for audio
    int16_t      PS_flag = 0;        // use for both audio and packet
    int16_t      subchannelId = 0;   // used in both audio and packet
    uint16_t     SCId = 0;           // used in packet
    uint8_t      CAflag = 0;         // used in packet (or not at all)
    int16_t      DSCTy = 0;          // used in packet
    uint8_t      DGflag = 0;         // used for TDC
    int16_t      packetAddress = 0;  // used in packet

    TransportMode transportMode(void) const;
    AudioServiceComponentType audioType(void) const;
};

enum class EEPProtectionProfile {
    EEP_A,
    EEP_B,
};

enum class EEPProtectionLevel {
    EEP_1 = 1,
    EEP_2 = 2,
    EEP_3 = 3,
    EEP_4 = 4,
};

struct ProtectionSettings {
    bool     shortForm = false;

    // when short-form, UEP:
    int16_t  uepTableIndex = 0;
    int16_t  uepLevel = 0;

    // when long-form, EEP:
    EEPProtectionProfile eepProfile = EEPProtectionProfile::EEP_A;
    EEPProtectionLevel eepLevel = EEPProtectionLevel::EEP_3;
};

struct Subchannel {
    int32_t  subChId = -1;
    int32_t  startAddr = 0;
    int32_t  length = 0;
    bool     programmeNotData = true;

    ProtectionSettings protectionSettings;

    int16_t  language = 0;

    // For subchannels carrying packet-mode service components
    int16_t  fecScheme = 0; // 0=no FEC, 1=FEC, 2=Rfu, 3=Rfu

    // Calculate the effective subchannel bitrate
    int bitrate(void) const;

    // Calculate number of CUs this subchannel consumes
    int numCU(void) const;

    std::string protection(void) const;

    inline bool valid() const { return subChId != -1; }
};

#endif
