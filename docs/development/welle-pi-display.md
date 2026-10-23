# welle-pi displays

welle-pi shows its screens on a text display. Built in is the terminal,
which also serves for debugging without hardware; hardware displays are
plugins. Input is described in
[welle-pi input sources](welle-pi-input.md).

## Configuration

The `"display"` entry of `welle-pi.json` (same file as `"input"`, see
[Configuration file](welle-pi-input.md#configuration-file)):

```json
{
    "display": {"type": "tty", "cols": 20, "rows": 4}
}
```

| Entry | Display | Notes |
|---|---|---|
| `{"type": "tty"}` | Terminal | `cols` and `rows`, default 78 x 22 (80 x 25 with the border). |
| `{"plugin": "<name>"}` | `welle-pi-<name>.so` | Size from the plugin; further keys are the plugin's. Found as described in [Plugins](welle-pi-input.md#plugins). |

An array shows the same screen on several displays, at the smallest size
of them (`MultiDisplay` in `display_interface.hpp`), e.g. a hardware
display and the terminal next to it:

```json
{
    "display": [{"plugin": "lcd"}, {"type": "tty"}]
}
```

An entry that cannot be used (unknown `type`, plugin not found or
refusing, bad size) is logged and skipped. Without `"display"`, or if no entry is left, welle-pi uses the
terminal with 78 x 22 if stdout is one, otherwise it shows nothing.

## Terminal

`TtyDisplay` (`src/welle-pi/tty_display.cpp`) draws every changed frame
in a box, the size of the configured display:

```
+--------------------+
|Radio Example       |
|11D                 |
|                    |
|Now playing: Artist |
+--------------------+
```

On a terminal it clears the screen before each frame, which also wipes
the messages on stderr: run `welle-pi 2>welle-pi.log`. All messages go to
stderr, so stdout carries only the frames. Written to a file, the frames
follow each other. As a daemon or with `-l`, stdout is the log and the
terminal display draws nothing.

## LCD plugin

`{"plugin": "lcd"}` (`src/welle-pi/plugins/lcd/`) drives a character LCD
through `/dev/lcd`, the kernel's `charlcd` driver. The HD44780 is
configured as a device tree overlay (e.g. `hd44780-i2c-lcd` for a PCF8574
backpack). The plugin reads the size from
`/sys/devices/platform/auxdisplay/of_node/display-{width,height}-chars`
and converts UTF-8 to the HD44780 character set (ROM A00,
`hd44780_charset.h`; accented letters without their own code become the
plain letter, the rest `?`). The code comes from
[liblcd](https://github.com/fridrich/lcd).

It writes only the rows that changed, so the I2C bus is not busy with
unchanged text, switches the backlight on when welle-pi starts and off
when it stops. Without an LCD (no size in the device tree or no
`/dev/lcd`) it refuses and the entry is skipped.

## Framebuffer plugin

`{"plugin": "fb", "device": "/dev/fb1"}` (`src/welle-pi/plugins/fb/`,
default device `/dev/fb0`) draws the text with an 8x8 font
([font8x8](https://github.com/dhepper/font8x8), public domain; U+0020 to
U+007F and U+00A0 to U+00FF, `?` for the rest) on any framebuffer with 16
or 32 bits per pixel, e.g. a 128 x 64 SSD1306 OLED gives 16 x 8
characters. The kernel drives the panel; for an I2C SSD1306 on a Raspberry
Pi:

```
dtparam=i2c_arm=on
dtoverlay=ssd1306          # ,inverted if the picture is mirrored
```

The `ssd130x` DRM driver then shows up as `ssd130xdrmfb` in
`/sys/class/graphics/fb*/name`. When it is the only framebuffer, the
kernel console draws on it too; `fbcon=map:1` in `cmdline.txt` keeps the
console off `fb0`.

## How screens are drawn

* A screen (`UIScreen`, `src/welle-pi/ui_screen.hpp`) fills a `Frame`
  (`display_interface.hpp`): `rows` lines of UTF-8 text and the time
  `now` of this tick. It neither waits nor writes to the display.
* `UIManager` calls `render()` of the top screen every 50 ms and right
  after a key or a screen change, fits every line to exactly `cols` code
  points and passes the frame to `IDisplay::present()`. Most frames are
  identical to the previous one; displays skip them.
* Animations are computed from `Frame::now`. The DLS marquee of
  `RadioScreen`: text, five spaces, text again, one step per 200 ms
  (`DLS_STEP_MS`). Each message runs round
  once and rests 1 s (`DLS_REST_MS`) before the next one; the last one
  keeps running round. From five rows on, the DLS is word-wrapped
  instead, filling the rows from the bottom: 1 s per line
  (`DLS_LINE_MS`), the first lines stay until they are read, then a
  longer message moves up one line per second.
* Layout: on two rows the programme and the DLS (the channel while there
  is no DLS), on three and four rows the programme, the channel and the
  DLS marquee on the last row, on more rows the programme, the channel,
  a blank row and the wrapped DLS.
  Lists put the focused item in the middle; with fewer items than rows
  each item is shown once. A station whose name is in the list more than
  once (the same service in several ensembles, or different services
  with the same label) shows its channel at the right. A focused name
  longer than its line shows its start and its end in turns, 1 s each
  (`NAME_SWITCH_MS`).

## Writing a display plugin

The interface is plain C, `src/welle-pi/display_plugin.h`. Besides its name
and description (see [Writing an input plugin](welle-pi-input.md#writing-an-input-plugin))
a plugin exports four symbols:

```c
#include "display_plugin.h"

const char welle_pi_plugin_name[] = "oled";
const char welle_pi_plugin_description[] = "SSD1306 128 x 64 on I2C";

const int welle_pi_display_abi = WELLE_PI_DISPLAY_ABI;

void* welle_pi_display_open(const char* config, int* cols, int* rows)
{
    /* open the device, read the settings from config, switch it on */
    *cols = 20;
    *rows = 4;
    return device;          /* NULL: not usable, the entry is skipped */
}

void welle_pi_display_present(void* handle, const char* const* lines, int cols, int rows)
{
    /* show rows lines of UTF-8, each exactly cols code points */
}

void welle_pi_display_close(void* handle)
{
    /* switch off, close the device */
}
```

* `config` is the plugin's own entry as JSON text, including `plugin`.
* `present()` is only called when the text changed, so a plugin can
  write the frame as it is. When the display is mirrored next to a smaller
  one, `cols` and `rows` are that smaller size: pad or centre as needed.
* All calls come from the UI thread; `present()` should not block for
  long, the UI ticks every 50 ms.
* A graphical display (OLED, small TFT) draws the lines with a fixed font;
  e.g. an SSD1306 with 128 x 64 pixels and a 6 x 8 font has 21 x 8 cells.

Build it like an input plugin, as `welle-pi-<name>.so`. A complete
example is `src/welle-pi/plugins/example.c`: 16 x 2 characters, it
appends every frame to the file given as `"file"`.

A display built into welle-pi instead implements `IDisplay`
(`display_interface.hpp`) and gets a `type` in `makeOneDisplay()` in
`src/welle-pi/welle-pi.cpp`; only worth it for displays without external
dependencies, like the terminal.

## Tests

`unit.display` (`src/tests/display_tests.cpp`): UTF-8 fitting, the
marquee timing, list layout, `TtyDisplay` output and the `UIManager`
loop. `unit.navigation` covers the screen stack. `unit.display_plugin`
(`src/tests/display_plugin_tests.cpp`) loads the example plugin by name and
checks that unchanged frames are not passed on and bad entries are refused.

```sh
ctest --test-dir build -R 'unit.(display|navigation|display_plugin)' --output-on-failure
```
