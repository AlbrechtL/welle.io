/******************************************************************************\
 * Description:
 * Symbols every welle-pi plugin exports (C, stable)
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
#ifndef WELLE_PI_PLUGIN_H
#define WELLE_PI_PLUGIN_H

/* welle-pi-<name>.so exports, besides the symbols of each kind it
 * implements (input_plugin.h, display_plugin.h):
 *   const char welle_pi_plugin_name[] = "<name>";
 *   const char welle_pi_plugin_description[] = "...";
 * One file can implement several kinds, e.g. a board with a display and
 * buttons; both entries of welle-pi.json then get the same dlopen handle. */

#ifdef __cplusplus
extern "C" {
#endif

extern const char welle_pi_plugin_name[];
extern const char welle_pi_plugin_description[];

#ifdef __cplusplus
}
#endif

#endif /* WELLE_PI_PLUGIN_H */
