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
#include <regex>
#include <iostream>
#include <utility>
#include <cstdio>
#include <errno.h>
#include <yaml-cpp/yaml.h>
#include "welle-cli/webradiointerface.h"

using namespace std;

static const char* http_ok = "HTTP/1.0 200 OK\r\n";
static const char* http_contentlength = "Content-Length: 0\r\n";
static const char* http_contenttype_wav = "Content-type: audio/wav\r\n";
static const char* http_contenttype_yaml = "Content-type: application/x-yaml\r\n";
static const char* http_nocache = "Cache-Control: no-cache\r\n";

void WebRadioInterface::dispatch_client(Socket s)
{
    vector<char> buf(1025);
    if (not s.valid()) {
        cerr << "socket in dispatcher not valid!" << endl;
        return;
    }
    ssize_t ret = s.recv(buf.data(), buf.size()-1, 0);

    if (ret == 0) {
        return;
    }
    else if (ret == -1) {
        string errstr = strerror(errno);
        cerr << "recv error " << errstr << endl;
        return;
    }
    else {
        buf.resize(ret);
        string request(buf.begin(), buf.end());

        const regex regex_head(R"(^HEAD [/]([^ ]+) HTTP)");
        const regex regex_get(R"(^GET [/]([^ ]+) HTTP)");

        std::smatch match_head;
        bool isHead = regex_search(request, match_head, regex_head);
        if (isHead) {
            const string resource = match_head[1];
            if (resource == "mux.yaml") {
                string headers = http_ok;
                headers += http_contenttype_yaml;
                headers += http_contentlength;
                headers += http_nocache;
                headers += "\r\n";
                s.send(headers.data(), headers.size(), 0);
            }
        }

        std::smatch match_get;
        bool isGet = regex_search(request, match_get, regex_get);
        if (isGet) {
            const string resource = match_get[1];
            if (resource == "mux.yaml") {
                YAML::Emitter e;
                e << YAML::BeginDoc << YAML::BeginMap <<
                    YAML::Key << "Ensemble" <<
                    YAML::Value <<
                        YAML::BeginMap <<
                            YAML::Key << "Name" <<
                            YAML::Value << rx->getEnsembleName() <<
                        YAML::EndMap <<
                    YAML::Key << "Services" <<
                    YAML::Value <<
                        YAML::BeginSeq;
                for (const auto& s : rx->getServiceList()) {
                    e << YAML::BeginMap;
                    e << YAML::Key << "SId" <<
                         YAML::Value << s.serviceId;
                    e << YAML::Key << "Label" <<
                         YAML::Value << s.serviceLabel.label;
                    e << YAML::Key << "Components" <<
                         YAML::Value << YAML::BeginSeq;
                    for (const auto& sc : rx->getComponents(s)) {
                        e << YAML::BeginMap;
                        e << YAML::Key << "ComponentNr" <<
                            YAML::Value << sc.componentNr;
                        e << YAML::Key << "ASCTy" <<
                            YAML::Value <<
                            (sc.audioType() == AudioServiceComponentType::DAB ? "DAB" :
                             sc.audioType() == AudioServiceComponentType::DABPlus ? "DAB+" :
                             "unknown");

                        const auto& sub = rx->getSubchannel(sc);
                        e << YAML::Key << "Subchannel_id" <<
                            YAML::Value << sub.subChId;
                        e << YAML::Key << "Bitrate" <<
                            YAML::Value << sub.bitrate();
                        e << YAML::Key << "SAd" <<
                            YAML::Value << sub.startAddr;
                        e << YAML::EndMap;
                    }
                    e << YAML::EndSeq;
                    e << YAML::EndMap;
                }
                e << YAML::EndSeq;

                {
                    lock_guard<mutex> lock(mut);

                    e << YAML::Key << "UTCTime" <<
                        YAML::Value <<
                            YAML::BeginMap <<
                                YAML::Key << "year" <<
                                YAML::Value << last_dateTime.year <<
                                YAML::Key << "month" <<
                                YAML::Value << last_dateTime.month <<
                                YAML::Key << "day" <<
                                YAML::Value << last_dateTime.day <<
                                YAML::Key << "hour" <<
                                YAML::Value << last_dateTime.hour <<
                                YAML::Key << "minutes" <<
                                YAML::Value << last_dateTime.minutes <<
                            YAML::EndMap;
                }
                e << YAML::EndMap << YAML::EndDoc;

                string headers = http_ok;
                headers += http_contenttype_yaml;
                headers += http_nocache;
                headers += "\r\n";
                s.send(headers.data(), headers.size(), 0);

                s.send(e.c_str(), e.size(), 0);
                e.c_str();
            }
        }

        if (not (isGet or isHead)) {
            cerr << "Could not understand request" << endl;
        }
    }
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
    deque<future<void> > running_connections;

    while (true) {
        auto client = serverSocket.accept();

        running_connections.push_back(async(launch::async,
                    &WebRadioInterface::dispatch_client, this, move(client)));

        deque<future<void> > still_running_connections;
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
    last_tii = move(m);
}

