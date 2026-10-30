/******************************************************************************\
 * Description:
 * Tests of welle-pi display plugins
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

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

int main()
{
    const std::string file = "/tmp/welle-pi-display-plugin-" + std::to_string(getpid()) + ".txt";
    {
        // found by name in welle-pi-plugins/ next to this executable
        auto display = PluginDisplay::open("example", "{\"plugin\":\"example\",\"file\":\"" + file + "\"}");
        assert(display);
        assert(display->cols() == 16 && display->rows() == 2);
        assert(!display->canHighlight());   // the example draws plain text

        Frame frame(16, 2, 0);
        frame.set(0, "Radio Example   ");
        frame.set(1, "11D             ");
        display->present(frame);
        display->present(frame);  // unchanged: not passed on
        frame.set(1, "Now playing     ");
        display->present(frame);
        // a row marked without new text is a new frame too
        frame.highlight = 1;
        display->present(frame);
        frame.highlight = 0;
        display->present(frame);
    }
    std::ifstream in(file);
    std::stringstream text;
    text << in.rdbuf();
    unlink(file.c_str());
    assert(text.str() ==
           "frame 16 x 2\nRadio Example   \n11D             \n"
           "frame 16 x 2\nRadio Example   \nNow playing     \n"
           "frame 16 x 2, highlighted row 1\nRadio Example   \nNow playing     \n"
           "frame 16 x 2, highlighted row 0\nRadio Example   \nNow playing     \n"
           "closed\n");

    assert(!PluginDisplay::open("nonexistent", "{}"));
    assert(!PluginDisplay::open("/nonexistent/plugin.so", "{}"));
    assert(!PluginDisplay::open("example", "{}"));  // plugin refuses: no "file"
    assert(!PluginDisplay::open("", "{}"));

    std::cout << "display_plugin_tests: OK" << std::endl;
    return 0;
}
