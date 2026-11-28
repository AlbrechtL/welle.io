/******************************************************************************\
 * Description:
 * Finds and loads welle-pi plugins (shared objects)
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
#ifndef PLUGIN_LOADER_HPP
#define PLUGIN_LOADER_HPP

#include <string>

/* kind "input" or "display", name e.g. "buttons": welle-pi-buttons.so in
 * welle-pi-plugins/ next to the executable (build tree), then in
 * WELLE_PI_PLUGIN_DIR (installed). A name with a '/' is a path. Checks
 * welle_pi_<kind>_abi == abi, logs name and description. Returns the
 * dlopen() handle, never unloaded; nullptr after logging why. */
void* loadPlugin(const std::string& kind, const std::string& name, int abi);

#endif // PLUGIN_LOADER_HPP
