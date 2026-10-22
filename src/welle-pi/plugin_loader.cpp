/******************************************************************************\
 * Description:
 * Finds and loads welle-pi plugins (shared objects)
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
#include "plugin_loader.hpp"

#include <climits>
#include <dlfcn.h>
#include <iostream>
#include <unistd.h>
#include <vector>

#ifndef WELLE_PI_PLUGIN_DIR
#define WELLE_PI_PLUGIN_DIR "/usr/local/lib/welle-pi"
#endif

static std::string exeDir() {
    char buf[PATH_MAX];
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return ".";
    const std::string path(buf, n);
    return path.substr(0, path.rfind('/'));
}

void* loadPlugin(const std::string& kind, const std::string& name, int abi) {
    if (name.empty()) {
        std::cerr << "welle-pi.json: " << kind << " plugin without name" << std::endl;
        return nullptr;
    }
    std::vector<std::string> paths;
    if (name.find('/') != std::string::npos) {
        paths.push_back(name);
    } else {
        const std::string file = "welle-pi-" + name + ".so";
        paths.push_back(exeDir() + "/welle-pi-plugins/" + file);
        paths.push_back(std::string(WELLE_PI_PLUGIN_DIR) + "/" + file);
    }
    for (const auto& path : paths) {
        if (access(path.c_str(), F_OK) != 0) continue;
        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            std::cerr << "Plugin " << path << ": " << dlerror() << std::endl;
            return nullptr;
        }
        auto version = static_cast<const int*>(dlsym(handle, ("welle_pi_" + kind + "_abi").c_str()));
        if (!version || *version != abi) {
            std::cerr << "Plugin " << path << ": ABI "
                      << (version ? std::to_string(*version) : "missing") << ", need " << abi << std::endl;
            dlclose(handle);
            return nullptr;
        }
        auto pluginName = static_cast<const char*>(dlsym(handle, "welle_pi_plugin_name"));
        auto description = static_cast<const char*>(dlsym(handle, "welle_pi_plugin_description"));
        if (!pluginName || !description) {
            std::cerr << "Plugin " << path << ": welle_pi_plugin_name/description missing" << std::endl;
            dlclose(handle);
            return nullptr;
        }
        std::cerr << kind << " plugin " << pluginName << " (" << description << "): " << path << std::endl;
        return handle;
    }
    std::cerr << "Plugin " << name << " not found:";
    for (const auto& path : paths) std::cerr << " " << path;
    std::cerr << std::endl;
    return nullptr;
}
