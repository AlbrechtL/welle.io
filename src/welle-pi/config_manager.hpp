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
#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "libs/json.hpp"

/* User settings, read-only: $XDG_CONFIG_HOME (or ~/.config)/welle-pi/welle-pi.json,
 * else /etc/welle-pi/welle-pi.json. Empty object if none. Unlike
 * stations.json, which is a cache welle-pi rewrites. */
nlohmann::json loadSettings();

class ConfigManager {
public:
    struct Station {
        std::string channel;
        std::string program;
        std::string short_program;
        uint32_t service_id;
    };

    ConfigManager(const std::string& filename = "stations.json");

    bool loadConfig();
    void saveConfig();

    const std::vector<Station>& getStations() const;
    void addStation(const Station& s);
    void clearStations();

    std::string getLastPlayedChannel() const;
    uint32_t getLastPlayedServiceId() const;

    void setLastPlayed(const std::string& channel, uint32_t service_id);

private:
    std::string m_filename;
    std::string m_lastPlayedChannel;
    uint32_t m_lastPlayedServiceId = 0;
    std::vector<Station> m_stations;
};

#endif // CONFIG_MANAGER_HPP
