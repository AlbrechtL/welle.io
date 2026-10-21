/******************************************************************************\
 * Description:
 * welle-pi display plugin: character LCD through liblcd (kernel charlcd, /dev/lcd)
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
#include "display_plugin.h"

#include <liblcd/liblcd.h>

#include <iostream>
#include <string>
#include <vector>

const char welle_pi_plugin_name[] = "lcd";
const char welle_pi_plugin_description[] = "character LCD on /dev/lcd (kernel charlcd) via liblcd";

const int welle_pi_display_abi = WELLE_PI_DISPLAY_ABI;

namespace {
struct Lcd {
    liblcd::LCDDisplay display;
    std::vector<std::string> shown;   // per row; only changed rows are written
};
}

// {"plugin": "lcd"}; size from the device tree
void* welle_pi_display_open(const char* /*config*/, int* cols, int* rows)
{
    auto lcd = new Lcd;
    if (lcd->display.getWidth() == 0 || lcd->display.getHeight() == 0) {
        std::cerr << "lcd: no character LCD (/dev/lcd)" << std::endl;
        delete lcd;
        return nullptr;
    }
    lcd->display.clear();
    lcd->display.backlightOn();
    *cols = lcd->display.getWidth();
    *rows = lcd->display.getHeight();
    return lcd;
}

void welle_pi_display_present(void* handle, const char* const* lines, int /*cols*/, int rows)
{
    auto lcd = static_cast<Lcd*>(handle);
    lcd->shown.resize(rows);
    for (int row = 0; row < rows; ++row) {
        if (lcd->shown[row] == lines[row]) continue;
        lcd->display.gotoXY(0, row);
        // write() is printf-like: DLS text must not be the format
        lcd->display.write("%s", lines[row]);
        lcd->shown[row] = lines[row];
    }
}

void welle_pi_display_close(void* handle)
{
    auto lcd = static_cast<Lcd*>(handle);
    lcd->display.clear();
    lcd->display.backlightOff();
    delete lcd;
}
