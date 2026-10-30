/******************************************************************************\
 * Description:
 * Input sources: selection from welle-pi.json, plugin loading
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
#include "input_sources.hpp"
#include "input_plugin.h"
#include "plugin_loader.hpp"

#include <dlfcn.h>
#include <iostream>
#include <memory>

static_assert(int(InputAction::UP) == WELLE_PI_UP && int(InputAction::DOWN) == WELLE_PI_DOWN &&
              int(InputAction::LEFT) == WELLE_PI_LEFT && int(InputAction::RIGHT) == WELLE_PI_RIGHT &&
              int(InputAction::ENTER) == WELLE_PI_ENTER && int(InputAction::MENU) == WELLE_PI_MENU &&
              int(InputAction::QUIT) == WELLE_PI_QUIT &&
              int(InputAction::VOLUME_UP) == WELLE_PI_VOLUME_UP &&
              int(InputAction::VOLUME_DOWN) == WELLE_PI_VOLUME_DOWN &&
              int(InputAction::MUTE) == WELLE_PI_MUTE &&
              int(InputAction::HOME) == WELLE_PI_HOME &&
              int(InputAction::NONE) == WELLE_PI_ACTION_COUNT,
              "input_plugin.h out of sync with InputAction");

namespace {

using RunFn = int (*)(const welle_pi_input_host*, const char*);

struct PluginCtx {
    InputQueue& queue;
    StopPredicate stop_requested;
};

void pluginPush(void* ctx, welle_pi_action action) {
    // plugin is foreign code: drop out-of-range values
    if (action >= WELLE_PI_UP && action < WELLE_PI_ACTION_COUNT)
        static_cast<PluginCtx*>(ctx)->queue.push(InputAction(action));
}

int pluginStop(void* ctx) {
    return static_cast<PluginCtx*>(ctx)->stop_requested() ? 1 : 0;
}

RunFn loadInputPlugin(const std::string& name) {
    void* handle = loadPlugin("input", name, WELLE_PI_INPUT_ABI);
    if (!handle) return nullptr;
    auto run = reinterpret_cast<RunFn>(dlsym(handle, "welle_pi_input_run"));
    if (!run)
        std::cerr << "Input plugin " << name << ": welle_pi_input_run missing" << std::endl;
    return run;
}

} // namespace

std::vector<std::thread> startInputSources(const nlohmann::json& inputs, InputQueue& queue,
                                           const StopPredicate& stop_requested, bool interactive)
{
    nlohmann::json list = inputs;
    if (list.is_null())
        list = {{{"type", "stdin"}}};

    std::vector<std::thread> threads;
    if (!list.is_array()) {
        std::cerr << "welle-pi.json: \"input\" must be an array" << std::endl;
        return threads;
    }
    for (const auto& entry : list) {
        const std::string plugin = entry.is_object() ? entry.value("plugin", "") : "";
        const std::string type = entry.is_object() ? entry.value("type", "") : "";
        if (!plugin.empty()) {
            RunFn run = loadInputPlugin(plugin);
            if (!run) continue;
            threads.emplace_back([run, plugin, config = entry.dump(), &queue, stop_requested]() {
                PluginCtx ctx{queue, stop_requested};
                const welle_pi_input_host host{&ctx, pluginPush, pluginStop};
                if (int rc = run(&host, config.c_str()))
                    std::cerr << "Input plugin " << plugin << " returned " << rc << std::endl;
            });
        } else if (type == "stdin") {
            // no terminal under systemd; raw mode would fail anyway
            if (interactive)
                threads.emplace_back(runStdinReader, std::ref(queue), stop_requested);
        } else {
            std::cerr << "welle-pi.json: unknown input " << entry.dump() << std::endl;
        }
    }
    return threads;
}

bool hasInputSources(const nlohmann::json& inputs, bool interactive)
{
    if (inputs.is_null())
        return interactive;      // the default source is the terminal
    if (!inputs.is_array())
        return false;
    for (const auto& entry : inputs) {
        if (!entry.is_object())
            continue;
        if (!entry.value("plugin", "").empty())
            return true;
        if (entry.value("type", "") == "stdin" && interactive)
            return true;
    }
    return false;
}
