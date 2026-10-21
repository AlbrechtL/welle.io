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

    // plugin receives its own JSON object; unknown entries and bad paths are skipped
    nlohmann::json inputs = nlohmann::json::array({
        {{"type", "plugin"}, {"path", INPUT_PLUGIN_EXAMPLE}, {"keys", "de"}},
        {{"type", "plugin"}, {"path", "/nonexistent/plugin.so"}},
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

    std::cout << "input_plugin_tests: OK" << std::endl;
    return 0;
}
