/******************************************************************************\
 * Description:
 * Input sources: each turns one device into InputActions on an InputQueue
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
#ifndef INPUT_SOURCES_HPP
#define INPUT_SOURCES_HPP

#include "input_queue.hpp"
#include "libs/json.hpp"
#include <functional>
#include <thread>
#include <vector>

/* Thread bodies. Each returns once stop_requested() is true. */
using StopPredicate = std::function<bool()>;

/* Terminal keys: arrows, u/d/l/r, e/Enter, m/Esc, q/. quits. Raw mode for its
 * lifetime. */
void runStdinReader(InputQueue& queue, const StopPredicate& stop_requested);

/* Starts the "input" entries of welle-pi.json ({"type": "stdin"} or
 * {"plugin": "<name>", ...}); null means stdin. Bad entries are logged and
 * skipped: the radio keeps playing without them. Join the threads after
 * stop_requested() turned true. */
std::vector<std::thread> startInputSources(const nlohmann::json& inputs, InputQueue& queue,
                                           const StopPredicate& stop_requested, bool interactive);

#endif // INPUT_SOURCES_HPP
