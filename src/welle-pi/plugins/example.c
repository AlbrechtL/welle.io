/******************************************************************************\
 * Description:
 * Example welle-pi plugin, input and display in one file, used by
 * input_plugin_tests and display_plugin_tests
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
#include "input_plugin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char welle_pi_plugin_name[] = "example";
const char welle_pi_plugin_description[] = "keys from the config, frames to a file";

const int welle_pi_input_abi = WELLE_PI_INPUT_ABI;
const int welle_pi_display_abi = WELLE_PI_DISPLAY_ABI;

/* config e.g. {"plugin":"example","keys":"d+hme"}: d = DOWN, e = ENTER,
 * + = VOLUME_UP, h = HOME, m = MUTE */
int welle_pi_input_run(const struct welle_pi_input_host* host, const char* config)
{
    const char* keys = strstr(config, "\"keys\":\"");
    if (keys) {
        for (keys += 8; *keys && *keys != '"'; keys++) {
            if (*keys == 'd') host->push(host->ctx, WELLE_PI_DOWN);
            else if (*keys == 'e') host->push(host->ctx, WELLE_PI_ENTER);
            else if (*keys == '+') host->push(host->ctx, WELLE_PI_VOLUME_UP);
            else if (*keys == 'h') host->push(host->ctx, WELLE_PI_HOME);
            else if (*keys == 'm') host->push(host->ctx, WELLE_PI_MUTE);
        }
    }
    while (!host->stop_requested(host->ctx))
        usleep(10000);
    return 0;
}

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
