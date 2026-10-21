/******************************************************************************\
 * Description:
 * Example welle-pi display plugin: appends every frame to a file
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int welle_pi_display_abi = WELLE_PI_DISPLAY_ABI;

/* config e.g. {"plugin":"example","file":"/tmp/frames.txt"}: 16 x 2 */
void* welle_pi_display_open(const char* config, int* cols, int* rows)
{
    const char* key = strstr(config, "\"file\":\"");
    if (!key) {
        fprintf(stderr, "display example: \"file\" missing\n");
        return NULL;
    }
    key += 8;
    const char* end = strchr(key, '"');
    if (!end) return NULL;
    char path[4096];
    snprintf(path, sizeof(path), "%.*s", (int)(end - key), key);
    FILE* out = fopen(path, "w");
    if (!out) return NULL;
    *cols = 16;
    *rows = 2;
    return out;
}

void welle_pi_display_present(void* handle, const char* const* lines, int cols, int rows)
{
    FILE* out = handle;
    fprintf(out, "frame %d x %d\n", cols, rows);
    for (int i = 0; i < rows; i++) fprintf(out, "%s\n", lines[i]);
    fflush(out);
}

void welle_pi_display_close(void* handle)
{
    FILE* out = handle;
    fprintf(out, "closed\n");
    fclose(out);
}
