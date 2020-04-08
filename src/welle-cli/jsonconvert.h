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
    bool coarsecorrectorenabled = false;
    std::string freqsyncmethod;
    std::time_t lastchannelchange = 0;
};

struct HardwareJson {
    std::string name;
    float gain = 0.0f;
};

struct ReceiverJson {
    HardwareJson hardware;
    SoftwareJson software;
};


struct ComponentJson {
    int16_t componentnr = 0;
    bool primary = false;
    bool caflag = false;

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
    int16_t programType = 0;
    std::string ptystring;
    int16_t language = 0;
    std::string languagestring;
    DabLabel label;

    std::vector<ComponentJson> components;

    std::string url_mp3;

    bool audiolevel_present = false;
    std::time_t audiolevel_time = 0;
    int audiolevel_left = -1;
    int audiolevel_right = -1;

    int channels = 0;
    int samplerate = 0;
    std::string mode;

    std::time_t mot_time = 0;
    std::time_t mot_lastchange = 0;

    std::string dls_label;
    std::time_t dls_time = 0;
    std::time_t dls_lastchange = 0;

    size_t errorcounters_frameerrors = 0;
    size_t errorcounters_rserrors = 0;
    size_t errorcounters_aacerrors = 0;
    std::time_t errorcounters_time = 0;

    bool xpaderror_haserror = false;
    size_t xpaderror_announcedlen = 0;
    size_t xpaderror_len = 0;
    std::time_t xpaderror_time = 0;
};

struct EnsembleJson {
    DabLabel label;
    std::string id;
    std::string ecc;
};

struct UTCJson {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minutes = 0;
    double lto = 0.0;
};

struct PeakJson {
    int index = -1;
    float value = -1e30f;
};

struct MuxJson {
    ReceiverJson receiver;
    EnsembleJson ensemble;
    std::vector<ServiceJson> services;
    size_t demodulator_fic_numcrcerrors = 0;
    UTCJson utctime;
    std::vector<std::string> messages;

    double demodulator_snr = 0.0;
    double demodulator_frequencycorrection = 0.0;

    std::list<tii_measurement_t> tii;
    std::vector<PeakJson> cir_peaks;
};

std::string build_mux_json(const MuxJson& mux);
