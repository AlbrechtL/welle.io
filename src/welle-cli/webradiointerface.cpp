/*
 *    Copyright (C) 2018
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

#include <future>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <utility>
#include <cstdio>
#include <errno.h>
#include "welle-cli/webradiointerface.h"
#include "libs/json.hpp"

using namespace std;

static const char* http_ok = "HTTP/1.0 200 OK\r\n";
static const char* http_contentlength = "Content-Length: 0\r\n";
static const char* http_contenttype_wav = "Content-type: audio/wav\r\n";
static const char* http_contenttype_json = "Content-type: application/json\r\n";
static const char* http_nocache = "Cache-Control: no-cache\r\n";

static string sid_to_hex(uint32_t serviceId)
{
    std::stringstream sidstream;
    sidstream << "0x" <<
        std::setfill('0') << std::setw(4) <<
        std::hex << serviceId;
    return sidstream.str();
}

bool WebRadioInterface::dispatch_client(Socket s)
{
    vector<char> buf(1025);
    if (not s.valid()) {
        cerr << "socket in dispatcher not valid!" << endl;
        return false;
    }
    ssize_t ret = s.recv(buf.data(), buf.size()-1, 0);

    if (ret == 0) {
        return false;
    }
    else if (ret == -1) {
        string errstr = strerror(errno);
        cerr << "recv error " << errstr << endl;
        return false;
    }
    else {
        buf.resize(ret);
        string request(buf.begin(), buf.end());

        const regex regex_mux_json(R"(^GET [/]mux.json HTTP)");

        std::smatch match_mux_json;
        bool is_mux_json = regex_search(request, match_mux_json, regex_mux_json);
        if (is_mux_json) {
            nlohmann::json j;
            j["Ensemble"]["Name"] = rx->getEnsembleName();

            nlohmann::json j_services;
            for (const auto& s : rx->getServiceList()) {
                nlohmann::json j_srv = {
                    {"SId", sid_to_hex(s.serviceId)},
                    {"Label", s.serviceLabel.label}};

                nlohmann::json j_components;

                for (const auto& sc : rx->getComponents(s)) {
                    nlohmann::json j_sc = {
                        {"ComponentNr", sc.componentNr},
                        {"ASCTy",
                        (sc.audioType() == AudioServiceComponentType::DAB ? "DAB" :
                         sc.audioType() == AudioServiceComponentType::DABPlus ? "DAB+" :
                         "unknown") } };

                    const auto& sub = rx->getSubchannel(sc);
                    j_sc["Subchannel"] = {
                        { "Subchannel_id", sub.subChId},
                        { "Bitrate", sub.bitrate()},
                        { "SAd", sub.startAddr}};

                    j_components.push_back(j_sc);
                }
                j_srv["Components"] = j_components;

                j_services.push_back(j_srv);
            }
            j["Services"] = j_services;

            {
                lock_guard<mutex> lock(mut);

                nlohmann::json j_utc = {
                    {"year", last_dateTime.year},
                    {"month", last_dateTime.month},
                    {"day", last_dateTime.day},
                    {"hour", last_dateTime.hour},
                    {"minutes", last_dateTime.minutes}
                };

                j["UTCTime"] = j_utc;

                j["SNR"] = last_snr;
                j["FrequencyCorrection"] =
                    last_fine_correction + last_coarse_correction;

            }
            for (const auto& tii : getTiiStats()) {
                j["TII"].push_back({
                        {"comb", tii.comb},
                        {"pattern", tii.pattern},
                        {"delay", tii.delay_samples},
                        {"delay_km", tii.getDelayKm()},
                        {"error", tii.error}});
            }


            string headers = http_ok;
            headers += http_contenttype_json;
            headers += http_nocache;
            headers += "\r\n";
            s.send(headers.data(), headers.size(), MSG_NOSIGNAL);

            const auto json_str = j.dump();

            s.send(json_str.c_str(), json_str.size(), MSG_NOSIGNAL);
            return true;
        }

        const regex regex_wav(R"(^HEAD [/]wav[/]([^ ]+) HTTP)");
        std::smatch match_wav;
        bool is_wav = regex_search(request, match_wav, regex_wav);
        if (is_wav) {
            const string stream = match_wav[1];
            for (const auto& s : rx->getServiceList()) {
                if (sid_to_hex(s.serviceId) == stream) {
#warning "TODO"
                }
            }
        }

        cerr << "Could not understand request" << endl;
    }
    return false;
}

WebRadioInterface::WebRadioInterface(CVirtualInput& in, int port)
{
    bool success = serverSocket.bind(port);
    if (success) {
        success = serverSocket.listen();
    }

    if (success) {
        rx = make_unique<RadioReceiver>(*this, in);
    }

    if (not rx) {
        throw runtime_error("Could not initialise WebRadioInterface");
    }

    rx->restart(false);
}

void WebRadioInterface::serve()
{
    deque<future<bool> > running_connections;

    while (true) {
        auto client = serverSocket.accept();

        running_connections.push_back(async(launch::async,
                    &WebRadioInterface::dispatch_client, this, move(client)));

        deque<future<bool> > still_running_connections;
        for (auto& fut : running_connections) {
            if (fut.valid()) {
                fut.get();
            }
            else {
                still_running_connections.push_back(move(fut));
            }
        }
        running_connections = move(still_running_connections);
    }
}

void WebRadioInterface::onSNR(int snr)
{
    lock_guard<mutex> lock(mut);
    last_snr = snr;
}

void WebRadioInterface::onFrequencyCorrectorChange(int fine, int coarse)
{
    lock_guard<mutex> lock(mut);
    last_fine_correction = fine;
    last_coarse_correction = coarse;
}

void WebRadioInterface::onSyncChange(char isSync) { (void)isSync; }
void WebRadioInterface::onSignalPresence(bool isSignal) { (void)isSignal; }
void WebRadioInterface::onServiceDetected(uint32_t sId, const std::string& label)
{(void)sId; (void)label; }
void WebRadioInterface::onNewEnsembleName(const std::string& name) {(void)name; }

void WebRadioInterface::onDateTimeUpdate(const dab_date_time_t& dateTime)
{
    lock_guard<mutex> lock(mut);
    last_dateTime = dateTime;
}

void WebRadioInterface::onFICDecodeSuccess(bool isFICCRC) {(void)isFICCRC; }

void WebRadioInterface::onNewImpulseResponse(std::vector<float>&& data)
{
    lock_guard<mutex> lock(mut);
    last_CIR = move(data);
}

void WebRadioInterface::onNewNullSymbol(std::vector<DSPCOMPLEX>&& data)
{
    lock_guard<mutex> lock(mut);
    last_NULL = move(data);
}

void WebRadioInterface::onConstellationPoints(std::vector<DSPCOMPLEX>&& data)
{
    lock_guard<mutex> lock(mut);
    last_constellation = move(data);
}

void WebRadioInterface::onMessage(message_level_t level, const std::string& text)
{
    lock_guard<mutex> lock(mut);
    pending_messages.emplace_back(level, text);
}

void WebRadioInterface::onTIIMeasurement(tii_measurement_t&& m)
{
    lock_guard<mutex> lock(mut);
    auto& l = tiis[make_pair(m.comb, m.pattern)];
    l.push_back(move(m));

    if (l.size() > 20) {
        l.pop_front();
    }
}

list<tii_measurement_t> WebRadioInterface::getTiiStats()
{
    list<tii_measurement_t> l;

    lock_guard<mutex> lock(mut);
    for (const auto& cp_list : tiis) {
        const auto comb = cp_list.first.first;
        const auto pattern = cp_list.first.second;

        if (cp_list.second.size() < 5) {
            cerr << "Skip TII " << comb << " " << pattern << " with " <<
                cp_list.second.size() << " measurements" << endl;
            continue;
        }

        tii_measurement_t avg;
        avg.comb = comb;
        avg.pattern = pattern;
        vector<int> delays;
        double error = 0.0;
        size_t len = 0;
        for (const auto& meas : cp_list.second) {
            delays.push_back(meas.delay_samples);
            error += meas.error;
            len++;
        }

        avg.error = error / len;

        // Calculate the median
        std::nth_element(delays.begin(), delays.begin() + len/2, delays.end());
        avg.delay_samples = delays[len/2];

        l.push_back(move(avg));
    }

    using namespace std::chrono;
    const auto now = steady_clock::now();
    // Remove a single entry every second to make the flukes
    // disappear
    if (time_last_tiis_clean + seconds(1) > now) {
        for (auto& cp_list : tiis) {
            cp_list.second.pop_front();
        }

        time_last_tiis_clean = now;
    }

    return l;
}
