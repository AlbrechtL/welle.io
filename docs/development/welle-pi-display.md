# welle-pi displays

welle-pi shows its screens on a text display. Built in is the terminal,
which also serves for debugging without hardware. Input is described in
[welle-pi input sources](welle-pi-input.md).

## Configuration

The `"display"` entry of `welle-pi.json` (same file as `"input"`, see
[Configuration file](welle-pi-input.md#configuration-file)):

```json
{
    "display": {"type": "tty", "cols": 20, "rows": 4}
}
```

| `type` | Display | Notes |
|---|---|---|
| `tty` | Terminal | `cols` and `rows`, default 40 x 4. |

An array shows the same screen on several displays, at the smallest size
of them (`MultiDisplay` in `display_interface.hpp`), e.g. a hardware
display and the terminal next to it.

An entry that cannot be used (unknown `type`, bad size) is logged and
skipped. Without `"display"`, or if no entry is left, welle-pi uses the
terminal with 40 x 4 if stdout is one, otherwise it shows nothing.

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
  keeps running round.
* Layout: on two rows the programme and the DLS (the channel while there
  is no DLS), on more rows the programme, the channel and the DLS on the
  last row. Lists put the focused item in the middle.

## A new display

Implement `IDisplay`: `cols()`, `rows()` and `present(const Frame&)`,
optionally `backlightOn()`/`backlightOff()`, and add a `type` to
`makeDisplay()` in `src/welle-pi/welle-pi.cpp`. A graphical display
(OLED, small TFT) can draw the lines with a fixed font; e.g. an SSD1306
with 128 x 64 pixels and a 6 x 8 font has 21 x 8 cells.

## Tests

`unit.display` (`src/tests/display_tests.cpp`): UTF-8 fitting, the
marquee timing, list layout, `TtyDisplay` output and the `UIManager`
loop. `unit.navigation` covers the screen stack.

```sh
ctest --test-dir build -R 'unit.(display|navigation)' --output-on-failure
```
