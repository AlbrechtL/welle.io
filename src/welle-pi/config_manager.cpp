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
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* True when path is a directory owned by us that no other user can write to.
 * Checked after lstat, so a symlink in the way is refused as well. */
static bool is_own_directory(const std::string& path) {
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode) && st.st_uid == geteuid() &&
           (st.st_mode & (S_IWGRP | S_IWOTH)) == 0;
}

/* Create path with 0700, then verify it. A path that exists as a symlink or
 * as somebody else's directory is refused instead of used. */
static bool make_own_directory(const std::string& path) {
    struct stat st;
    if (lstat(path.c_str(), &st) != 0 && mkdir(path.c_str(), 0700) != 0) {
        return false;
    }
    return is_own_directory(path);
}

/* The cache directory itself, or $HOME/.cache below it when a fresh account
 * has no ~/.cache yet. Never walks further up: a missing HOME must not make
 * welle-pi create directories on the way. */
static bool ensure_cache_directory(const std::string& dir) {
    if (make_own_directory(dir)) {
        return true;
    }
    const auto slash = dir.find_last_of('/');
    if (slash == std::string::npos || slash == 0) {
        return false;
    }
    return make_own_directory(dir.substr(0, slash)) && make_own_directory(dir);
}

static bool home_is_usable(const std::string& home) {
    struct stat st;
    return stat(home.c_str(), &st) == 0 && S_ISDIR(st.st_mode) &&
           access(home.c_str(), W_OK | X_OK) == 0;
}

/* O_NOFOLLOW: never write the cache through a symlink planted in a shared
 * directory. shortcut: mkdir followed by lstat is racy against an attacker
 * with write access to the parent, use openat(O_DIRECTORY) chains if the
 * cache ever has to live in such a directory. */

/* Write next to the target and rename it over it, so a crash or a full disk
 * leaves either the old file or the new one, never a truncated one. */
static bool write_file(const std::string& path, const std::string& data) {
    const std::string tmp = path + ".tmp";
    const int fd = ::open(tmp.c_str(),
                          O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW,
                          0600);
    if (fd < 0) {
        return false;
    }
    int err = 0;
    size_t done = 0;
    while (done < data.size()) {
        const ssize_t n = ::write(fd, data.data() + done, data.size() - done);
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n <= 0) {
            err = n < 0 ? errno : EIO;
            break;
        }
        done += size_t(n);
    }
    if (::close(fd) != 0 && err == 0) {
        err = errno;
    }
    if (err == 0 && ::rename(tmp.c_str(), path.c_str()) != 0) {
        err = errno;
    }
    if (err != 0) {
        ::unlink(tmp.c_str());
        errno = err;
        return false;
    }
    return true;
}

nlohmann::json loadSettings(std::string* path) {
    std::vector<std::string> paths;
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    const char* home_env = std::getenv("HOME");
    if (xdg_config != nullptr && xdg_config[0] != '\0') {
        paths.push_back(std::string(xdg_config) + "/welle-pi/welle-pi.json");
    } else if (home_env != nullptr && home_env[0] != '\0') {
        paths.push_back(std::string(home_env) + "/.config/welle-pi/welle-pi.json");
    }
    paths.push_back("/etc/welle-pi/welle-pi.json");

    for (const auto& candidate : paths) {
        std::ifstream f(candidate);
        if (!f.is_open()) continue;
        try {
            nlohmann::json j = nlohmann::json::parse(f);
            if (path != nullptr) {
                *path = candidate;
            }
            return j.is_object() ? j : nlohmann::json::object();
        } catch (const std::exception& e) {
            std::cerr << "Error parsing " << candidate << ": " << e.what() << std::endl;
            return nlohmann::json::object();
        }
    }
    return nlohmann::json::object();
}

ConfigManager::ConfigManager(const std::string& filename) {
    if (filename != "stations.json") {
        /* Explicit file, no cache directory to pick or create. */
        m_filename = filename;
        return;
    }

    const char* systemd_cache = std::getenv("CACHE_DIRECTORY");
    const char* xdg_cache = std::getenv("XDG_CACHE_HOME");
    const char* home_env = std::getenv("HOME");

    if (systemd_cache != nullptr && systemd_cache[0] != '\0') {
        /* CacheDirectory= of the systemd unit: already the finished directory,
         * created for the service user by systemd. */
        m_cache_dir = systemd_cache;
    }
    else if (xdg_cache != nullptr && xdg_cache[0] != '\0') {
        m_cache_dir = std::string(xdg_cache) + "/welle-pi";
    }
    else if (home_env != nullptr && home_env[0] != '\0' && home_is_usable(home_env)) {
        m_cache_dir = std::string(home_env) + "/.cache/welle-pi";
    }
    else {
        /* No usable home: the system service and system accounts land here,
         * so the cache does not end up in the working directory. */
        m_cache_dir = "/var/cache/welle-pi";
    }

    m_filename = m_cache_dir + "/stations.json";
}

const std::string& ConfigManager::filename() const {
    return m_filename;
}

bool ConfigManager::loadConfig() {
    std::ifstream f(m_filename);
    if (!f.is_open()) {
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

        if (!m_cache_dir.empty() && !ensure_cache_directory(m_cache_dir)) {
            std::cerr << "Cannot use the cache directory " << m_cache_dir << std::endl;
            return;
        }
        if (!write_file(m_filename, j.dump(2))) {
            std::cerr << "Cannot write " << m_filename << ": " << std::strerror(errno) << std::endl;
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
