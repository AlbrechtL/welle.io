/******************************************************************************\
 * Description:
 * ABI of welle-pi display plugins (C, stable)
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
#ifndef WELLE_PI_DISPLAY_PLUGIN_H
#define WELLE_PI_DISPLAY_PLUGIN_H

/* A plugin is a shared object listed in welle-pi.json:
 *   "display": {"plugin": "foo", ...own keys...}
 * welle-pi loads welle-pi-foo.so, see plugin_loader.hpp. It exports the
 * symbols of plugin.h and the ones below. All calls come from the UI
 * thread. */

#include "plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WELLE_PI_DISPLAY_ABI 1

/* Plugin defines: const int welle_pi_display_abi = WELLE_PI_DISPLAY_ABI; */
extern const int welle_pi_display_abi;

/* config: the plugin's JSON object. Sets the size in characters (> 0) and
 * returns a handle passed to the other calls, NULL if the display is not
 * usable (logged by the plugin). */
void* welle_pi_display_open(const char* config, int* cols, int* rows);

/* rows lines of UTF-8, each exactly cols code points. cols and rows can be
 * smaller than the display's own size when mirrored next to a smaller
 * display. Only called when the text changed. */
void welle_pi_display_present(void* handle, const char* const* lines, int cols, int rows);

/* Last call; the plugin is never unloaded. */
void welle_pi_display_close(void* handle);

#ifdef __cplusplus
}
#endif

#endif /* WELLE_PI_DISPLAY_PLUGIN_H */
