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

#include "welle-cli/jsonconvert.h"
#include "libs/json.hpp"

using namespace std;

static void to_json(nlohmann::json& j, const DabLabel& l)
{
    j["label"] = l.fig1_label_utf8();
    j["shortlabel"] = l.fig1_shortlabel_utf8();
    j["fig2label"] = l.fig2_label();
    j["fig2rfu"] = l.fig2_rfu;
    string extended_label_charset = "Unknown";
    switch (l.extended_label_charset) {
        case CharacterSet::EbuLatin: extended_label_charset = "EBU Latin (not allowed in FIG 2)"; break;
        case CharacterSet::UnicodeUcs2: extended_label_charset = "UCS2"; break;
        case CharacterSet::UnicodeUtf8: extended_label_charset = "UTF-8"; break;
        case CharacterSet::Undefined: extended_label_charset = "Undefined"; break;
    }
    j["fig2charset"] = extended_label_charset;
}


static void to_json(nlohmann::json& j, const SoftwareJson& s) {
    j = nlohmann::json{
        {"name", s.name},
        {"version", s.version},
        {"fftwindowplacement", s.fftwindowplacement},
        {"coarsecorrectorenabled", s.coarsecorrectorenabled},
        {"freqsyncmethod", s.freqsyncmethod},
        {"lastchannelchange", s.lastchannelchange}
    };
}

static void to_json(nlohmann::json& j, const HardwareJson& h) {
    j = nlohmann::json{
        {"name", h.name},
        {"gain", h.gain}
    };
}

static void to_json(nlohmann::json& j, const ReceiverJson& r) {
    j = nlohmann::json{
        {"hardware", r.hardware},
        {"software", r.software}
    };
}

static void to_json(nlohmann::json& j, const Subchannel& sub) {
    j = nlohmann::json{
        {"subchid", sub.subChId},
        {"bitrate", sub.bitrate()},
        {"cu", sub.numCU()},
        {"sad", sub.startAddr},
        {"protection", sub.protection()},
        {"language", sub.language},
        {"languagestring", DABConstants::getLanguageName(sub.language)}
    };
}

static void to_json(nlohmann::json& j, const ComponentJson& c) {
    j = nlohmann::json{
        {"componentnr", c.componentnr},
        {"primary", c.primary},
        {"caflag", c.caflag},
        {"transportmode", c.transportmode},
        {"label", c.label},
        {"subchannel", c.subchannel}
    };

    if (c.scid) {
        j["scid"] = *c.scid;
    }
    else {
        j["scid"] = nullptr;
    }

    if (c.ascty) {
        j["ascty"] = *c.ascty;
    }
    else {
        j["ascty"] = nullptr;
    }

    if (c.dscty) {
        j["dscty"] = *c.dscty;
    }
    else {
        j["dscty"] = nullptr;
    }
}

static void to_json(nlohmann::json& j, const ServiceJson& s) {
    j = nlohmann::json{
        {"sid", s.sid},
        {"programType", s.programType},
        {"ptystring", s.ptystring},
        {"language", s.language},
        {"languagestring", s.languagestring},
        {"label", s.label},
        {"components", s.components},
        {"channels", s.channels},
        {"samplerate", s.samplerate},
        {"mode", s.mode},
        {"mot", nlohmann::json{
            {"time", s.mot_time},
            {"lastchange", s.mot_lastchange}}},
        {"dls", nlohmann::json{
            {"label", s.dls_label},
            {"time", s.dls_time},
            {"lastchange", s.dls_lastchange}}},
        {"errorcounters", nlohmann::json{
            {"frameerrors", s.errorcounters_frameerrors},
            {"rserrors", s.errorcounters_rserrors},
            {"aacerrors", s.errorcounters_aacerrors},
            {"time", s.errorcounters_time}}}};

    if (s.xpaderror_haserror) {
        j["xpaderror"] = nlohmann::json{
            {"haserror", s.xpaderror_haserror},
            {"announcedlen", s.xpaderror_announcedlen},
            {"len", s.xpaderror_len},
            {"time", s.xpaderror_time}};
    }
    else {
        j["xpaderror"]["haserror"] = false;
    }

    if (s.url_mp3.empty()) {
        j["url_mp3"] = nullptr;
    }
    else {
        j["url_mp3"] = s.url_mp3;
    }

    if (s.audiolevel_present) {
        j["audiolevel"] = nlohmann::json{
            {"time", s.audiolevel_time},
            {"left", s.audiolevel_left},
            {"right", s.audiolevel_right}};
    }
    else {
        j["audiolevel"] = nullptr;
    }
}

static void to_json(nlohmann::json& j, const EnsembleJson& e) {
    j = nlohmann::json{
        {"label", e.label},
        {"id", e.id},
        {"ecc", e.ecc}
    };
}

static void to_json(nlohmann::json& j, const UTCJson& u) {
    j = nlohmann::json{
        {"year", u.year},
        {"month", u.month},
        {"day", u.day},
        {"hour", u.hour},
        {"minutes", u.minutes},
        {"lto", u.lto}
    };
}

static void to_json(nlohmann::json& j, const tii_measurement_t& tii) {
    j = nlohmann::json{
        {"comb", tii.comb},
        {"pattern", tii.pattern},
        {"delay", tii.delay_samples},
        {"delay_km", tii.getDelayKm()},
        {"error", tii.error}
    };
}

static void to_json(nlohmann::json& j, const PeakJson& peak)
{
    j = nlohmann::json{
        {"index", peak.index},
        {"value", 10.0f * log10(peak.value)}};
}


static void to_json(nlohmann::json& j, const MuxJson& mux) {
    j = nlohmann::json{
        {"receiver", mux.receiver},
        {"ensemble", mux.ensemble},
        {"services", mux.services},
        {"utctime", mux.utctime},
        {"messages", mux.messages},
        {"tii", mux.tii},
        {"cir_peaks", mux.cir_peaks}
    };

    j["demodulator"]["fic"]["numcrcerrors"] = mux.demodulator_fic_numcrcerrors;
    j["demodulator"]["snr"] = mux.demodulator_snr;
    j["demodulator"]["frequencycorrection"] = mux.demodulator_frequencycorrection;
}

std::string build_mux_json(const MuxJson& mux)
{
    nlohmann::json j = mux;
    return j.dump();
}
