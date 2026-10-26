/******************************************************************************\
 * Description:
 * Tests for input source selection and plugin loading
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

#include <atomic>
#include <cassert>
#include <iostream>

static std::atomic<bool> stop(false);

int main()
{
    InputQueue queue;
    const StopPredicate stop_requested = [] { return stop.load(); };

    // found by name in welle-pi-plugins/ next to this executable; gets its own
    // JSON object; unknown entries and plugins are skipped
    nlohmann::json inputs = nlohmann::json::array({
        {{"plugin", "example"}, {"keys", "de"}},
        {{"plugin", "nonexistent"}},
        {{"plugin", "/nonexistent/plugin.so"}},
        {{"type", "bogus"}},
        {{"type", "stdin"}},  // skipped: not interactive
    });
    auto threads = startInputSources(inputs, queue, stop_requested, false);
    assert(threads.size() == 1);
    assert(queue.pop(2000) == InputAction::DOWN);
    assert(queue.pop(2000) == InputAction::ENTER);
    stop = true;
    for (auto& t : threads) t.join();

    assert(startInputSources(nlohmann::json::object(), queue, stop_requested, false).empty());

    // a daemon without "-p" can only start when something can pick a
    // programme: a plugin, or the terminal when welle-pi has one
    nlohmann::json stdin_source = nlohmann::json::array();
    stdin_source.push_back({{"type", "stdin"}});
    nlohmann::json plugin_source = nlohmann::json::array();
    plugin_source.push_back({{"plugin", "evdev"}});

    assert(!hasInputSources(nlohmann::json(), false));      // default, no terminal
    assert(hasInputSources(nlohmann::json(), true));
    assert(!hasInputSources(nlohmann::json::array(), false));
    assert(hasInputSources(stdin_source, true));
    assert(!hasInputSources(stdin_source, false));
    assert(hasInputSources(plugin_source, false));
    assert(!hasInputSources(nlohmann::json::object(), false));

    std::cout << "input_plugin_tests: OK" << std::endl;
    return 0;
}
