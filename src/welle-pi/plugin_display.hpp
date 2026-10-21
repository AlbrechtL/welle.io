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
#ifndef PLUGIN_DISPLAY_HPP
#define PLUGIN_DISPLAY_HPP

#include "display_interface.hpp"

class PluginDisplay : public IDisplay {
public:
    // name: see loadPlugin(); config: the plugin's JSON object. nullptr if unusable.
    static std::unique_ptr<PluginDisplay> open(const std::string& name, const std::string& config);
    ~PluginDisplay() override;

    int cols() const override { return m_cols; }
    int rows() const override { return m_rows; }
    void present(const Frame& frame) override;

private:
    using PresentFn = void (*)(void*, const char* const*, int, int);
    using CloseFn = void (*)(void*);

    PluginDisplay(void* handle, PresentFn present, CloseFn close, int cols, int rows)
        : m_handle(handle), m_present(present), m_close(close), m_cols(cols), m_rows(rows) {}

    void* m_handle;
    PresentFn m_present;
    CloseFn m_close;
    int m_cols;
    int m_rows;
    std::vector<std::string> m_shown;
};

#endif // PLUGIN_DISPLAY_HPP
