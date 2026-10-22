# welle-pi input sources

welle-pi is controlled by input sources: each turns a device (terminal,
buttons, …) into the actions `UP`, `DOWN`, `LEFT`, `RIGHT`, `ENTER`, `MENU`
and `QUIT`. Which sources run is set in a configuration file, new ones can
be added as plugins (shared objects) without rebuilding welle-pi.

## Configuration file

welle-pi reads its settings from the first file found of

1. `$XDG_CONFIG_HOME/welle-pi/welle-pi.json`, or `~/.config/welle-pi/welle-pi.json`
   if `XDG_CONFIG_HOME` is not set
2. `/etc/welle-pi/welle-pi.json`

The systemd service (`scripts/welle-pi.service`) has no `User=`, and
systemd sets `HOME` only together with `User=`: use
`/etc/welle-pi/welle-pi.json` there. The file is only read, never written. It is not `stations.json`:
that one is a cache in `$XDG_CACHE_HOME/welle-pi/` that welle-pi rewrites
after each scan.

```json
{
    "input": [
        {"type": "stdin"},
        {"plugin": "buttons", "pins": [17, 27]}
    ]
}
```

* Without the file, or without `"input"`, welle-pi starts `stdin`.
* `{"type": …}` is a built-in source, `{"plugin": …}` a plugin (see
  [Plugins](#plugins)).
* An entry that fails (unknown `type`, plugin not found, wrong ABI) is
  logged on stderr and skipped. welle-pi keeps playing, it is a radio.
* An empty array `[]` starts no input source at all.

## Built-in source

| `type` | Device | Notes |
|---|---|---|
| `stdin` | Terminal | Arrow keys or `u` `d` `l` `r`; `e` or Enter: `ENTER`; `m` or Esc: `MENU`; `q` or `.`: `QUIT`. Only started when stdin is a terminal (not under systemd). |

## How the sources run

Every source runs in its own thread and pushes its actions into the
`InputQueue` (`src/welle-pi/input_queue.hpp`). The main loop takes them
from there and passes them to the `UIManager`. No source is polled by
welle-pi, so a source may block in whatever its device needs (`read()`,
`poll()`, libgpiod edge events). When welle-pi stops, it waits for all
input threads to return.

Code: `startInputSources()` in `src/welle-pi/input_sources.cpp`, the
built-in source in `input_stdin.cpp`.

## Plugins

Input sources and displays ([welle-pi displays](welle-pi-display.md)) can
be plugins: shared objects with a plain C interface, built with welle.io or
separately. `{"plugin": "buttons"}` in the `"input"` array loads
`welle-pi-buttons.so` from the first of

1. `welle-pi-plugins/` next to the welle-pi executable: the plugins built
   with welle.io land there, so `build/welle-pi` runs without installing
2. `<libdir>/welle-pi/`, e.g. `/usr/lib64/welle-pi/` (`WELLE_PI_PLUGIN_DIR`)

A name containing `/` is used as a path, e.g. for a plugin built in
another directory. The plugin is loaded with `dlopen(RTLD_NOW |
RTLD_LOCAL)` and never unloaded; its `welle_pi_<kind>_abi` must match.
One file can implement several kinds, e.g. a board with a display and
buttons: `{"plugin": "board"}` in `"input"` and in `"display"` loads the
same `welle-pi-board.so` once, so both parts can share their state.
Code: `src/welle-pi/plugin_loader.cpp`. The headers are installed to
`<includedir>/welle-pi/`.

Plugins built with welle.io are in `src/welle-pi/plugins/`: single files
in its root, plugins with their own dependencies in a subdirectory with a
`CMakeLists.txt` that skips the plugin if a dependency is missing. Each
calls `welle_pi_plugin(<name> <sources>...)` from
`src/welle-pi/plugins/CMakeLists.txt`, which names the shared object, puts
it into `welle-pi-plugins/` and installs it.

## Writing an input plugin

The interface is plain C, `src/welle-pi/input_plugin.h`, so plugins can be
written in C or C++ and built separately. A plugin exports its name and
a description (`plugin.h`, every plugin) and two input symbols:

```c
#include "input_plugin.h"

const char welle_pi_plugin_name[] = "buttons";
const char welle_pi_plugin_description[] = "push buttons on GPIO lines";

const int welle_pi_input_abi = WELLE_PI_INPUT_ABI;

int welle_pi_input_run(const struct welle_pi_input_host* host, const char* config)
{
    /* open the device, read the settings from config */
    while (!host->stop_requested(host->ctx)) {
        /* wait for the device, at most a few 100 ms */
        host->push(host->ctx, WELLE_PI_DOWN);
    }
    return 0;
}
```

* `welle_pi_input_abi` must be `WELLE_PI_INPUT_ABI`. welle-pi refuses a
  plugin with another value.
* `welle_pi_input_run` is called once, in its own thread. It has to check
  `stop_requested()` regularly and return soon after it is nonzero,
  otherwise welle-pi hangs on exit. A nonzero return value is logged.
* `config` is the plugin's own entry of the `"input"` array as JSON text,
  including `plugin`. Add whatever keys the plugin needs. C++
  plugins can parse it with nlohmann/json (`src/libs/json.hpp`).
* `push()` accepts the values of `enum welle_pi_action`; other values are
  ignored. Its values are those of `InputAction` (a `static_assert` in
  `input_sources.cpp` checks that) and are never renumbered.

Build it, for example:

```sh
gcc -shared -fPIC -I/usr/include/welle-pi buttons.c -o welle-pi-buttons.so
```

A complete example is `src/welle-pi/plugins/example.c`: it pushes the
keys listed in its `"keys"` entry and waits for the stop.

## Tests

`unit.input_plugin` (`src/tests/input_plugin_tests.cpp`) loads the example
plugin through a configuration entry, checks that its actions arrive in
order, and that bad entries are skipped:

```sh
ctest --test-dir build -R unit.input_plugin --output-on-failure
```
