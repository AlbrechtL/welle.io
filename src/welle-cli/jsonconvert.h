/*
 *    Copyright (C) 2020
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
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
#pragma once

#include <string>
#include <list>
#include <vector>
#include <memory>
#include <ctime>
#include "dab-constants.h"
#include "backend/radio-controller.h"

struct SoftwareJson {
    std::string name;
    std::string version;
    std::string fftwindowplacement;
    bool coarsecorrectorenabled;
    std::string freqsyncmethod;
    std::time_t lastchannelchange;
};

struct HardwareJson {
    std::string name;
    float gain;
};

struct ReceiverJson {
    HardwareJson hardware;
    SoftwareJson software;
};


struct ComponentJson {
    int16_t componentnr;
    bool primary;
    bool caflag;

    // std::optional would be preferable
    std::unique_ptr<uint16_t> scid;
    std::unique_ptr<std::string> ascty;
    std::unique_ptr<uint16_t> dscty;
    std::string transportmode;

    DabLabel label;

    Subchannel subchannel;
};

struct ServiceJson {
    std::string sid;
    int16_t programType;
    std::string ptystring;
    int16_t language;
    std::string languagestring;
    DabLabel label;

    std::vector<ComponentJson> components;

    std::string url_mp3;

    bool audiolevel_present;
    std::time_t audiolevel_time;
    int audiolevel_left;
    int audiolevel_right;

    int channels;
    int samplerate;
    std::string mode;

    std::time_t mot_time;
    std::time_t mot_lastchange;

    std::string dls_label;
    std::time_t dls_time;
    std::time_t dls_lastchange;

    size_t errorcounters_frameerrors;
    size_t errorcounters_rserrors;
    size_t errorcounters_aacerrors;
    std::time_t errorcounters_time;

    bool xpaderror_haserror;
    size_t xpaderror_announcedlen;
    size_t xpaderror_len;
    std::time_t xpaderror_time;
};

struct EnsembleJson {
    DabLabel label;
    std::string id;
    std::string ecc;
};

struct UTCJson {
    int year;
    int month;
    int day;
    int hour;
    int minutes;
    double lto;
};

struct PeakJson {
    int index = -1;
    float value = -1e30f;
};

struct MuxJson {
    ReceiverJson receiver;
    EnsembleJson ensemble;
    std::vector<ServiceJson> services;
    size_t demodulator_fic_numcrcerrors;
    UTCJson utctime;
    std::vector<std::string> messages;

    double demodulator_snr;
    double demodulator_frequencycorrection;

    std::list<tii_measurement_t> tii;
    std::vector<PeakJson> peaks;
};

std::string build_mux_json(const MuxJson& mux);
