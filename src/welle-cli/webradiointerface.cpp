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
static const char* http_500 = "HTTP/1.0 500 Internal Server Error\r\n";
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

static string to_hex(uint32_t value)
{
    std::stringstream sidstream;
    sidstream << "0x" <<
        std::setfill('0') << std::setw(4) <<
        std::hex << value;
    return sidstream.str();
}

ProgrammeSender::ProgrammeSender(Socket&& s) :
    s(move(s))
{
}

void ProgrammeSender::cancel()
{
    s.close();
}

bool ProgrammeSender::send_mp3(const std::vector<uint8_t>& mp3Data)
{
    if (not s.valid()) {
        return false;
    }

    const int flags = MSG_NOSIGNAL;

    ssize_t ret = s.send(mp3Data.data(), mp3Data.size(), flags);
    if (ret == -1) {
        s.close();
        std::unique_lock<std::mutex> lock(mutex);
        running = false;
        lock.unlock();
        cv.notify_all();
        return false;
    }

    return true;
}

void ProgrammeSender::wait_for_termination()
{
    std::unique_lock<std::mutex> lock(mutex);
    while (running) {
        cv.wait_for(lock, chrono::seconds(2));
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

void WebProgrammeHandler::cancelAll()
{
    std::unique_lock<std::mutex> lock(senders_mutex);
    for (auto& s : senders) {
        s->cancel();
    }
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
                int sampleRate, bool isStereo, const string& m)
{
    stereo = isStereo;
    rate = sampleRate;
    mode = m;

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
    if (decode_strategy == DecodeStrategy::All) {
        return;
    }

    lock_guard<mutex> lock(rx_mut);
    for (auto& s : rx->getServiceList()) {
        const auto sid = s.serviceId;

        try {
            bool require = phs.at(sid).needsToBeDecoded() or
                sid == current_carousel_service;
            bool is_decoded = programmes_being_decoded[sid];

            if (require and not is_decoded) {
                bool success = rx->addServiceToDecode(phs.at(sid), "", s);

                if (success) {
                    programmes_being_decoded[sid] = success;
                }
                else {
                    cerr << "Tune to 0x" << to_hex(s.serviceId) <<
                        " failed" << endl;
                }
            }
            else if (is_decoded and not require) {
                bool success = rx->removeServiceToDecode(s);

                if (success) {
                    programmes_being_decoded[sid] = false;
                }
                else {
                    cerr << "Stop playing 0x" << to_hex(s.serviceId) <<
                        " failed" << endl;
                }
            }

        }
        catch (const out_of_range&) {
            cerr << "Cannot tune to 0x" << to_hex(s.serviceId) <<
                " because no handler exists!" << endl;
        }
    }
    phs_changed.notify_all();
}

void WebRadioInterface::retune(const std::string& channel)
{
    auto freq = channels.getFrequency(channel);
    if (freq == 0) {
        cerr << "Invalid channel: " << channel << endl;
        return;
    }

    cerr << "Retune to " << freq << endl;
    {
        unique_lock<mutex> lock(rx_mut);

        cerr << "Destroy RX" << endl;
        rx.reset();

        for (auto& ph : phs) {
            cerr << "Cancel ph " << ph.first << endl;
            ph.second.cancelAll();
        }

        for (auto& ph : phs) {
            cerr << "Wait on ph " << ph.first << endl;
            while (ph.second.needsToBeDecoded() and
                    programmes_being_decoded[ph.first]) {
                phs_changed.wait_for(lock, chrono::seconds(1));
            }
        }

        this_thread::sleep_for(chrono::seconds(2));

        cerr << "Clearing" << endl;

        phs.clear();
        programmes_being_decoded.clear();
        tiis.clear();
        current_carousel_service = 0;

        cerr << "Set frequency" << endl;
        input.setFrequency(freq);
    }

    {
        cerr << "Restart RX" << endl;
        lock_guard<mutex> lock(rx_mut);
        rx = make_unique<RadioReceiver>(*this, input);

        if (not rx) {
            throw runtime_error("Could not initialise WebRadioInterface");
        }

        rx->restart(false);
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
        while (true) {
            unique_lock<mutex> lock(data_mut);
            if (rx) {
                break;
            }
            lock.unlock();
            this_thread::sleep_for(chrono::seconds(1));
        }

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
        else if (request.find("GET /spectrum HTTP") == 0) {
            return send_spectrum(s);
        }
        else if (request.find("GET /constellation HTTP") == 0) {
            return send_constellation(s);
        }
        else if (request.find("GET /nullspectrum HTTP") == 0) {
            return send_null_spectrum(s);
        }
        else if (request.find("GET /channel HTTP") == 0) {
            return send_channel(s);
        }
        else if (request.find("POST /channel HTTP") == 0) {
            return handle_channel_post(s, request);
        }
        else {
            const regex regex_mp3(R"(^GET [/]mp3[/]([^ ]+) HTTP)");
            std::smatch match_mp3;
            if (regex_search(request, match_mp3, regex_mp3)) {
                return send_mp3(s, match_mp3[1]);
            }
            else {
                cerr << "Could not understand request " << request << endl;
            }
        }

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
    {
        lock_guard<mutex> lock(rx_mut);
        const auto ensembleLabel = rx->getEnsembleLabel();
        j["ensemble"]["label"] = ensembleLabel.utf8_label();
        j["ensemble"]["shortlabel"] = ensembleLabel.utf8_shortlabel();
        j["ensemble"]["id"] = to_hex(rx->getEnsembleId());

        nlohmann::json j_services;
        for (const auto& s : rx->getServiceList()) {
            nlohmann::json j_srv = {
                {"sid", to_hex(s.serviceId)},
                {"pty", s.programType},
                {"ptystring", DABConstants::getProgramTypeName(s.programType)},
                {"label", s.serviceLabel.utf8_label()},
                {"shortlabel", s.serviceLabel.utf8_shortlabel()},
                {"language", s.language},
                {"languagestring", DABConstants::getLanguageName(s.language)}};

            nlohmann::json j_components;

            bool hasAudioComponent = false;
            for (const auto& sc : rx->getComponents(s)) {
                nlohmann::json j_sc = {
                    {"componentnr", sc.componentNr},
                    {"subchannelid", nullptr},
                    {"primary", (sc.PS_flag ? true : false)},
                    {"caflag", (sc.CAflag ? true : false)},
                    {"scid", nullptr},
                    {"ascty", nullptr},
                    {"dscty", nullptr}};


                const auto& sub = rx->getSubchannel(sc);

                switch (sc.transportMode()) {
                    case TransportMode::Audio:
                        j_sc["transportmode"] = "audio";
                        j_sc["subchannelid"] = sub.subChId;
                        j_sc["ascty"] =
                                (sc.audioType() == AudioServiceComponentType::DAB ? "DAB" :
                                 sc.audioType() == AudioServiceComponentType::DABPlus ? "DAB+" :
                                 "unknown");
                        hasAudioComponent |= (
                                sc.audioType() == AudioServiceComponentType::DAB or
                                sc.audioType() == AudioServiceComponentType::DABPlus);
                        break;
                    case TransportMode::FIDC:
                        j_sc["transportmode"] = "fidc";
                        j_sc["dscty"] = sc.DSCTy;
                        break;
                    case TransportMode::PacketData:
                        j_sc["transportmode"] = "packetdata";
                        j_sc["scid"] = sc.SCId;
                        break;
                    case TransportMode::StreamData:
                        j_sc["subchannelid"] = sub.subChId;
                        j_sc["transportmode"] = "streamdata";
                        j_sc["dscty"] = sc.DSCTy;
                        break;
                }

                j_sc["subchannel"] = {
                    { "bitrate", sub.bitrate()},
                    { "sad", sub.startAddr},
                    { "protection", sub.protection()}};

                j_components.push_back(j_sc);
            }

            if (hasAudioComponent) {
                string urlmp3 = "/mp3/" + to_hex(s.serviceId);
                j_srv["url_mp3"] = urlmp3;
            }
            else {
                j_srv["url_mp3"] = nullptr;
            }

            j_srv["components"] = j_components;

            try {
                const auto& wph = phs.at(s.serviceId);
                nlohmann::json j_audio = {
                    {"left", wph.last_audioLevel_L},
                    {"right", wph.last_audioLevel_R}};
                j_srv["audiolevel"] = j_audio;

                j_srv["channels"] = wph.stereo ? 2 : 1;
                j_srv["samplerate"] = wph.rate;
                j_srv["mode"] = wph.mode;

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
                j_srv["channels"] = 0;
                j_srv["samplerate"] = 0;
                j_srv["mot"] = nullptr;
                j_srv["dls"] = nullptr;
            }

            j_services.push_back(j_srv);
        }
        j["services"] = j_services;
    }

    {
        lock_guard<mutex> lock(data_mut);

        nlohmann::json j_utc = {
            {"year", last_dateTime.year},
            {"month", last_dateTime.month},
            {"day", last_dateTime.day},
            {"hour", last_dateTime.hour},
            {"minutes", last_dateTime.minutes}
        };

        j["utctime"] = j_utc;

        j["snr"] = last_snr;
        j["frequencycorrection"] =
            last_fine_correction + last_coarse_correction;

        for (const auto& tii : getTiiStats()) {
            j["tii"].push_back({
                    {"comb", tii.comb},
                    {"pattern", tii.pattern},
                    {"delay", tii.delay_samples},
                    {"delay_km", tii.getDelayKm()},
                    {"error", tii.error}});
        }
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
        if (to_hex(srv.serviceId) == stream or
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
        unique_lock<mutex> lock(fib_mut);
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

    lock_guard<mutex> lock(plotdata_mut);
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

static bool send_fft_data(Socket& s, DSPCOMPLEX *spectrumBuffer, size_t T_u)
{
    vector<float> spectrum(T_u);

    // Shift FFT samples
    const size_t half_Tu = T_u / 2;
    for (size_t i = 0; i < half_Tu; i++) {
        spectrum[i] = abs(spectrumBuffer[i + half_Tu]);
    }
    for (size_t i = half_Tu; i < T_u; i++) {
        spectrum[i] = abs(spectrumBuffer[i - half_Tu]);
    }

    string headers = http_ok;
    headers += http_contenttype_data;
    headers += http_nocache;
    headers += "\r\n";
    ssize_t ret = s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
    if (ret == -1) {
        cerr << "Failed to send spectrum headers" << endl;
        return false;
    }

    size_t lengthBytes = spectrum.size() * sizeof(float);
    ret = s.send(spectrum.data(), lengthBytes, MSG_NOSIGNAL);
    if (ret == -1) {
        cerr << "Failed to send spectrum data" << endl;
        return false;
    }

    return true;
}

bool WebRadioInterface::send_spectrum(Socket& s)
{
    // Get FFT buffer
    DSPCOMPLEX* spectrumBuffer = spectrum_fft_handler.getVector();
    auto samples = input.getSpectrumSamples(dabparams.T_u);

    // Continue only if we got data
    if (samples.size() != dabparams.T_u)
        return false;

    std::copy(samples.begin(), samples.end(), spectrumBuffer);

    // Do FFT to get the spectrum
    spectrum_fft_handler.do_FFT();

    return send_fft_data(s, spectrumBuffer, dabparams.T_u);
}

bool WebRadioInterface::send_null_spectrum(Socket& s)
{
    // Get FFT buffer
    DSPCOMPLEX* spectrumBuffer = spectrum_fft_handler.getVector();

    lock_guard<mutex> lock(plotdata_mut);
    if (last_NULL.size() != (size_t)dabparams.T_null) {
        cerr << "Invalid NULL size " << last_NULL.size() << endl;
        return false;
    }

    copy(last_NULL.begin(), last_NULL.begin() + dabparams.T_u, spectrumBuffer);

    // Do FFT to get the spectrum
    spectrum_fft_handler.do_FFT();

    return send_fft_data(s, spectrumBuffer, dabparams.T_u);
}

bool WebRadioInterface::send_constellation(Socket& s)
{
    const size_t decim = OfdmDecoder::constellationDecimation;
    const size_t num_iqpoints = (dabparams.L-1) * dabparams.K / decim;
    std::vector<float> phases(num_iqpoints);

    lock_guard<mutex> lock(plotdata_mut);
    if (last_constellation.size() == num_iqpoints) {
        phases.resize(num_iqpoints);
        for (size_t i = 0; i < num_iqpoints; i++) {
            const float y = 180.0f / (float)M_PI * std::arg(last_constellation[i]);
            phases[i] = y;
        }

        string headers = http_ok;
        headers += http_contenttype_data;
        headers += http_nocache;
        headers += "\r\n";
        ssize_t ret = s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
        if (ret == -1) {
            cerr << "Failed to send constellation headers" << endl;
            return false;
        }

        size_t lengthBytes = phases.size() * sizeof(float);
        ret = s.send(phases.data(), lengthBytes, MSG_NOSIGNAL);
        if (ret == -1) {
            cerr << "Failed to send constellation data" << endl;
            return false;
        }

        return true;
    }

    return false;
}

bool WebRadioInterface::send_channel(Socket& s)
{
    const auto freq = input.getFrequency();

    try {
        const auto chan = channels.getChannelForFrequency(freq);

        string response = http_ok;
        response += http_contenttype_text;
        response += http_nocache;
        response += "\r\n";
        response += chan;
        ssize_t ret = s.send(response.data(), response.size(), MSG_NOSIGNAL);
        if (ret == -1) {
            cerr << "Failed to send frequency" << endl;
            return false;
        }
    }
    catch (const out_of_range& e) {
        string response = http_500;
        response += http_contenttype_text;
        response += http_nocache;
        response += "\r\n";
        response += "Error: ";
        response += e.what();
        ssize_t ret = s.send(response.data(), response.size(), MSG_NOSIGNAL);
        if (ret == -1) {
            cerr << "Failed to send frequency 500" << endl;
            return false;
        }
    }
    return true;
}

bool WebRadioInterface::handle_channel_post(Socket& s, const std::string& request)
{
    auto header_end_ix = request.find("\r\n\r\n");
    if (header_end_ix != string::npos) {
        auto channel = request.substr(header_end_ix + 4);

        cerr << "POST channel: " << channel << endl;

        retune(channel);

        string response = http_ok;
        response += http_contenttype_text;
        response += http_nocache;
        response += "\r\n";
        response += "Retuning...";
        ssize_t ret = s.send(response.data(), response.size(), MSG_NOSIGNAL);
        if (ret == -1) {
            cerr << "Failed to send frequency" << endl;
            return false;
        }
        return true;
    }
    return false;
}

WebRadioInterface::WebRadioInterface(CVirtualInput& in,
        int port, DecodeStrategy ds) :
    dabparams(1),
    input(in),
    spectrum_fft_handler(dabparams.T_u),
    decode_strategy(ds)
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

    programme_handler_thread = thread(&WebRadioInterface::handle_phs, this);
}

WebRadioInterface::~WebRadioInterface()
{
    running = false;
    if (programme_handler_thread.joinable()) {
        programme_handler_thread.join();
    }
}

void WebRadioInterface::handle_phs()
{
    while (running) {
        this_thread::sleep_for(chrono::seconds(2));
        {
            lock_guard<mutex> lock_data(data_mut);
            if (not rx) {
                continue;
            }
        }

        unique_lock<mutex> lock(rx_mut);
        auto serviceList = rx->getServiceList();
        for (auto& s : serviceList) {
            if (phs.count(s.serviceId) == 0) {
                WebProgrammeHandler ph(s.serviceId);
                phs.emplace(std::make_pair(s.serviceId, move(ph)));
            }
            else if (decode_strategy == DecodeStrategy::All) {
                auto scs = rx->getComponents(s);
                for (auto& sc : scs) {
                    if (sc.transportMode() == TransportMode::Audio) {
                        bool success = rx->addServiceToDecode(
                                phs.at(s.serviceId), "", s);

                        if (not success) {
                            cerr << "Tune to 0x" << to_hex(s.serviceId) <<
                                " failed" << endl;
                        }
                    }
                }
            }
        }

        if (decode_strategy == DecodeStrategy::Carousel) {
            if (current_carousel_service == 0) {
                if (not serviceList.empty()) {
                    current_carousel_service = serviceList.front().serviceId;
                    time_carousel_change = chrono::steady_clock::now();
                }
            }
            else if (time_carousel_change + chrono::seconds(40) <
                    chrono::steady_clock::now()) {
                auto current_it = phs.find(current_carousel_service);
                if (current_it == phs.end()) {
                    cerr << "Reset service decoder carousel! Cannot find service "
                        << current_carousel_service << endl;
                    current_carousel_service = 0;
                }
                else {
                    // Rotate through phs
                    if (++current_it == phs.end()) {
                        current_it = phs.begin();
                    }
                    current_carousel_service = current_it->first;
                }
                time_carousel_change = chrono::steady_clock::now();
            }

            lock.unlock();
            check_decoders_required();
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
{
    (void)sId; (void)label;
}

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

    lock_guard<mutex> lock(fib_mut);

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

    new_fib_block_available.notify_one();
}

void WebRadioInterface::onNewImpulseResponse(std::vector<float>&& data)
{
    lock_guard<mutex> lock(plotdata_mut);
    last_CIR = move(data);
}

void WebRadioInterface::onNewNullSymbol(std::vector<DSPCOMPLEX>&& data)
{
    lock_guard<mutex> lock(plotdata_mut);
    last_NULL = move(data);
}

void WebRadioInterface::onConstellationPoints(std::vector<DSPCOMPLEX>&& data)
{
    lock_guard<mutex> lock(plotdata_mut);
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

    for (const auto& cp_list : tiis) {
        const auto comb = cp_list.first.first;
        const auto pattern = cp_list.first.second;

        if (cp_list.second.size() < 5) {
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
