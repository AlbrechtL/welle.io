/******************************************************************************\
 * Description:
 * welle-pi display plugin: text on a Linux framebuffer (e.g. SSD1306 OLED)
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

#include "font8x8_basic.h"
#include "font8x8_ext_latin.h"

#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

const char welle_pi_plugin_name[] = "fb";
const char welle_pi_plugin_description[] = "8x8 text on a framebuffer (/dev/fbN, 16 or 32 bpp)";

const int welle_pi_display_abi = WELLE_PI_DISPLAY_ABI;

struct fb {
    uint8_t* mem;
    size_t size;
    unsigned stride;   /* bytes per pixel row */
    unsigned bpp;
    unsigned width, height;
};

/* Next code point of UTF-8 s; '?' outside the font (U+0000-007F, U+00A0-00FF) */
static const char* glyph(const char* s, const char** next)
{
    unsigned char c = (unsigned char)s[0];
    unsigned cp = '?';
    *next = s + 1;
    if (c < 0x80) {
        cp = c;
    } else if ((c & 0xe0) == 0xc0 && s[1]) {
        cp = ((c & 0x1fu) << 6) | (s[1] & 0x3fu);
        *next = s + 2;
    } else {
        while ((**next & 0xc0) == 0x80) (*next)++;
    }
    if (cp >= 0x20 && cp < 0x80) return font8x8_basic[cp];
    if (cp >= 0xa0 && cp <= 0xff) return font8x8_ext_latin[cp - 0xa0];
    return font8x8_basic['?'];
}

static void put_pixel(struct fb* fb, unsigned x, unsigned y, int on)
{
    uint8_t* p = fb->mem + y * fb->stride + x * (fb->bpp / 8);
    if (fb->bpp == 32) *(uint32_t*)p = on ? 0xffffffff : 0;
    else *(uint16_t*)p = on ? 0xffff : 0;
}

/* config e.g. {"plugin":"fb","device":"/dev/fb1"}, default /dev/fb0 */
void* welle_pi_display_open(const char* config, int* cols, int* rows)
{
    char device[256] = "/dev/fb0";
    const char* key = strstr(config, "\"device\":\"");
    if (key) {
        key += 10;
        const char* end = strchr(key, '"');
        if (end) snprintf(device, sizeof(device), "%.*s", (int)(end - key), key);
    }

    int fd = open(device, O_RDWR);
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    if (fd < 0 || ioctl(fd, FBIOGET_VSCREENINFO, &var) || ioctl(fd, FBIOGET_FSCREENINFO, &fix)) {
        perror(device);
        if (fd >= 0) close(fd);
        return NULL;
    }
    if (var.bits_per_pixel != 16 && var.bits_per_pixel != 32) {
        fprintf(stderr, "fb: %s has %u bpp, only 16 and 32 supported\n", device, var.bits_per_pixel);
        close(fd);
        return NULL;
    }
    /* Without fbcon nobody sets the mode: DRM fbdev emulation (ssd130x)
     * keeps the panel off until fb_set_par */
    var.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;
    ioctl(fd, FBIOPUT_VSCREENINFO, &var);
    ioctl(fd, FBIOBLANK, FB_BLANK_UNBLANK);
    void* mem = mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mem == MAP_FAILED) {
        perror(device);
        return NULL;
    }

    struct fb* fb = malloc(sizeof(*fb));
    fb->mem = mem;
    fb->size = fix.smem_len;
    fb->stride = fix.line_length;
    fb->bpp = var.bits_per_pixel;
    fb->width = var.xres;
    fb->height = var.yres;
    memset(fb->mem, 0, fb->size);
    *cols = (int)(fb->width / 8);
    *rows = (int)(fb->height / 8);
    return fb;
}

void welle_pi_display_present(void* handle, const char* const* lines, int cols, int rows)
{
    struct fb* fb = handle;
    memset(fb->mem, 0, fb->size);
    for (int row = 0; row < rows; row++) {
        const char* s = lines[row];
        for (int col = 0; col < cols && *s; col++) {
            const char* g = glyph(s, &s);
            for (unsigned y = 0; y < 8; y++)
                for (unsigned x = 0; x < 8; x++)
                    put_pixel(fb, col * 8 + x, row * 8 + y, (g[y] >> x) & 1);
        }
    }
}

void welle_pi_display_close(void* handle)
{
    struct fb* fb = handle;
    memset(fb->mem, 0, fb->size);
    munmap(fb->mem, fb->size);
    free(fb);
}
