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

// For Qt translation if Qt is exisiting
#ifdef QT_CORE_LIB
    #include <QtGlobal>
#else
    #define QT_TR_NOOP(x) (x)
#endif

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
    const auto fig2 = fig2_label();
    if (not fig2.empty()) {
        return fig2;
    }
    else {
        return fig1_label_utf8();
    }
}

string DabLabel::fig1_label_utf8() const
{
    return toUtf8StringUsingCharset(fig1_label.c_str(), charset);
}

string DabLabel::fig1_shortlabel_utf8() const
{
    const string shortlabel = flag_to_shortlabel(fig1_label, fig1_flag);
    return toUtf8StringUsingCharset(shortlabel.c_str(), charset);
}

void DabLabel::setCharset(uint8_t charset_id)
{
    charset = static_cast<CharacterSet>(charset_id);
}

string DabLabel::fig2_label() const
{
    vector<uint8_t> segments_cat;
    for (size_t i = 0; i < segment_count; i++) {
        if (segments.count(i) == 0) {
            return "";
        }
        else {
            const auto& s = segments.at(i);
            copy(s.begin(), s.end(), back_inserter(segments_cat));
        }
    }

    switch (extended_label_charset) {
        case CharacterSet::EbuLatin:
            std::clog << "DABConstants: FIG2 label encoded in EBU Latin is not allowed." << std::endl;
            return ""; // Fallback to FIG1
        case CharacterSet::UnicodeUtf8:
            return string(segments_cat.begin(), segments_cat.end());
        case CharacterSet::UnicodeUcs2:
            return toUtf8StringUsingCharset(
                    segments_cat.data(), CharacterSet::UnicodeUcs2, segments_cat.size());
        case CharacterSet::Undefined:
            return "";
    }
    throw logic_error("invalid extended label charset " + to_string((int)extended_label_charset));
}

const char* DABConstants::getProgramTypeName(int type)
{
    const char* typeName = "";
    switch (type) {
        case 0: typeName = ""; break;
        case 1: typeName = QT_TR_NOOP("News"); break;
        case 2: typeName = QT_TR_NOOP("Current Affairs"); break;
        case 3: typeName = QT_TR_NOOP("Information"); break;
        case 4: typeName = QT_TR_NOOP("Sport"); break;
        case 5: typeName = QT_TR_NOOP("Education"); break;
        case 6: typeName = QT_TR_NOOP("Drama"); break;
        case 7: typeName = QT_TR_NOOP("Arts"); break;
        case 8: typeName = QT_TR_NOOP("Science"); break;
        case 9: typeName = QT_TR_NOOP("Talk"); break;
        case 10: typeName = QT_TR_NOOP("Pop Music"); break;
        case 11: typeName = QT_TR_NOOP("Rock Music"); break;
        case 12: typeName = QT_TR_NOOP("Easy Listening"); break;
        case 13: typeName = QT_TR_NOOP("Light classical"); break;
        case 14: typeName = QT_TR_NOOP("Classical Music"); break;
        case 15: typeName = QT_TR_NOOP("Other Music"); break;
        case 16: typeName = QT_TR_NOOP("Weather"); break;
        case 17: typeName = QT_TR_NOOP("Finance"); break;
        case 18: typeName = QT_TR_NOOP("Children\'s"); break;
        case 19: typeName = QT_TR_NOOP("Factual"); break;
        case 20: typeName = QT_TR_NOOP("Religion"); break;
        case 21: typeName = QT_TR_NOOP("Phone In"); break;
        case 22: typeName = QT_TR_NOOP("Travel"); break;
        case 23: typeName = QT_TR_NOOP("Leisure"); break;
        case 24: typeName = QT_TR_NOOP("Jazz and Blues"); break;
        case 25: typeName = QT_TR_NOOP("Country Music"); break;
        case 26: typeName = QT_TR_NOOP("National Music"); break;
        case 27: typeName = QT_TR_NOOP("Oldies Music"); break;
        case 28: typeName = QT_TR_NOOP("Folk Music"); break;
        case 29: typeName = QT_TR_NOOP("Documentary"); break;
        case 30: typeName = QT_TR_NOOP("entry 30 not used"); break;
        case 31: typeName = QT_TR_NOOP("entry 31 not used"); break;
        default: typeName = QT_TR_NOOP("UNKNOWN");
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
        case 1: languageName = QT_TR_NOOP("Albanian"); break;
        case 2: languageName = QT_TR_NOOP("Breton"); break;
        case 3: languageName = QT_TR_NOOP("Catalan"); break;
        case 4: languageName = QT_TR_NOOP("Croatian"); break;
        case 5: languageName = QT_TR_NOOP("Welsh"); break;
        case 6: languageName = QT_TR_NOOP("Czech"); break;
        case 7: languageName = QT_TR_NOOP("Danish"); break;
        case 8: languageName = QT_TR_NOOP("German"); break;
        case 9: languageName = QT_TR_NOOP("English"); break;
        case 10: languageName = QT_TR_NOOP("Spanish"); break;
        case 11: languageName = QT_TR_NOOP("Esperanto"); break;
        case 12: languageName = QT_TR_NOOP("Estonian"); break;
        case 13: languageName = QT_TR_NOOP("Basque"); break;
        case 14: languageName = QT_TR_NOOP("Faroese"); break;
        case 15: languageName = QT_TR_NOOP("French"); break;
        case 16: languageName = QT_TR_NOOP("Frisian"); break;
        case 17: languageName = QT_TR_NOOP("Irish"); break;
        case 18: languageName = QT_TR_NOOP("Gaelic"); break;
        case 19: languageName = QT_TR_NOOP("Galician"); break;
        case 20: languageName = QT_TR_NOOP("Icelandic"); break;
        case 21: languageName = QT_TR_NOOP("Italian"); break;
        case 22: languageName = QT_TR_NOOP("Lappish"); break;
        case 23: languageName = QT_TR_NOOP("Latin"); break;
        case 24: languageName = QT_TR_NOOP("Latvian"); break;
        case 25: languageName = QT_TR_NOOP("Luxembourgian"); break;
        case 26: languageName = QT_TR_NOOP("Lithuanian"); break;
        case 27: languageName = QT_TR_NOOP("Hungarian"); break;
        case 28: languageName = QT_TR_NOOP("Maltese"); break;
        case 29: languageName = QT_TR_NOOP("Dutch"); break;
        case 30: languageName = QT_TR_NOOP("Norwegian"); break;
        case 31: languageName = QT_TR_NOOP("Occitan"); break;
        case 32: languageName = QT_TR_NOOP("Polish"); break;
        case 33: languageName = QT_TR_NOOP("Portuguese"); break;
        case 34: languageName = QT_TR_NOOP("Romanian"); break;
        case 35: languageName = QT_TR_NOOP("Romansh"); break;
        case 36: languageName = QT_TR_NOOP("Serbian"); break;
        case 37: languageName = QT_TR_NOOP("Slovak"); break;
        case 38: languageName = QT_TR_NOOP("Slovene"); break;
        case 39: languageName = QT_TR_NOOP("Finnish"); break;
        case 40: languageName = QT_TR_NOOP("Swedish"); break;
        case 41: languageName = QT_TR_NOOP("Turkish"); break;
        case 42: languageName = QT_TR_NOOP("Flemish"); break;
        case 43: languageName = QT_TR_NOOP("Walloon"); break;
        case 64: languageName = QT_TR_NOOP("Background sound/clean feed"); break;
        case 69: languageName = QT_TR_NOOP("Zulu"); break;
        case 70: languageName = QT_TR_NOOP("Vietnamese"); break;
        case 71: languageName = QT_TR_NOOP("Uzbek"); break;
        case 72: languageName = QT_TR_NOOP("Urdu"); break;
        case 73: languageName = QT_TR_NOOP("Ukranian"); break;
        case 74: languageName = QT_TR_NOOP("Thai"); break;
        case 75: languageName = QT_TR_NOOP("Telugu"); break;
        case 76: languageName = QT_TR_NOOP("Tatar"); break;
        case 77: languageName = QT_TR_NOOP("Tamil"); break;
        case 78: languageName = QT_TR_NOOP("Tadzhik"); break;
        case 79: languageName = QT_TR_NOOP("Swahili"); break;
        case 80: languageName = QT_TR_NOOP("Sranan Tongo"); break;
        case 81: languageName = QT_TR_NOOP("Somali"); break;
        case 82: languageName = QT_TR_NOOP("Sinhalese"); break;
        case 83: languageName = QT_TR_NOOP("Shona"); break;
        case 84: languageName = QT_TR_NOOP("Serbo-Croat"); break;
        case 85: languageName = QT_TR_NOOP("Rusyn"); break;
        case 86: languageName = QT_TR_NOOP("Russian"); break;
        case 87: languageName = QT_TR_NOOP("Quechua"); break;
        case 88: languageName = QT_TR_NOOP("Pushtu"); break;
        case 89: languageName = QT_TR_NOOP("Punjabi"); break;
        case 90: languageName = QT_TR_NOOP("Persian"); break;
        case 91: languageName = QT_TR_NOOP("Papiamento"); break;
        case 92: languageName = QT_TR_NOOP("Oriya"); break;
        case 93: languageName = QT_TR_NOOP("Nepali"); break;
        case 94: languageName = QT_TR_NOOP("Ndebele"); break;
        case 95: languageName = QT_TR_NOOP("Marathi"); break;
        case 96: languageName = QT_TR_NOOP("Moldavian"); break;
        case 97: languageName = QT_TR_NOOP("Malaysian"); break;
        case 98: languageName = QT_TR_NOOP("Malagasay"); break;
        case 99: languageName = QT_TR_NOOP("Macedonian"); break;
        case 100: languageName = QT_TR_NOOP("Laotian"); break;
        case 101: languageName = QT_TR_NOOP("Korean"); break;
        case 102: languageName = QT_TR_NOOP("Khmer"); break;
        case 103: languageName = QT_TR_NOOP("Kazakh"); break;
        case 104: languageName = QT_TR_NOOP("Kannada"); break;
        case 105: languageName = QT_TR_NOOP("Japanese"); break;
        case 106: languageName = QT_TR_NOOP("Indonesian"); break;
        case 107: languageName = QT_TR_NOOP("Hindi"); break;
        case 108: languageName = QT_TR_NOOP("Hebrew"); break;
        case 109: languageName = QT_TR_NOOP("Hausa"); break;
        case 110: languageName = QT_TR_NOOP("Gurani"); break;
        case 111: languageName = QT_TR_NOOP("Gujurati"); break;
        case 112: languageName = QT_TR_NOOP("Greek"); break;
        case 113: languageName = QT_TR_NOOP("Georgian"); break;
        case 114: languageName = QT_TR_NOOP("Fulani"); break;
        case 115: languageName = QT_TR_NOOP("Dari"); break;
        case 116: languageName = QT_TR_NOOP("Chuvash"); break;
        case 117: languageName = QT_TR_NOOP("Chinese"); break;
        case 118: languageName = QT_TR_NOOP("Burmese"); break;
        case 119: languageName = QT_TR_NOOP("Bulgarian"); break;
        case 120: languageName = QT_TR_NOOP("Bengali"); break;
        case 121: languageName = QT_TR_NOOP("Belorussian"); break;
        case 122: languageName = QT_TR_NOOP("Bambora"); break;
        case 123: languageName = QT_TR_NOOP("Azerbaijani"); break;
        case 124: languageName = QT_TR_NOOP("Assamese"); break;
        case 125: languageName = QT_TR_NOOP("Armenian"); break;
        case 126: languageName = QT_TR_NOOP("Arabic"); break;
        case 127: languageName = QT_TR_NOOP("Amharic"); break;
        default: languageName = QT_TR_NOOP("UNKNOWN");
                 std::clog << "DABConstants: Unknown language type: "
                     << language << std::endl;
                 break;
    }

    return languageName;
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
        prot = "UEP " + to_string((int)ps.uepLevel);
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

