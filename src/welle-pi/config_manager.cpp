/*
 *    Copyright (C) 2026
 *    Fridrich Strba (fridrich.strba@bluewin.ch)
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
#include "config_manager.hpp"
#include "libs/json.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void create_directories(const std::string& dir_path) {
    std::string current_path = "";
    for (size_t i = 0; i < dir_path.size(); ++i) {
        current_path += dir_path[i];
        if (dir_path[i] == '/') {
            if (!current_path.empty()) {
                mkdir(current_path.c_str(), 0755);
            }
        }
    }
    if (!current_path.empty() && current_path.back() != '/') {
        mkdir(current_path.c_str(), 0755);
    }
}

nlohmann::json loadSettings() {
    std::vector<std::string> paths;
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    const char* home_env = std::getenv("HOME");
    if (xdg_config != nullptr && xdg_config[0] != '\0') {
        paths.push_back(std::string(xdg_config) + "/welle-pi/welle-pi.json");
    } else if (home_env != nullptr && home_env[0] != '\0') {
        paths.push_back(std::string(home_env) + "/.config/welle-pi/welle-pi.json");
    }
    paths.push_back("/etc/welle-pi/welle-pi.json");

    for (const auto& path : paths) {
        std::ifstream f(path);
        if (!f.is_open()) continue;
        try {
            nlohmann::json j = nlohmann::json::parse(f);
            std::cerr << "Settings: " << path << std::endl;
            return j.is_object() ? j : nlohmann::json::object();
        } catch (const std::exception& e) {
            std::cerr << "Error parsing " << path << ": " << e.what() << std::endl;
            return nlohmann::json::object();
        }
    }
    return nlohmann::json::object();
}

ConfigManager::ConfigManager(const std::string& filename) {
    if (filename != "stations.json") {
        m_filename = filename;
    } else {
        std::string cache_dir = "";
        const char* xdg_cache = std::getenv("XDG_CACHE_HOME");
        if (xdg_cache != nullptr && xdg_cache[0] != '\0') {
            cache_dir = xdg_cache;
        } else {
            const char* home_env = std::getenv("HOME");
            if (home_env != nullptr && home_env[0] != '\0') {
                cache_dir = std::string(home_env) + "/.cache";
            } else {
                cache_dir = "/var/cache";
            }
        }
        m_filename = cache_dir + "/welle-pi/stations.json";
    }
}

bool ConfigManager::loadConfig() {
    std::ifstream f(m_filename);
    if (!f.is_open()) {
        // Fallback check: if the main resolved path doesn't exist, try local stations.json
        std::ifstream local_f("stations.json");
        if (local_f.is_open()) {
            m_filename = "stations.json";
            return loadConfig();
        }
        return false;
    }
    try {
        nlohmann::json j;
        f >> j;
        m_stations.clear();
        auto lp_it = j.find("last_played");
        if (lp_it != j.end()) {
            auto lp = *lp_it;
            auto ch_it = lp.find("channel");
            if (ch_it != lp.end()) {
                m_lastPlayedChannel = ch_it->get<std::string>();
            }
            auto id_it = lp.find("service_id");
            if (id_it != lp.end()) {
                m_lastPlayedServiceId = id_it->get<uint32_t>();
            }
        }
        auto st_it = j.find("stations");
        if (st_it != j.end()) {
            for (const auto& item : *st_it) {
                Station s;
                auto ch_it = item.find("channel");
                if (ch_it != item.end()) s.channel = ch_it->get<std::string>();
                auto pr_it = item.find("program");
                if (pr_it != item.end()) s.program = pr_it->get<std::string>();
                auto spr_it = item.find("short_program");
                if (spr_it != item.end()) s.short_program = spr_it->get<std::string>();
                auto id_it = item.find("service_id");
                if (id_it != item.end()) s.service_id = id_it->get<uint32_t>();
                m_stations.push_back(s);
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config file: " << e.what() << std::endl;
        return false;
    }
}

void ConfigManager::saveConfig() {
    if (m_filename != "stations.json") {
        size_t last_slash = m_filename.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string parent_dir = m_filename.substr(0, last_slash);
            create_directories(parent_dir);
        }
    }

    try {
        nlohmann::json j;
        j["last_played"] = {
            {"channel", m_lastPlayedChannel},
            {"service_id", m_lastPlayedServiceId}
        };
        nlohmann::json stations_json = nlohmann::json::array();
        for (const auto& s : m_stations) {
            stations_json.push_back({
                {"channel", s.channel},
                {"program", s.program},
                {"short_program", s.short_program},
                {"service_id", s.service_id}
            });
        }
        j["stations"] = stations_json;

        std::ofstream f(m_filename);
        if (f.is_open()) {
            f << j.dump(2);
        } else {
            std::cerr << "Failed to open " << m_filename << " for writing. Falling back to local stations.json" << std::endl;
            m_filename = "stations.json";
            std::ofstream local_f(m_filename);
            if (local_f.is_open()) {
                local_f << j.dump(2);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving config file: " << e.what() << std::endl;
    }
}

const std::vector<ConfigManager::Station>& ConfigManager::getStations() const { return m_stations; }

void ConfigManager::addStation(const Station& s) {
    for (const auto& existing : m_stations) {
        if (existing.channel == s.channel && existing.program == s.program && existing.service_id == s.service_id) {
            return;
        }
    }
    m_stations.push_back(s);
}

void ConfigManager::clearStations() {
    m_stations.clear();
}

std::string ConfigManager::getLastPlayedChannel() const { return m_lastPlayedChannel; }
uint32_t ConfigManager::getLastPlayedServiceId() const { return m_lastPlayedServiceId; }

void ConfigManager::setLastPlayed(const std::string& channel, uint32_t service_id) {
    m_lastPlayedChannel = channel;
    m_lastPlayedServiceId = service_id;
}
