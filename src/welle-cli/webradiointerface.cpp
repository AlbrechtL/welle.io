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
#include <array>
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
static const char* http_500 = "HTTP/1.0 500 Internal Server Error\r\n";
static const char* http_503 = "HTTP/1.0 503 Service Unavailable\r\n";
static const char* http_contenttype_mp3 = "Content-Type: audio/mpeg\r\n";
static const char* http_contenttype_text = "Content-Type: text/plain\r\n";
static const char* http_contenttype_data =
        "Content-Type: application/octet-stream\r\n";

static const char* http_contenttype_json =
        "Content-Type: application/json; charset=utf-8\r\n";

static const char* http_contenttype_js =
        "Content-Type: text/javascript; charset=utf-8\r\n";

static const char* http_contenttype_html =
        "Content-Type: Content-Type: text/html; charset=utf-8\r\n";

static const char* http_nocache = "Cache-Control: no-cache\r\n";

template <int W>
static string to_hex(uint32_t value)
{
    std::stringstream sidstream;
    sidstream << "0x" <<
        std::setfill('0') << std::setw(W) <<
        std::hex << value;
    return sidstream.str();
}

WebRadioInterface::WebRadioInterface(CVirtualInput& in,
        int port,
        DecodeSettings ds,
        RadioReceiverOptions rro) :
    dabparams(1),
    input(in),
    spectrum_fft_handler(dabparams.T_u),
    rro(rro),
    decode_settings(ds)
{
    bool success = serverSocket.bind(port);
    if (success) {
        success = serverSocket.listen();
    }

    if (success) {
        rx = make_unique<RadioReceiver>(*this, in, rro);
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

class TuneFailed {};

void WebRadioInterface::check_decoders_required()
{
    lock_guard<mutex> lock(rx_mut);
    try {
        for (auto& s : rx->getServiceList()) {
            const auto sid = s.serviceId;

            try {
                const bool is_active = std::find_if(
                        carousel_services_active.cbegin(),
                        carousel_services_active.cend(),
                        [&](const ActiveCarouselService& acs){
                        return acs.sid == sid;
                        }) != carousel_services_active.cend();

                const bool require =
                    rx->serviceHasAudioComponent(s) and
                    (decode_settings.strategy == DecodeStrategy::All or
                     phs.at(sid).needsToBeDecoded() or
                     is_active);
                const bool is_decoded = programmes_being_decoded[sid];

                if (require and not is_decoded) {
                    bool success = rx->addServiceToDecode(phs.at(sid), "", s);

                    if (success) {
                        programmes_being_decoded[sid] = success;
                    }
                    else {
                        throw TuneFailed();
                    }
                }
                else if (is_decoded and not require) {
                    bool success = rx->removeServiceToDecode(s);

                    if (success) {
                        programmes_being_decoded[sid] = false;
                    }
                    else {
                        cerr << "Stop playing 0x" << to_hex<4>(s.serviceId) <<
                            " failed" << endl;
                        throw TuneFailed();
                    }
                }
            }
            catch (const out_of_range&) {
                cerr << "Cannot tune to 0x" << to_hex<4>(s.serviceId) <<
                    " because no handler exists!" << endl;
            }
        }
    }
    catch (const TuneFailed&) {
        phs.clear();
        programmes_being_decoded.clear();
        rx->restart_decoder();
        carousel_services_available.clear();
        carousel_services_active.clear();
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

    cerr << "Kill programme handler" << freq << endl;
    running = false;
    if (programme_handler_thread.joinable()) {
        programme_handler_thread.join();
    }

    cerr << "Take ownership of RX" << endl;
    {
        unique_lock<mutex> lock(rx_mut);

        cerr << "Destroy RX" << endl;
        rx.reset();

        cerr << "Set frequency" << endl;
        input.setFrequency(freq);
        input.reset(); // Clear buffer

        cerr << "Restart RX" << endl;
        rx = make_unique<RadioReceiver>(*this, input, rro);

        if (not rx) {
            throw runtime_error("Could not initialise WebRadioInterface");
        }

        rx->restart(false);

        cerr << "Start programme handler" << endl;
        running = true;
        programme_handler_thread = thread(&WebRadioInterface::handle_phs, this);
    }
}

bool WebRadioInterface::dispatch_client(Socket&& client)
{
    Socket s(move(client));

    bool success = false;

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
            unique_lock<mutex> lock(rx_mut);
            if (rx) {
                break;
            }
            lock.unlock();
            this_thread::sleep_for(chrono::seconds(1));
        }

        buf.resize(ret);
        string request(buf.begin(), buf.end());

        if (request.find("GET / HTTP") == 0) {
            success = send_file(s, "index.html", http_contenttype_html);
        }
        else if (request.find("GET /index.js HTTP") == 0) {
            success = send_file(s, "index.js", http_contenttype_js);
        }
        else if (request.find("GET /mux.json HTTP") == 0) {
            success = send_mux_json(s);
        }
        else if (request.find("GET /fic HTTP") == 0) {
            success = send_fic(s);
        }
        else if (request.find("GET /impulseresponse HTTP") == 0) {
            success = send_impulseresponse(s);
        }
        else if (request.find("GET /spectrum HTTP") == 0) {
            success = send_spectrum(s);
        }
        else if (request.find("GET /constellation HTTP") == 0) {
            success = send_constellation(s);
        }
        else if (request.find("GET /nullspectrum HTTP") == 0) {
            success = send_null_spectrum(s);
        }
        else if (request.find("GET /channel HTTP") == 0) {
            success = send_channel(s);
        }
        else if (request.find("POST /channel HTTP") == 0) {
            success = handle_channel_post(s, request);
        }
        else {
            const regex regex_mp3(R"(^GET [/]mp3[/]([^ ]+) HTTP)");
            std::smatch match_mp3;
            if (regex_search(request, match_mp3, regex_mp3)) {
                success = send_mp3(s, match_mp3[1]);
            }
            else {
                cerr << "Could not understand request " << request << endl;
            }
        }

        if (not success) {
            string headers = http_404;
            headers += http_contenttype_text;
            headers += http_nocache;
            headers += "\r\n";
            headers += "404 Not Found\r\n";
            headers += "Could not understand request.\r\n";
            s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
        }

        return success;
    }
}

bool WebRadioInterface::send_file(Socket& s,
        const std::string& filename,
        const std::string& content_type)
{
    FILE *fd = fopen(filename.c_str(), "r");
    if (fd) {
        string headers = http_ok;
        headers += content_type;
        headers += http_nocache;
        headers += "\r\n";
        ssize_t ret = s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
        if (ret == -1) {
            cerr << "Failed to send file headers" << endl;
            return false;
        }

        vector<char> data(1024);
        do {
            ret = fread(data.data(), 1, data.size(), fd);
            ret = s.send(data.data(), ret, MSG_NOSIGNAL);
            if (ret == -1) {
                cerr << "Failed to send file data" << endl;
                fclose(fd);
                return false;
            }

        } while (ret > 0);

        fclose(fd);
        return true;
    }
    else {
        string headers = http_500;
        headers += http_contenttype_text;
        headers += http_nocache;
        headers += "\r\n";
        headers += "file '" + filename + "' is missing!";
        ssize_t ret = s.send(headers.data(), headers.size(), MSG_NOSIGNAL);
        if (ret == -1) {
            cerr << "Failed to send file headers" << endl;
            return false;
        }
        return true;
    }
    return false;
}

struct peak_t {
    int index = -1;
    float value = -1e30f;
};

static void to_json(nlohmann::json& j, const peak_t& peak)
{
    j = nlohmann::json{
        {"index", peak.index},
            {"value", 10.0f * log10(peak.value)}};
}

static nlohmann::json calculate_cir_peaks(const vector<float>& cir_linear)
{
    constexpr size_t num_peaks = 6;

    list<peak_t> peaks;

    if (not cir_linear.empty()) {
        vector<float> cir_lin(cir_linear);

        // Every time we find a peak, we attenuate it, including
        // its surrounding values, and we go search the next peak.
        for (size_t peak = 0; peak < num_peaks; peak++) {
            peak_t p;
            for (size_t i = 1; i < cir_lin.size(); i++) {
                if (cir_lin[i] > p.value) {
                    p.value = cir_lin[i];
                    p.index = i;
                }
            }

            const size_t windowsize = 25;
            for (size_t j = 0; j < windowsize; j++) {
                const ssize_t i = p.index + j - windowsize/2;
                if (i >= 0 and i < (ssize_t)cir_lin.size()) {
                    cir_lin[i] *= 0;
                }
            }

            peaks.push_back(move(p));
        }
    }

    nlohmann::json j = peaks;
    return j;
}

bool WebRadioInterface::send_mux_json(Socket& s)
{
    nlohmann::json j;

    j["receiver"]["software"]["name"] = "welle.io";
    j["receiver"]["software"]["version"] = GITDESCRIBE;
    j["receiver"]["hardware"]["name"] = input.getName();
    j["receiver"]["hardware"]["gain"] = input.getGain();

    {
        lock_guard<mutex> lock(fib_mut);
        j["ensemble"]["fic"]["numcrcerrors"] = num_fic_crc_errors;
    }

    {
        lock_guard<mutex> lock(rx_mut);
        if (!rx) {
            return false;
        }
        const auto ensembleLabel = rx->getEnsembleLabel();
        j["ensemble"]["label"] = ensembleLabel.utf8_label();
        j["ensemble"]["shortlabel"] = ensembleLabel.utf8_shortlabel();
        j["ensemble"]["id"] = to_hex<4>(rx->getEnsembleId());
        j["ensemble"]["ecc"] = to_hex<2>(rx->getEnsembleEcc());

        nlohmann::json j_services;
        for (const auto& s : rx->getServiceList()) {
            nlohmann::json j_srv = {
                {"sid", to_hex<4>(s.serviceId)},
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
                    {"primary", (sc.PS_flag ? true : false)},
                    {"caflag", (sc.CAflag ? true : false)},
                    {"scid", nullptr},
                    {"ascty", nullptr},
                    {"dscty", nullptr}};


                const auto& sub = rx->getSubchannel(sc);

                switch (sc.transportMode()) {
                    case TransportMode::Audio:
                        j_sc["transportmode"] = "audio";
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
                        j_sc["transportmode"] = "streamdata";
                        j_sc["dscty"] = sc.DSCTy;
                        break;
                }

                j_sc["subchannel"] = {
                    {"subchid", sub.subChId},
                    {"bitrate", sub.bitrate()},
                    {"cu", sub.numCU()},
                    {"sad", sub.startAddr},
                    {"protection", sub.protection()},
                    {"language", sub.language},
                    {"languagestring", DABConstants::getLanguageName(sub.language)}};


                j_components.push_back(j_sc);
            }

            if (hasAudioComponent) {
                string urlmp3 = "/mp3/" + to_hex<4>(s.serviceId);
                j_srv["url_mp3"] = urlmp3;
            }
            else {
                j_srv["url_mp3"] = nullptr;
            }

            j_srv["components"] = j_components;

            try {
                const auto& wph = phs.at(s.serviceId);
                const auto al = wph.getAudioLevels();
                nlohmann::json j_audio = {
                    {"time", chrono::system_clock::to_time_t(al.time)},
                    {"left", al.last_audioLevel_L},
                    {"right", al.last_audioLevel_R}};
                j_srv["audiolevel"] = j_audio;

                j_srv["channels"] = wph.stereo ? 2 : 1;
                j_srv["samplerate"] = wph.rate;
                j_srv["mode"] = wph.mode;

                auto mot = wph.getMOT_base64();
                nlohmann::json j_mot = {
                    {"data", mot.data},
                    {"time", chrono::system_clock::to_time_t(mot.time)}};
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
                    {"time", chrono::system_clock::to_time_t(dls.time)}};
                j_srv["dls"] = j_dls;

                auto errorcounters = wph.getErrorCounters();
                nlohmann::json j_errorcounters = {
                    {"frameerrors", errorcounters.num_frameErrors},
                    {"rserrors", errorcounters.num_rsErrors},
                    {"aacerrors", errorcounters.num_aacErrors},
                    {"time", chrono::system_clock::to_time_t(dls.time)}};
                j_srv["errorcounters"] = j_errorcounters;

                auto xpad_err = wph.getXPADErrors();
                nlohmann::json j_xpad_err;
                j_xpad_err["haserror"] = xpad_err.has_error;
                if (xpad_err.has_error) {
                    j_xpad_err["announcedlen"] = xpad_err.announced_xpad_len;
                    j_xpad_err["len"] = xpad_err.xpad_len;
                    j_xpad_err["time"] = chrono::system_clock::to_time_t(xpad_err.time);
                }
                j_srv["xpaderror"] = j_xpad_err;
            }
            catch (const out_of_range&) {
                j_srv["audiolevel"] = nullptr;
                j_srv["channels"] = 0;
                j_srv["samplerate"] = 0;
                j_srv["mode"] = "invalid";
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
            {"minutes", last_dateTime.minutes},
            {"lto", last_dateTime.hourOffset + ((double)last_dateTime.minuteOffset / 30.0)},
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

    {
        lock_guard<mutex> lock(plotdata_mut);
        j["cir"] = calculate_cir_peaks(last_CIR);
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
        if (to_hex<4>(srv.serviceId) == stream or
                (uint32_t)std::stoul(stream) == srv.serviceId) {
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
    if (samples.size() != (size_t)dabparams.T_u)
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

void WebRadioInterface::handle_phs()
{
    while (running) {
        this_thread::sleep_for(chrono::seconds(2));

        unique_lock<mutex> lock(rx_mut);
        if (not rx) {
            lock.unlock();
            continue;
        }

        auto serviceList = rx->getServiceList();
        for (auto& s : serviceList) {
            auto scs = rx->getComponents(s);

            if (std::find(
                        carousel_services_available.cbegin(),
                        carousel_services_available.cend(),
                        s.serviceId) == carousel_services_available.cend()) {
                for (auto& sc : scs) {
                    if (sc.transportMode() == TransportMode::Audio) {
                        carousel_services_available.push_back(s.serviceId);
                    }
                }
            }

            if (phs.count(s.serviceId) == 0) {
                WebProgrammeHandler ph(s.serviceId);
                phs.emplace(std::make_pair(s.serviceId, move(ph)));
            }
        }

        using namespace chrono;
        size_t max_services_in_carousel = std::min(
                carousel_services_available.size(),
                (size_t)decode_settings.num_decoders_in_carousel);

        if (decode_settings.strategy == DecodeStrategy::Carousel10) {
            while (carousel_services_active.size() < max_services_in_carousel) {
                carousel_services_active.emplace_back(
                        carousel_services_available.front());
                carousel_services_available.pop_front();
            }

            for (auto& acs : carousel_services_active) {
                if (acs.time_change + chrono::seconds(10) <
                        chrono::steady_clock::now()) {
                    acs.sid = 0;

                    if (not carousel_services_available.empty()) {
                        carousel_services_active.emplace_back(
                                carousel_services_available.front());
                        carousel_services_available.pop_front();
                    }
                }
            }
        }
        else if (decode_settings.strategy == DecodeStrategy::CarouselPAD) {
            while (carousel_services_active.size() < max_services_in_carousel) {
                if (not serviceList.empty()) {
                    carousel_services_active.emplace_back(
                            carousel_services_available.front());
                    carousel_services_available.pop_front();
                }
            }

            for (auto& acs : carousel_services_active) {
                if (acs.time_change + chrono::seconds(5) <
                        chrono::steady_clock::now()) {
                    auto current_it = phs.find(acs.sid);
                    if (current_it == phs.end()) {
                        cerr << "Reset service decoder carousel!"
                            "Cannot find service "
                            << acs.sid << endl;
                        acs.sid = 0;
                    }
                    else {
                        // Switch to next programme once both DLS and Slideshow
                        // got decoded, but at most after 80 seconds
                        const auto now = system_clock::now();
                        const auto mot = current_it->second.getMOT_base64();
                        const auto dls = current_it->second.getDLS();
                        // Slide and DLS received in the last 60 seconds?
                        const bool switchBecausePAD = (
                                now - mot.time < seconds(60) and
                                now - dls.time < seconds(60));

                        const bool switchBecauseLate =
                            acs.time_change + seconds(80) < steady_clock::now();

                        if (switchBecausePAD or switchBecauseLate) {
                            acs.sid = 0;

                            if (not carousel_services_available.empty()) {
                                carousel_services_active.emplace_back(
                                        carousel_services_available.front());
                                carousel_services_available.pop_front();
                            }
                        }
                    }
                }
            }
        }

        carousel_services_active.erase(
                remove_if(
                    carousel_services_active.begin(),
                    carousel_services_active.end(),
                    [](const ActiveCarouselService& acs){
                        return acs.sid == 0;
                    }), carousel_services_active.end());
        lock.unlock();
        check_decoders_required();
    }

    {
        unique_lock<mutex> lock(rx_mut);
        if (rx) {
            for (auto& s : rx->getServiceList()) {
                const auto sid = s.serviceId;
                const bool is_decoded = programmes_being_decoded[sid];
                if (is_decoded) {
                    (void)rx->removeServiceToDecode(s);
                }
            }
        }
    }

    phs.clear();
    programmes_being_decoded.clear();
    carousel_services_available.clear();
    carousel_services_active.clear();
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
        lock_guard<mutex> lock(fib_mut);
        num_fic_crc_errors++;
        return;
    }

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

    {
        lock_guard<mutex> lock(fib_mut);
        fib_blocks.push_back(move(buf));

        if (fib_blocks.size() > 3*250) { // six seconds
            fib_blocks.pop_front();
        }
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
