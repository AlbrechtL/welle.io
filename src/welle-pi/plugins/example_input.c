/******************************************************************************\
 * Description:
 * Example welle-pi input plugin, used by input_plugin_tests: pushes the keys listed in its config
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
#include "input_plugin.h"

#include <string.h>
#include <unistd.h>

const int welle_pi_input_abi = WELLE_PI_INPUT_ABI;

/* config e.g. {"plugin":"example","keys":"de"}: d = DOWN, e = ENTER */
int welle_pi_input_run(const struct welle_pi_input_host* host, const char* config)
{
    const char* keys = strstr(config, "\"keys\":\"");
    if (keys) {
        for (keys += 8; *keys && *keys != '"'; keys++) {
            if (*keys == 'd') host->push(host->ctx, WELLE_PI_DOWN);
            else if (*keys == 'e') host->push(host->ctx, WELLE_PI_ENTER);
        }
    }
    while (!host->stop_requested(host->ctx))
        usleep(10000);
    return 0;
}
