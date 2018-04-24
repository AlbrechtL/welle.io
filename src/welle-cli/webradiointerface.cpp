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
#include "libs/base64.h"

using namespace std;

static const char* http_ok = "HTTP/1.0 200 OK\r\n";
static const char* http_404 = "HTTP/1.0 404 Not Found\r\n";
static const char* http_503 = "HTTP/1.0 503 Service Unavailable\r\n";
static const char* http_contenttype_mp3 = "Content-Type: audio/mpeg\r\n";
static const char* http_contenttype_text = "Content-Type: text/plain\r\n";
static const char* http_contenttype_data =
        "Content-Type: application/octet-stream\r\n";

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

bool WebProgrammeHandler::needsToBeDecoded() const
{
    std::unique_lock<std::mutex> lock(senders_mutex);
    return not senders.empty();
}


WebProgrammeHandler::dls_t WebProgrammeHandler::getDLS() const
{
    dls_t dls;

    std::unique_lock<std::mutex> lock(pad_mutex);
    if (last_label_valid) {
        dls.label = last_label;
        dls.time = std::chrono::system_clock::to_time_t(time_label);
    }

    return dls;
}

WebProgrammeHandler::mot_t WebProgrammeHandler::getMOT_base64() const
{
    mot_t mot;

    std::unique_lock<std::mutex> lock(pad_mutex);
    if (last_mot_valid) {
        mot.data = Base64::Encode(last_mot);
        mot.time = chrono::system_clock::to_time_t(time_mot);
        mot.subtype = last_subtype;
    }
    return mot;
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

    if (audioData.empty()) {
        return;
    }

    const int channels = stereo ? 2 : 1;

    if (stereo) {
        int16_t max_L = 0;
        int16_t max_R = 0;
        for (size_t i = 0; i < audioData.size()-1; i+=2) {
            max_L = std::max(max_L, audioData[i]);
            max_R = std::max(max_R, audioData[i+1]);
        }

        last_audioLevel_L = max_L;
        last_audioLevel_R = max_R;
    }
    else {
        int16_t max_mono = 0;
        for (size_t i = 0; i < audioData.size(); i++) {
            max_mono = std::max(max_mono, audioData[i]);
        }

        last_audioLevel_L = max_mono;
        last_audioLevel_R = max_mono;
    }

    if (not lame_initialised) {
        lame_set_in_samplerate(lame.lame, rate);
        lame_set_num_channels(lame.lame, channels);
        lame_set_VBR(lame.lame, vbr_default);
        lame_set_VBR_q(lame.lame, 2);
        lame_init_params(lame.lame);
        lame_initialised = true;
    }

    vector<uint8_t> mp3buf(16384);

    int written = lame_encode_buffer_interleaved(lame.lame,
            audioData.data(), audioData.size()/channels,
            mp3buf.data(), mp3buf.size());

    if (written < 0) {
        cerr << "Failed to encode mp3: " << written << endl;
    }
    else if (written > (ssize_t)mp3buf.size()) {
        cerr << "mp3 encoder wrote more than buffer size!" << endl;
    }
    else if (written > 0) {
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
    if (subtype == 0x01) {
        last_subtype = MOTType::JPEG;
    }
    else if (subtype == 0x03) {
        last_subtype = MOTType::PNG;
    }
    else {
        last_subtype = MOTType::Unknown;
    }
}

void WebRadioInterface::check_decoders_required()
{
    if (decode_all) {
        return;
    }

    std::unique_lock<std::mutex> lock(phs_decode_mut);
    for (auto& s : rx->getServiceList()) {
        const auto sid = s.serviceId;

        try {
            bool require = phs.at(sid).needsToBeDecoded();
            bool is_decoded = programmes_being_decoded[sid];

            if (require and not is_decoded) {
                bool success = rx->addServiceToDecode(phs.at(sid), "", s);

                if (success) {
                    programmes_being_decoded[sid] = success;
                }
                else {
                    cerr << "Tune to 0x" << sid_to_hex(s.serviceId) <<
                        " failed" << endl;
                }
            }
            else if (is_decoded and not require) {
                bool success = rx->removeServiceToDecode(s);

                if (success) {
                    programmes_being_decoded[sid] = false;
                }
                else {
                    cerr << "Stop playing 0x" << sid_to_hex(s.serviceId) <<
                        " failed" << endl;
                }
            }

        }
        catch (const out_of_range&) {
            cerr << "Cannot tune to 0x" << sid_to_hex(s.serviceId) <<
                " because no handler exists!" << endl;
        }
    }
}

bool WebRadioInterface::dispatch_client(Socket&& client)
{
    Socket s(move(client));

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

        if (request.find("GET / HTTP") == 0) {
            return send_index(s);
        }
        else if (request.find("GET /mux.json HTTP") == 0) {
            return send_mux_json(s);
        }
        else if (request.find("GET /fic HTTP") == 0) {
            return send_fic(s);
        }
        else if (request.find("GET /impulseresponse HTTP") == 0) {
            return send_impulseresponse(s);
        }
        else {
            const regex regex_mp3(R"(^GET [/]mp3[/]([^ ]+) HTTP)");
            std::smatch match_mp3;
            if (regex_search(request, match_mp3, regex_mp3)) {
                return send_mp3(s, match_mp3[1]);
            }
        }

        cerr << "Could not understand request " << request << endl;
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

bool WebRadioInterface::send_index(Socket& s)
{
    FILE *fd = fopen("index.html", "r");
    if (fd) {
        string headers = http_ok;
        headers += http_contenttype_html;
        headers += http_nocache;
        headers += "\r\n";
        ssize_t ret = s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
        if (ret == -1) {
            cerr << "Failed to send index.html headers" << endl;
            return false;
        }

        vector<char> data(1024);
        do {
            ret = fread(data.data(), 1, data.size(), fd);
            ret = s.send(data.data(), ret, MSG_NOSIGNAL);
            if (ret == -1) {
                cerr << "Failed to send index.html data" << endl;
                fclose(fd);
                return false;
            }

        } while (ret > 0);

        fclose(fd);
        return true;
    }
    return false;
}

bool WebRadioInterface::send_mux_json(Socket& s)
{
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

        try {
            const auto& wph = phs.at(s.serviceId);
            nlohmann::json j_audio = {
                {"left", wph.last_audioLevel_L},
                {"right", wph.last_audioLevel_R}};
            j_srv["audiolevel"] = j_audio;

            j_srv["channels"] = wph.stereo ? 2 : 1;
            j_srv["samplerate"] = wph.rate;

            auto mot = wph.getMOT_base64();
            nlohmann::json j_mot = {
                {"data", mot.data},
                {"time", mot.time}};
            switch (mot.subtype) {
                case MOTType::Unknown:
                    j_mot["type"] = "application/octet-stream";
                    break;
                case MOTType::JPEG:
                    j_mot["type"] = "image/jpeg";
                    break;
                case MOTType::PNG:
                    j_mot["type"] = "image/png";
                    break;
            }
            j_srv["mot"] = j_mot;

            auto dls = wph.getDLS();
            nlohmann::json j_dls = {
                {"label", dls.label},
                {"time", dls.time}};
            j_srv["dls"] = j_dls;
        }
        catch (const out_of_range&) {
            j_srv["audiolevel"] = nullptr;
        }

        j_services.push_back(j_srv);
    }
    j["Services"] = j_services;

    {
        lock_guard<mutex> lock(data_mut);

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
    ssize_t ret = s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
    if (ret == -1) {
        cerr << "Failed to send mux.json headers" << endl;
        return false;
    }

    const auto json_str = j.dump();

    ret = s.send(json_str.c_str(), json_str.size(), MSG_NOSIGNAL);
    if (ret == -1) {
        cerr << "Failed to send mux.json data" << endl;
        return false;
    }
    return true;
}

bool WebRadioInterface::send_mp3(Socket& s, const std::string& stream)
{
    for (const auto& srv : rx->getServiceList()) {
        if (sid_to_hex(srv.serviceId) == stream or
                (uint32_t)std::stoi(stream) == srv.serviceId) {
            try {
                auto& ph = phs.at(srv.serviceId);

                string headers = http_ok;
                headers += http_contenttype_mp3;
                headers += http_nocache;
                headers += "\r\n";
                ssize_t ret = s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
                if (ret == -1) {
                    cerr << "Failed to send mp3 headers" << endl;
                    return false;
                }

                ProgrammeSender sender(move(s));

                cerr << "Registering mp3 sender" << endl;
                ph.registerSender(&sender);
                check_decoders_required();
                sender.wait_for_termination();

                cerr << "Removing mp3 sender" << endl;
                ph.removeSender(&sender);
                check_decoders_required();

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
    return false;
}

bool WebRadioInterface::send_fic(Socket& s)
{
    string headers = http_ok;
    headers += http_contenttype_data;
    headers += http_nocache;
    headers += "\r\n";
    ssize_t ret = s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
    if (ret == -1) {
        cerr << "Failed to send FIC headers" << endl;
        return false;
    }

    while (true) {
        unique_lock<mutex> lock(data_mut);
        while (fib_blocks.empty()) {
            new_fib_block_available.wait_for(lock, chrono::seconds(1));
        }
        ret = s.send(fib_blocks.front().data(),
                fib_blocks.front().size(), MSG_NOSIGNAL);
        if (ret == -1) {
            cerr << "Failed to send FIC data" << endl;
            return false;
        }

        fib_blocks.pop_front();
    }
    return true;
}

bool WebRadioInterface::send_impulseresponse(Socket& s)
{
    string headers = http_ok;
    headers += http_contenttype_data;
    headers += http_nocache;
    headers += "\r\n";
    ssize_t ret = s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
    if (ret == -1) {
        cerr << "Failed to send CIR headers" << endl;
        return false;
    }

    unique_lock<mutex> lock(data_mut);
    vector<float> cir_db(last_CIR.size());
    std::transform(last_CIR.begin(), last_CIR.end(), cir_db.begin(),
            [](float y) { return 10.0f * log10(y); });

    size_t lengthBytes = cir_db.size() * sizeof(float);
    ret = s.send(cir_db.data(), lengthBytes, MSG_NOSIGNAL);
    if (ret == -1) {
        cerr << "Failed to send CIR data" << endl;
        return false;
    }

    return true;
}

WebRadioInterface::WebRadioInterface(CVirtualInput& in, int port, bool decode_all) :
    decode_all(decode_all)
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

        if (decode_all) {
            bool success = rx->addServiceToDecode(phs.at(s.serviceId), "", s);
            if (not success) {
                cerr << "Tune to 0x" << sid_to_hex(s.serviceId) <<
                    " failed" << endl;
            }
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
                switch (fut.wait_for(chrono::milliseconds(1))) {
                    case future_status::deferred:
                    case future_status::timeout:
                        still_running_connections.push_back(move(fut));
                        break;
                    case future_status::ready:
                        fut.get();
                        break;
                }
            }
        }
        running_connections = move(still_running_connections);
    }
}

void WebRadioInterface::onSNR(int snr)
{
    lock_guard<mutex> lock(data_mut);
    last_snr = snr;
}

void WebRadioInterface::onFrequencyCorrectorChange(int fine, int coarse)
{
    lock_guard<mutex> lock(data_mut);
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
    lock_guard<mutex> lock(data_mut);
    last_dateTime = dateTime;
}

void WebRadioInterface::onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib)
{
    if (not crcCheckOk) {
        return;
    }

    lock_guard<mutex> lock(data_mut);

    // Convert the fib bitvector to bytes
    vector<uint8_t> buf(32);
    for (size_t i = 0; i < buf.size(); i++) {
        uint8_t v = 0;
        for (int j = 0; j < 8; j++) {
            if (fib[8*i+j]) {
                v |= 1 << (7-j);
            }
        }
        buf[i] = v;
    }
    fib_blocks.push_back(move(buf));

    if (fib_blocks.size() > 3*250) { // six seconds
        fib_blocks.pop_front();
    }
}

void WebRadioInterface::onNewImpulseResponse(std::vector<float>&& data)
{
    lock_guard<mutex> lock(data_mut);
    last_CIR = move(data);
}

void WebRadioInterface::onNewNullSymbol(std::vector<DSPCOMPLEX>&& data)
{
    lock_guard<mutex> lock(data_mut);
    last_NULL = move(data);
}

void WebRadioInterface::onConstellationPoints(std::vector<DSPCOMPLEX>&& data)
{
    lock_guard<mutex> lock(data_mut);
    last_constellation = move(data);
}

void WebRadioInterface::onMessage(message_level_t level, const std::string& text)
{
    lock_guard<mutex> lock(data_mut);
    pending_messages.emplace_back(level, text);
}

void WebRadioInterface::onTIIMeasurement(tii_measurement_t&& m)
{
    lock_guard<mutex> lock(data_mut);
    auto& l = tiis[make_pair(m.comb, m.pattern)];
    l.push_back(move(m));

    if (l.size() > 20) {
        l.pop_front();
    }
}

list<tii_measurement_t> WebRadioInterface::getTiiStats()
{
    list<tii_measurement_t> l;

    lock_guard<mutex> lock(data_mut);
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
