/******************************************************************************\
 * Description:
 * Tests for the station cache: which directory ConfigManager picks, that it
 * creates only directories it owns, and that it never writes through a
 * symlink
 *
 * This file was generated with the help of an AI assistant (Claude Code).
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
#include "config_manager.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {

std::string make_temp_dir()
{
    char tmpl[] = "/tmp/welle-pi-cache-XXXXXX";
    char* dir = mkdtemp(tmpl);
    assert(dir != nullptr);
    return dir;
}

std::string read_file(const std::string& path)
{
    std::ifstream f(path);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

void clear_cache_env()
{
    unsetenv("CACHE_DIRECTORY");
    unsetenv("XDG_CACHE_HOME");
    unsetenv("HOME");
}

}

int main()
{
    const std::string base = make_temp_dir();

    // CacheDirectory= of the systemd unit wins, and names the finished
    // directory rather than a parent.
    {
        clear_cache_env();
        const std::string dir = base + "/cache";
        setenv("CACHE_DIRECTORY", dir.c_str(), 1);
        ConfigManager config;
        assert(config.filename() == dir + "/stations.json");

        // saveConfig creates the missing directory with 0700
        config.addStation({"10B", "GRRIF", "GRRIF", 0x1234});
        config.saveConfig();
        struct stat st;
        assert(stat(dir.c_str(), &st) == 0);
        assert(S_ISDIR(st.st_mode));
        assert((st.st_mode & (S_IWGRP | S_IWOTH)) == 0);
        assert(read_file(dir + "/stations.json").find("GRRIF") != std::string::npos);
    }

    // Without CACHE_DIRECTORY the XDG cache directory is used
    {
        clear_cache_env();
        const std::string xdg = base + "/xdg";
        assert(mkdir(xdg.c_str(), 0700) == 0);
        setenv("XDG_CACHE_HOME", xdg.c_str(), 1);
        ConfigManager config;
        assert(config.filename() == xdg + "/welle-pi/stations.json");
        config.saveConfig();
        assert(read_file(xdg + "/welle-pi/stations.json").find("last_played") != std::string::npos);
    }

    // A home directory without ~/.cache: both levels are created, nothing
    // above the home directory
    {
        clear_cache_env();
        const std::string home = base + "/home";
        assert(mkdir(home.c_str(), 0700) == 0);
        setenv("HOME", home.c_str(), 1);
        ConfigManager config;
        assert(config.filename() == home + "/.cache/welle-pi/stations.json");
        config.saveConfig();
        assert(read_file(home + "/.cache/welle-pi/stations.json").find("last_played") != std::string::npos);
    }

    // No usable home: /var/cache instead of the working directory
    {
        clear_cache_env();
        setenv("HOME", (base + "/missing").c_str(), 1);
        ConfigManager config;
        assert(config.filename() == "/var/cache/welle-pi/stations.json");
    }

    // A symlink where the cache directory should be is refused, and the
    // target is left alone
    {
        clear_cache_env();
        const std::string victim = base + "/victim";
        assert(mkdir(victim.c_str(), 0700) == 0);
        const std::string dir = base + "/link";
        assert(symlink(victim.c_str(), dir.c_str()) == 0);
        setenv("CACHE_DIRECTORY", dir.c_str(), 1);
        ConfigManager config;
        config.saveConfig();
        assert(access((victim + "/stations.json").c_str(), F_OK) != 0);
    }

    // A symlink at the file itself is not followed either
    {
        clear_cache_env();
        const std::string dir = base + "/plain";
        assert(mkdir(dir.c_str(), 0700) == 0);
        const std::string victim = base + "/victim.txt";
        {
            std::ofstream f(victim);
            f << "untouched";
        }
        assert(symlink(victim.c_str(), (dir + "/stations.json").c_str()) == 0);
        setenv("CACHE_DIRECTORY", dir.c_str(), 1);
        ConfigManager config;
        config.saveConfig();
        assert(read_file(victim) == "untouched");
    }

    // The file is written beside and renamed: no temporary file stays behind,
    // and the new content replaces the old one completely
    {
        clear_cache_env();
        const std::string dir = base + "/atomic";
        setenv("CACHE_DIRECTORY", dir.c_str(), 1);
        ConfigManager config;
        config.addStation({"10B", "GRRIF", "GRRIF", 1});
        config.saveConfig();
        config.clearStations();
        config.saveConfig();
        assert(access((dir + "/stations.json.tmp").c_str(), F_OK) != 0);
        assert(read_file(dir + "/stations.json").find("GRRIF") == std::string::npos);
    }

    // An explicit file name is used as given, without creating anything
    {
        clear_cache_env();
        ConfigManager config(base + "/explicit.json");
        assert(config.filename() == base + "/explicit.json");
        config.saveConfig();
        assert(access((base + "/explicit.json").c_str(), F_OK) == 0);
    }

    std::cout << "config_tests: OK" << std::endl;
    return 0;
}
