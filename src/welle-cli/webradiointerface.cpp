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
static const char* http_404 = "HTTP/1.0 404 Not Found\r\n";
static const char* http_503 = "HTTP/1.0 503 Service Unavailable\r\n";
static const char* http_contenttype_mp3 = "Content-Type: audio/mpeg\r\n";
static const char* http_contenttype_text = "Content-Type: text/plain\r\n";
static const char* http_contenttype_json =
        "Content-Type: application/json; charset=utf-8\r\n";

static const char* http_contenttype_html =
        "Content-Type: Content-Type: text/html; charset=utf-8\r\n";

static const char* http_nocache = "Cache-Control: no-cache\r\n";

static string sid_to_hex(uint32_t serviceId)
{
    std::stringstream sidstream;
    sidstream << "0x" <<
        std::setfill('0') << std::setw(4) <<
        std::hex << serviceId;
    return sidstream.str();
}

ProgrammeSender::ProgrammeSender(Socket&& s) :
    s(move(s))
{
}

void ProgrammeSender::cancel()
{
    s.close();
    std::unique_lock<std::mutex> lock(mutex);
    running = false;
    lock.unlock();
    cv.notify_all();
}

bool ProgrammeSender::send_mp3(const std::vector<uint8_t>& mp3Data)
{
    if (not s.valid()) {
        return false;
    }

    const int flags = MSG_NOSIGNAL;

    ssize_t ret = s.send(mp3Data.data(), mp3Data.size(), flags);
    if (ret == -1) {
        cancel();
        return false;
    }

    return true;
}

void ProgrammeSender::wait_for_termination()
{
    std::unique_lock<std::mutex> lock(mutex);
    while (running) {
        cv.wait(lock);
    }
}

WebProgrammeHandler::WebProgrammeHandler(uint32_t serviceId) :
    serviceId(serviceId)
{
    time_label = chrono::system_clock::now();
    time_mot = chrono::system_clock::now();
}

WebProgrammeHandler::WebProgrammeHandler(WebProgrammeHandler&& other) :
    serviceId(other.serviceId),
    senders(move(other.senders))
{
    time_label = chrono::system_clock::now();
    time_mot = chrono::system_clock::now();
}

void WebProgrammeHandler::registerSender(ProgrammeSender *sender)
{
    std::unique_lock<std::mutex> lock(senders_mutex);
    senders.push_back(sender);
}

void WebProgrammeHandler::removeSender(ProgrammeSender *sender)
{
    std::unique_lock<std::mutex> lock(senders_mutex);
    senders.remove(sender);
}

void WebProgrammeHandler::onFrameErrors(int frameErrors)
{
    last_frameErrors = frameErrors;
}

void WebProgrammeHandler::onNewAudio(std::vector<int16_t>&& audioData,
                int sampleRate, bool isStereo)
{
    stereo = isStereo;
    rate = sampleRate;

    const int channels = stereo ? 2 : 1;

    if (not lame_initialised) {
        lame_set_in_samplerate(lame.lame, rate);
        lame_set_num_channels(lame.lame, channels);
        lame_set_VBR(lame.lame, vbr_default);
        lame_set_VBR_q(lame.lame, 2);
        lame_init_params(lame.lame);
        lame_initialised = true;
    }

    vector<uint8_t> mp3buf(8192);

    int written = lame_encode_buffer_interleaved(lame.lame,
            audioData.data(), audioData.size()/channels,
            mp3buf.data(), mp3buf.size());

    if (written < 0) {
        cerr << "Failed to encode mp3: " << written << endl;
    }
    else if (written > (ssize_t)mp3buf.size()) {
        cerr << "mp3 encoder wrote more than buffer size!" << endl;
    }
    else {
        mp3buf.resize(written);

        std::unique_lock<std::mutex> lock(senders_mutex);
        for (auto *sender : senders) {
            bool success = sender->send_mp3(mp3buf);
            if (not success) {
                cerr << "Failed to send audio for " << serviceId << endl;
            }
        }
    }
}

void WebProgrammeHandler::onRsErrors(int rsErrors)
{
    last_rsErrors = rsErrors;
}

void WebProgrammeHandler::onAacErrors(int aacErrors)
{
    last_aacErrors = aacErrors;
}

void WebProgrammeHandler::onNewDynamicLabel(const string& label)
{
    std::unique_lock<std::mutex> lock(pad_mutex);
    last_label_valid = true;
    time_label = chrono::system_clock::now();
    last_label = label;
}

void WebProgrammeHandler::onMOT(const std::vector<uint8_t>& data, int subtype)
{
    std::unique_lock<std::mutex> lock(pad_mutex);
    last_mot_valid = true;
    time_mot = chrono::system_clock::now();
    last_mot = data;
    last_subtype = subtype;
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

        const regex regex_index(R"(^GET [/] HTTP)");
        std::smatch match_index;
        bool is_index = regex_search(request, match_index, regex_index);
        if (is_index) {
            FILE *fd = fopen("index.html", "r");
            if (fd) {
                string headers = http_ok;
                headers += http_contenttype_html;
                headers += http_nocache;
                headers += "\r\n";
                s.send(headers.data(), headers.size(), MSG_NOSIGNAL);

                vector<char> data(1024);
                size_t ret = 0;
                do {
                    ret = fread(data.data(), 1, data.size(), fd);
                    s.send(data.data(), ret, MSG_NOSIGNAL);
                } while (ret > 0);

                fclose(fd);
                return true;
            }
        }

        const regex regex_mux_json(R"(^GET [/]mux.json HTTP)");

        std::smatch match_mux_json;
        bool is_mux_json = regex_search(request, match_mux_json, regex_mux_json);
        if (is_mux_json) {
            nlohmann::json j;
            j["Ensemble"]["Name"] = rx->getEnsembleName();

            nlohmann::json j_services;
            for (const auto& s : rx->getServiceList()) {
                string urlmp3 = "/mp3/" + sid_to_hex(s.serviceId);
                nlohmann::json j_srv = {
                    {"SId", sid_to_hex(s.serviceId)},
                    {"Label", s.serviceLabel.label},
                    {"url_mp3", urlmp3}};

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

        const regex regex_mp3(R"(^GET [/]mp3[/]([^ ]+) HTTP)");
        std::smatch match_mp3;
        bool is_mp3 = regex_search(request, match_mp3, regex_mp3);
        if (is_mp3) {
            const string stream = match_mp3[1];
            for (const auto& srv : rx->getServiceList()) {
                if (sid_to_hex(srv.serviceId) == stream) {
                    try {
                        auto& ph = phs.at(srv.serviceId);

                        if (ph.rate != 48000 and ph.rate != 32000) {
                            throw out_of_range("Invalid rate "+to_string(ph.rate));
                        }

                        string headers = http_ok;
                        headers += http_contenttype_mp3;
                        headers += http_nocache;
                        headers += "\r\n";
                        s.send(headers.data(), headers.size(), MSG_NOSIGNAL);

                        ProgrammeSender sender(move(s));

                        cerr << "Registering mp3 sender" << endl;
                        ph.registerSender(&sender);
                        sender.wait_for_termination();

                        cerr << "Removing mp3 sender" << endl;
                        ph.removeSender(&sender);

                        return true;
                    }
                    catch (const out_of_range& e) {
                        cerr << "Could not setup mp3 sender for " <<
                            srv.serviceId << ": " << e.what() << endl;

                        string headers = http_503;
                        headers += http_contenttype_text;
                        headers += http_nocache;
                        headers += "\r\n";
                        headers += "503 Service Unavailable\r\n";
                        headers += e.what();
                        headers += "\r\n";
                        s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
                        return false;
                    }
                }
            }
        }

        cerr << "Could not understand request" << endl;
    }

    string headers = http_404;
    headers += http_contenttype_text;
    headers += http_nocache;
    headers += "\r\n";
    headers += "404 Not Found\r\n";
    headers += "Sorry\r\n";
    s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
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

    cerr << "Wait for sync" << endl;
    while (not synced) {
        this_thread::sleep_for(chrono::seconds(3));
    }

    // Wait for the number of services to converge
    ssize_t num_services = -1;
    while (true) {
        this_thread::sleep_for(chrono::seconds(3));
        auto list = rx->getServiceList();
        if (num_services == (ssize_t)list.size()) {
            cerr << "Found " << num_services << " services" << endl;
            break;
        }
        num_services = list.size();
    }

    for (auto& s : rx->getServiceList()) {
        WebProgrammeHandler ph(s.serviceId);
        phs.emplace(std::make_pair(s.serviceId, move(ph)));

        if (rx->addServiceToDecode(phs.at(s.serviceId), "", s) == false) {
            cerr << "Tune to 0x" << sid_to_hex(s.serviceId) << " failed" << endl;
        }
    }
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

void WebRadioInterface::onSyncChange(char isSync)
{
    synced = isSync;
}

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
