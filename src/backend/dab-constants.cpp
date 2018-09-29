/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
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

#include "dab-constants.h"
#include <iostream>
#include <exception>
#include <sstream>

using namespace std;

// Table ETSI EN 300 401 Page 50
// Table is copied from the work of Michael Hoehn
const int ProtLevel[64][3] = {
    {16,5,32},  // Index 0
    {21,4,32},
    {24,3,32},
    {29,2,32},
    {35,1,32},  // Index 4
    {24,5,48},
    {29,4,48},
    {35,3,48},
    {42,2,48},
    {52,1,48},  // Index 9
    {29,5,56},
    {35,4,56},
    {42,3,56},
    {52,2,56},
    {32,5,64},  // Index 14
    {42,4,64},
    {48,3,64},
    {58,2,64},
    {70,1,64},
    {40,5,80},  // Index 19
    {52,4,80},
    {58,3,80},
    {70,2,80},
    {84,1,80},
    {48,5,96},  // Index 24
    {58,4,96},
    {70,3,96},
    {84,2,96},
    {104,1,96},
    {58,5,112}, // Index 29
    {70,4,112},
    {84,3,112},
    {104,2,112},
    {64,5,128},
    {84,4,128}, // Index 34
    {96,3,128},
    {116,2,128},
    {140,1,128},
    {80,5,160},
    {104,4,160},    // Index 39
    {116,3,160},
    {140,2,160},
    {168,1,160},
    {96,5,192},
    {116,4,192},    // Index 44
    {140,3,192},
    {168,2,192},
    {208,1,192},
    {116,5,224},
    {140,4,224},    // Index 49
    {168,3,224},
    {208,2,224},
    {232,1,224},
    {128,5,256},
    {168,4,256},    // Index 54
    {192,3,256},
    {232,2,256},
    {280,1,256},
    {160,5,320},
    {208,4,320},    // index 59
    {280,2,320},
    {192,5,384},
    {280,3,384},
    {416,1,384}};


static std::string flag_to_shortlabel(const std::string& label, uint16_t flag)
{
    stringstream shortlabel;
    for (size_t i = 0; i < label.size(); ++i) {
        if (flag & 0x8000 >> i) {
            shortlabel << label[i];
        }
    }

    return shortlabel.str();
}

string DabLabel::utf8_label() const
{
    return toUtf8StringUsingCharset(raw_label.c_str(), charset);
}

string DabLabel::utf8_shortlabel() const
{
    const string shortlabel = flag_to_shortlabel(raw_label, flag);
    return toUtf8StringUsingCharset(shortlabel.c_str(), charset);
}

void DabLabel::setCharset(uint8_t charset_id)
{
    charset = static_cast<CharacterSet>(charset_id);
}

const char* DABConstants::getProgramTypeName(int type)
{
    const char* typeName = "";
    switch (type) {
        case 0: typeName = ""; break;
        case 1: typeName = "News"; break;
        case 2: typeName = "Current Affairs"; break;
        case 3: typeName = "Information"; break;
        case 4: typeName = "Sport"; break;
        case 5: typeName = "Education"; break;
        case 6: typeName = "Drama"; break;
        case 7: typeName = "Arts"; break;
        case 8: typeName = "Science"; break;
        case 9: typeName = "Talk"; break;
        case 10: typeName = "Pop Music"; break;
        case 11: typeName = "Rock Music"; break;
        case 12: typeName = "Easy Listening"; break;
        case 13: typeName = "Light classical"; break;
        case 14: typeName = "Classical Music"; break;
        case 15: typeName = "Other Music"; break;
        case 16: typeName = "Weather"; break;
        case 17: typeName = "Finance"; break;
        case 18: typeName = "Children\'s"; break;
        case 19: typeName = "Factual"; break;
        case 20: typeName = "Religion"; break;
        case 21: typeName = "Phone In"; break;
        case 22: typeName = "Travel"; break;
        case 23: typeName = "Leisure"; break;
        case 24: typeName = "Jazz and Blues"; break;
        case 25: typeName = "Country Music"; break;
        case 26: typeName = "National Music"; break;
        case 27: typeName = "Oldies Music"; break;
        case 28: typeName = "Folk Music"; break;
        case 29: typeName = "entry 29 not used"; break;
        case 30: typeName = "entry 30 not used"; break;
        case 31: typeName = "entry 31 not used"; break;
        default: typeName = "UNKNOWN";
                 std::clog << "DABConstants: Unknown program type" << std::endl;
                 break;
    }

    return typeName;
}

const char* DABConstants::getLanguageName(int language)
{
    const char* languageName = "";

    switch (language) {
        case 0: languageName = ""; break;
        case 1: languageName = "Albanian"; break;
        case 2: languageName = "Breton"; break;
        case 3: languageName = "Catalan"; break;
        case 4: languageName = "Croatian"; break;
        case 5: languageName = "Welsh"; break;
        case 6: languageName = "Czech"; break;
        case 7: languageName = "Danish"; break;
        case 8: languageName = "German"; break;
        case 9: languageName = "English"; break;
        case 10: languageName = "Spanish"; break;
        case 11: languageName = "Esperanto"; break;
        case 12: languageName = "Estonian"; break;
        case 13: languageName = "Basque"; break;
        case 14: languageName = "Faroese"; break;
        case 15: languageName = "French"; break;
        case 16: languageName = "Frisian"; break;
        case 17: languageName = "Irish"; break;
        case 18: languageName = "Gaelic"; break;
        case 19: languageName = "Galician"; break;
        case 20: languageName = "Icelandic"; break;
        case 21: languageName = "Italian"; break;
        case 22: languageName = "Lappish"; break;
        case 23: languageName = "Latin"; break;
        case 24: languageName = "Latvian"; break;
        case 25: languageName = "Luxembourgian"; break;
        case 26: languageName = "Lithuanian"; break;
        case 27: languageName = "Hungarian"; break;
        case 28: languageName = "Maltese"; break;
        case 29: languageName = "Dutch"; break;
        case 30: languageName = "Norwegian"; break;
        case 31: languageName = "Occitan"; break;
        case 32: languageName = "Polish"; break;
        case 33: languageName = "Portuguese"; break;
        case 34: languageName = "Romanian"; break;
        case 35: languageName = "Romansh"; break;
        case 36: languageName = "Serbian"; break;
        case 37: languageName = "Slovak"; break;
        case 38: languageName = "Slovene"; break;
        case 39: languageName = "Finnish"; break;
        case 40: languageName = "Swedish"; break;
        case 41: languageName = "Turkish"; break;
        case 42: languageName = "Flemish"; break;
        case 43: languageName = "Walloon"; break;
        case 64: languageName = "Background sound/clean feed"; break;
        case 69: languageName = "Zulu"; break;
        case 70: languageName = "Vietnamese"; break;
        case 71: languageName = "Uzbek"; break;
        case 72: languageName = "Urdu"; break;
        case 73: languageName = "Ukranian"; break;
        case 74: languageName = "Thai"; break;
        case 75: languageName = "Telugu"; break;
        case 76: languageName = "Tatar"; break;
        case 77: languageName = "Tamil"; break;
        case 78: languageName = "Tadzhik"; break;
        case 79: languageName = "Swahili"; break;
        case 80: languageName = "Sranan Tongo"; break;
        case 81: languageName = "Somali"; break;
        case 82: languageName = "Sinhalese"; break;
        case 83: languageName = "Shona"; break;
        case 84: languageName = "Serbo-Croat"; break;
        case 85: languageName = "Rusyn"; break;
        case 86: languageName = "Russian"; break;
        case 87: languageName = "Quechua"; break;
        case 88: languageName = "Pushtu"; break;
        case 89: languageName = "Punjabi"; break;
        case 90: languageName = "Persian"; break;
        case 91: languageName = "Papiamento"; break;
        case 92: languageName = "Oriya"; break;
        case 93: languageName = "Nepali"; break;
        case 94: languageName = "Ndebele"; break;
        case 95: languageName = "Marathi"; break;
        case 96: languageName = "Moldavian"; break;
        case 97: languageName = "Malaysian"; break;
        case 98: languageName = "Malagasay"; break;
        case 99: languageName = "Macedonian"; break;
        case 100: languageName = "Laotian"; break;
        case 101: languageName = "Korean"; break;
        case 102: languageName = "Khmer"; break;
        case 103: languageName = "Kazakh"; break;
        case 104: languageName = "Kannada"; break;
        case 105: languageName = "Japanese"; break;
        case 106: languageName = "Indonesian"; break;
        case 107: languageName = "Hindi"; break;
        case 108: languageName = "Hebrew"; break;
        case 109: languageName = "Hausa"; break;
        case 110: languageName = "Gurani"; break;
        case 111: languageName = "Gujurati"; break;
        case 112: languageName = "Greek"; break;
        case 113: languageName = "Georgian"; break;
        case 114: languageName = "Fulani"; break;
        case 115: languageName = "Dari"; break;
        case 116: languageName = "Chuvash"; break;
        case 117: languageName = "Chinese"; break;
        case 118: languageName = "Burmese"; break;
        case 119: languageName = "Bulgarian"; break;
        case 120: languageName = "Bengali"; break;
        case 121: languageName = "Belorussian"; break;
        case 122: languageName = "Bambora"; break;
        case 123: languageName = "Azerbaijani"; break;
        case 124: languageName = "Assamese"; break;
        case 125: languageName = "Armenian"; break;
        case 126: languageName = "Arabic"; break;
        case 127: languageName = "Amharic"; break;
        default: languageName = "UNKNOWN";
                 std::clog << "DABConstants: Unknown language type: "
                     << language << std::endl;
                 break;
    }

    return languageName;
}


DABParams::DABParams()
{
    setMode(1);
}

DABParams::DABParams(int mode)
{
    setMode(mode);
}

void DABParams::setMode(int mode)
{
    switch (mode)
    {
        case 1:
            dabMode = 1;
            L = 76;
            K = 1536;
            T_F = 196608;
            T_null = 2656;
            T_s = 2552;
            T_u = 2048;
            guardLength = 504;
            carrierDiff = 1000;
            break;

        case 2:
            dabMode = 2;
            L = 76;
            K = 384;
            T_null = 664;
            T_F = 49152;
            T_s = 638;
            T_u = 512;
            guardLength = 126;
            carrierDiff = 4000;
            break;

        case 3:
            dabMode = 3;
            L = 153;
            K = 192;
            T_F = 49152;
            T_null = 345;
            T_s = 319;
            T_u = 256;
            guardLength = 63;
            carrierDiff = 2000;
            break;

        case 4:
            dabMode = 4;
            L = 76;
            K = 768;
            T_F = 98304;
            T_null = 1328;
            T_s = 1276;
            T_u = 1024;
            guardLength = 252;
            carrierDiff = 2000;
            break;

        default:
            throw out_of_range("Unknown mode " + to_string(mode));
    }
}

int Subchannel::bitrate() const
{
    const auto& ps = protectionSettings;
    if (ps.shortForm) {
        return ProtLevel[ps.uepTableIndex][2];
    }
    else {  // EEP
        switch (ps.eepProfile) {
            case EEPProtectionProfile::EEP_A:
                switch (ps.eepLevel) {
                    case EEPProtectionLevel::EEP_1:
                        return length / 12 * 8;
                    case EEPProtectionLevel::EEP_2:
                        return length / 8 * 8;
                    case EEPProtectionLevel::EEP_3:
                        return length / 6 * 8;
                    case EEPProtectionLevel::EEP_4:
                        return length / 4 * 8;
                }
                break;
            case EEPProtectionProfile::EEP_B:
                switch (ps.eepLevel) {
                    case EEPProtectionLevel::EEP_1:
                        return length / 27 * 32;
                    case EEPProtectionLevel::EEP_2:
                        return length / 21 * 32;
                    case EEPProtectionLevel::EEP_3:
                        return length / 18 * 32;
                    case EEPProtectionLevel::EEP_4:
                        return length / 15 * 32;
                }
                break;
        }
    }

    throw std::runtime_error("Unsupported protection");
}

int Subchannel::numCU() const
{
    const auto& ps = protectionSettings;
    if (ps.shortForm) {
        return ProtLevel[ps.uepTableIndex][0];
    }
    else {
        switch (ps.eepProfile) {
            case EEPProtectionProfile::EEP_A:
                switch (ps.eepLevel) {
                    case EEPProtectionLevel::EEP_1:
                        return (bitrate() * 12) >> 3;
                    case EEPProtectionLevel::EEP_2:
                        return bitrate();
                    case EEPProtectionLevel::EEP_3:
                        return (bitrate() * 6) >> 3;
                    case EEPProtectionLevel::EEP_4:
                        return (bitrate() >> 1);
                }
                break;
            case EEPProtectionProfile::EEP_B:
                switch (ps.eepLevel) {
                    case EEPProtectionLevel::EEP_1:
                        return (bitrate() * 27) >> 5;
                    case EEPProtectionLevel::EEP_2:
                        return (bitrate() * 21) >> 5;
                    case EEPProtectionLevel::EEP_3:
                        return (bitrate() * 18) >> 5;
                    case EEPProtectionLevel::EEP_4:
                        return (bitrate() * 15) >> 5;
                }
                break;
        }
    }
    return -1;
}

string Subchannel::protection() const
{
    string prot;
    const auto& ps = protectionSettings;
    if (ps.shortForm) {
        prot = "UEP " + to_string(ps.uepTableIndex);
    }
    else {  // EEP
        prot = "EEP ";
        switch (ps.eepProfile) {
            case EEPProtectionProfile::EEP_A:
                prot += to_string((int)ps.eepLevel) + "-A";
                break;
            case EEPProtectionProfile::EEP_B:
                prot += to_string((int)ps.eepLevel) + "-B";
                break;
        }
    }
    return prot;
}

TransportMode ServiceComponent::transportMode() const
{
    if (TMid == 0) {
        return TransportMode::Audio;
    }
    else if (TMid == 1) {
        return TransportMode::StreamData;
    }
    else if (TMid == 2) {
        return TransportMode::FIDC;
    }
    else if (TMid == 3) {
        return TransportMode::PacketData;
    }
    throw std::logic_error("Illegal TMid!");
}

AudioServiceComponentType ServiceComponent::audioType() const
{
    if (ASCTy == 0) {
        return AudioServiceComponentType::DAB;
    }
    else if (ASCTy == 63) {
        return AudioServiceComponentType::DABPlus;
    }
    else {
        return AudioServiceComponentType::Unknown;
    }
}

