/******************************************************************************\
 * Description:
 * welle-pi display plugin: character LCD on /dev/lcd (kernel charlcd, e.g. HD44780)
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

#include "hd44780_charset.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char welle_pi_plugin_name[] = "lcd";
const char welle_pi_plugin_description[] = "character LCD on /dev/lcd (kernel charlcd)";

const int welle_pi_display_abi = WELLE_PI_DISPLAY_ABI;

#define OF_NODE "/sys/devices/platform/auxdisplay/of_node/"

struct lcd {
    int fd;
    int rows;
    char** shown;   /* per row; only changed rows are written */
};

/* device tree property: one big-endian u32, 0 if missing */
static unsigned read_of_u32(const char* path)
{
    unsigned char b[4];
    FILE* f = fopen(path, "rb");
    size_t n = f ? fread(b, 1, 4, f) : 0;
    if (f) fclose(f);
    return n == 4 ? (unsigned)b[0] << 24 | b[1] << 16 | b[2] << 8 | b[3] : 0;
}

static int by_cp(const void* key, const void* entry)
{
    unsigned cp = *(const unsigned*)key, other = ((const struct hd44780_char*)entry)->cp;
    return cp < other ? -1 : cp > other;
}

/* UTF-8 line to ROM codes; '?' for characters the ROM lacks */
static size_t to_hd44780(const char* s, char* out, size_t size)
{
    size_t n = 0;
    while (*s && n + 2 < size) {
        unsigned char c = (unsigned char)*s++;
        if (c < 0x80) {
            out[n++] = c == '\\' ? '|' : c == '~' ? '-' : (char)c;   /* not in ROM A00 */
            continue;
        }
        int more = c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : c >= 0xc0 ? 1 : 0;
        unsigned cp = c & (0x3f >> more);
        for (; more && (*s & 0xc0) == 0x80; --more) cp = cp << 6 | (*s++ & 0x3f);
        const struct hd44780_char* m = more ? NULL :
            bsearch(&cp, hd44780_charset, sizeof hd44780_charset / sizeof *hd44780_charset,
                    sizeof *hd44780_charset, by_cp);
        for (const char* r = m ? m->rom : "?"; *r; ++r) out[n++] = *r;
    }
    return n;
}

static void put(struct lcd* lcd, const char* s, size_t len)
{
    if (write(lcd->fd, s, len) < 0) perror("lcd");
}

/* {"plugin": "lcd"}; size from the device tree */
void* welle_pi_display_open(const char* config, int* cols, int* rows)
{
    (void)config;
    *cols = (int)read_of_u32(OF_NODE "display-width-chars");
    *rows = (int)read_of_u32(OF_NODE "display-height-chars");
    int fd = *cols > 0 && *rows > 0 ? open("/dev/lcd", O_WRONLY) : -1;
    if (fd < 0) {
        fprintf(stderr, "lcd: no character LCD (/dev/lcd)\n");
        return NULL;
    }
    struct lcd* lcd = calloc(1, sizeof *lcd);
    lcd->fd = fd;
    lcd->rows = *rows;
    lcd->shown = calloc((size_t)*rows, sizeof *lcd->shown);
    put(lcd, "\x1b[2J\x1b[L+", 8);   /* clear, backlight on */
    return lcd;
}

void welle_pi_display_present(void* handle, const char* const* lines, int cols, int rows)
{
    (void)cols;
    struct lcd* lcd = handle;
    char buf[256];
    for (int row = 0; row < rows && row < lcd->rows; ++row) {
        if (lcd->shown[row] && !strcmp(lcd->shown[row], lines[row])) continue;
        size_t n = (size_t)snprintf(buf, sizeof buf, "\x1b[Lx0y%d;", row);
        n += to_hd44780(lines[row], buf + n, sizeof buf - n);
        put(lcd, buf, n);
        free(lcd->shown[row]);
        lcd->shown[row] = strdup(lines[row]);
    }
}

void welle_pi_display_close(void* handle)
{
    struct lcd* lcd = handle;
    put(lcd, "\x1b[2J\x1b[L-", 8);   /* clear, backlight off */
    close(lcd->fd);
    for (int row = 0; row < lcd->rows; ++row) free(lcd->shown[row]);
    free(lcd->shown);
    free(lcd);
}
