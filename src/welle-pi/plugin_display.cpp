/******************************************************************************\
 * Description:
 * Display implemented by a plugin (display_plugin.h)
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
#include "plugin_display.hpp"
#include "display_plugin.h"
#include "plugin_loader.hpp"

#include <dlfcn.h>
#include <iostream>

std::unique_ptr<PluginDisplay> PluginDisplay::open(const std::string& name, const std::string& config) {
    void* lib = loadPlugin("display", name, WELLE_PI_DISPLAY_ABI);
    if (!lib) return nullptr;
    using OpenFn = void* (*)(const char*, int*, int*, unsigned*);
    auto openFn = reinterpret_cast<OpenFn>(dlsym(lib, "welle_pi_display_open"));
    auto present = reinterpret_cast<PresentFn>(dlsym(lib, "welle_pi_display_present"));
    auto close = reinterpret_cast<CloseFn>(dlsym(lib, "welle_pi_display_close"));
    if (!openFn || !present || !close) {
        std::cerr << "Display plugin " << name << ": welle_pi_display_open/present/close missing" << std::endl;
        return nullptr;
    }
    int cols = 0, rows = 0;
    unsigned features = 0;
    void* handle = openFn(config.c_str(), &cols, &rows, &features);
    if (!handle) return nullptr;
    if (cols <= 0 || rows <= 0) {
        std::cerr << "Display plugin " << name << ": size " << cols << " x " << rows << std::endl;
        close(handle);
        return nullptr;
    }
    return std::unique_ptr<PluginDisplay>(new PluginDisplay(
        handle, present, close, cols, rows, (features & WELLE_PI_DISPLAY_HIGHLIGHT) != 0));
}

PluginDisplay::~PluginDisplay() {
    m_close(m_handle);
}

void PluginDisplay::present(const Frame& frame) {
    // the same text on another row is a new frame too: only the row is inverted
    if (frame.lines == m_shown && frame.highlight == m_highlight) return;
    m_shown = frame.lines;
    m_highlight = frame.highlight;
    std::vector<const char*> lines;
    for (const auto& line : frame.lines) lines.push_back(line.c_str());
    m_present(m_handle, lines.data(), frame.cols, int(lines.size()), frame.highlight);
}
