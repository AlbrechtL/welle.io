/******************************************************************************\
 * Description:
 * ABI of welle-pi input plugins (C, stable)
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
#ifndef WELLE_PI_INPUT_PLUGIN_H
#define WELLE_PI_INPUT_PLUGIN_H

/* A plugin is a shared object listed in welle-pi.json:
 *   "input": [{"plugin": "foo", ...own keys...}]
 * welle-pi loads welle-pi-foo.so, see plugin_loader.hpp. It exports the
 * symbols of plugin.h, welle_pi_input_abi and welle_pi_input_run. */

#include "plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WELLE_PI_INPUT_ABI 1

/* Same values as InputAction. Never renumber, only append. */
enum welle_pi_action {
    WELLE_PI_UP, WELLE_PI_DOWN, WELLE_PI_LEFT, WELLE_PI_RIGHT,
    WELLE_PI_ENTER, WELLE_PI_MENU, WELLE_PI_QUIT,
    WELLE_PI_VOLUME_UP, WELLE_PI_VOLUME_DOWN, WELLE_PI_MUTE, WELLE_PI_HOME,
    /* not an action: one more than the last, so that push() does not have to
     * be touched when one is appended */
    WELLE_PI_ACTION_COUNT
};

struct welle_pi_input_host {
    void* ctx;
    void (*push)(void* ctx, enum welle_pi_action action);
    int (*stop_requested)(void* ctx);
};

/* Plugin defines: const int welle_pi_input_abi = WELLE_PI_INPUT_ABI; */
extern const int welle_pi_input_abi;

/* Runs in its own thread; must return soon after stop_requested() != 0,
 * welle-pi joins it on exit. config: the plugin's JSON object. Nonzero
 * return is logged. */
int welle_pi_input_run(const struct welle_pi_input_host* host, const char* config);

#ifdef __cplusplus
}
#endif

#endif /* WELLE_PI_INPUT_PLUGIN_H */
