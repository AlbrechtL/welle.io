# Notes for AI coding agents

welle.io is a DAB/DAB+ software defined radio: a decoder library
(`src/backend`, `src/libs`), the Qt 6 / QML desktop application `welle-io`
(`src/welle-gui`) and the command line tool `welle-cli`. See `README.md`.

## Rules

* **Never commit or push** unless the user explicitly asks for it. Leave the
  changes in the working tree.
* The working tree often contains the user's own uncommitted work. Don't
  revert, stash, reformat or commit it. If you are asked to commit, commit
  only your own files (`git add <files>`, not `git add -A`).
* The development branch is **`next`**. `master` is the release branch.
* Don't rewrite history that is already on `origin` (no force push).
* Commit messages, if asked for: `type(scope): summary` (`feat(gui):`,
  `fix(backend):`, `test(gui):`, `docs:` …), English, body explains what
  and why.
* Android is not worked on at the moment: don't change `android.yml` or
  Android specific code, but don't use QML APIs newer than Qt 6.5 either.
* New source files: a short description and the GPL v2+ text, plus the line
  "This file was generated with the help of an AI assistant (Claude Code)."
  **No copyright line and no author line.** Banners of existing files (also
  copied SDR-J code) stay as they are.
* Technical documentation of a feature goes into `docs/` (e.g.
  `docs/spi-station-logos.md`), **not into `README.md`**.
* Backend changes have to be reviewable: change as few existing files and
  lines as possible, extend existing classes instead of adding new base
  classes or moving code (e.g. the SPI subchannel reuses `DabAudio` with a
  `DabProcessor`), keep new code in new, self-contained files. Split the
  work into commits that each build and pass their tests: bug fixes first
  and separately, then backend, then GUI. Check intermediate commits in a
  `git worktree` before reporting them as done.
* Don't change `welle-cli` unless asked; it may be used as a read-only tool.

## Building

```sh
cmake -B build -DBUILD_TESTS=ON \
      -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/gcc_64 \
      -DWELLE_TEST_DATA_DIR=$PWD/../welle.io-test-data
cmake --build build -j$(nproc)
```

* `BUILD_WELLE_IO` (GUI) is on by default. Qt 6.8 or newer is required.
* If a `builddir/` (flatpak-builder output) exists in the repository,
  configuring with the GUI hangs: qmlimportscanner follows
  `builddir/var/run -> /run`. Remove `builddir/` (ask first) or configure a
  copy without it.
* New QML files must be added to `src/welle-gui/resources.qrc`; new QML
  folders also to the wildcards in `src/welle-gui/welle-gui.pro` (used by
  lupdate).

## Test data

The recordings are in the **private** repository `welle.io-test-data`
(locally `../welle.io-test-data`, on GitHub `AlbrechtL/welle.io-test-data`).
They contain copyrighted broadcast content: **never copy recordings or
decoded audio into welle.io**, welle.io only contains the test code.

* `manifest.json`: every clip with what has to be decoded from it
  (ensemble, services, DAB/DAB+, bitrate, DLS and slide minimums).
* Clips are stored compressed (`clips/<id>.<format>.iq.zst`); unpack them
  with `scripts/prepare_test_data.py --unpack`.
* The tests find the data with `WELLE_TEST_DATA_DIR` (environment variable or
  CMake cache variable). Without it the tests that need recordings skip.
* GitHub Actions (`.github/workflows/tests.yml`) checks the repository out
  with the secret `WELLE_TEST_DATA_TOKEN`.
* Useful clips: `odr_dabmod` (synthetic, DAB + DAB+, DLS, slides; the main
  clip of the UI tests), `uk_11d_2020` (real, with programme types),
  `au_syd_9a` (24 services).
* The IQ clips (8–30 s) are too short for data services such as SPI, whose
  carousels take 10–15 minutes. `spi/*.bin` are recorded SPI subchannels
  instead (the logical frames after channel decoding, a few MB) and listed
  in the `"spi"` section of `manifest.json` (bitrate, packet address,
  expected logos). Each becomes `backend.spi.<id>`. Record new ones with
  `spi_capture` (see "Live reception"); the expectations come from a replay
  (`spi_capture --replay`), because the decoding is deterministic.

## Automated tests

CTest, details in `src/tests/README.md`.

| Command | Runs |
|---|---|
| `ctest --test-dir build -j8` | Everything |
| `ctest --test-dir build -L unit` / `-L backend` / `-L gui` | One group |
| `ctest --test-dir build -R gui.ui.playback --output-on-failure` | One test |
| `build/src/tests/tst_ui_settings switchesLanguage` | One test function |

* `backend.recording.<clip>` decodes every recording of the manifest and
  compares it with its expectations.
* `gui.*` start the real user interface. `gui.ui.<area>`
  (`src/tests/gui/ui/tst_ui_<area>.cpp`) click through `MainView.qml` with
  real mouse and keyboard events while a recording plays as RAW file input,
  and compare what is shown with the manifest.
* The UI harness (`UiApp` in `src/tests/gui/ui/ui_app.h`, base class
  `UiTestCase`) finds elements by `objectName`, refuses clicks on covered or
  moving items, checks that nothing sticks out of the window and that every
  control has an accessible name. **Every QML warning fails the test.**
* Run the GUI tests with both renderers before saying the UI works:
  `ctest -L gui` (OpenGL if available) and
  `QT_QUICK_BACKEND=software ctest -L gui`.
* Changing the UI means changing or adding UI tests in the same change. A
  new file `gui/ui/tst_ui_<area>.cpp` becomes `gui.ui.<area>` when CMake
  runs again.
* Prove that a new test can fail (e.g. by breaking the code temporarily),
  then restore the code.

## Live reception (rtl_tcp)

The maintainer can provide an rtl_tcp server for live reception; ask for
its address, it isn't written down here. Ensembles received there with SPI
(station logos): 11D (WDR-EPG, 8 kbit/s, packet address 1) and 5C (EPG
Deutschland, 24 kbit/s, packet address 3).

* The server serves **one client at a time**: don't run two receivers at
  once, and check that the user's welle.io isn't connected.
* `build/src/tests/spi_capture <host> <port> <channel> <seconds> <dir>`
  receives the SPI services of an ensemble, saves the logos and the logical
  frames of each SPI subchannel; `spi_capture --replay <file> <kbit/s>
  <address> <dir>` decodes such a file again and lists every MOT object.
* welle-cli works as a quick probe of an ensemble without changing it:
  `build/welle-cli -c 11D -F rtl_tcp,<host>:<port> -g -1 -w 18001 -C 1`
  and read `http://localhost:18001/mux.json`. Use a new port for each run
  and stop it with `pkill -x welle-cli` (not `pkill -f`, which also matches
  the shell running the command).
* Don't run CPU heavy work (e.g. `ctest -j4`) during a live capture with a
  Debug build: the receiver loses samples, which shows up as thousands of
  CRC errors.
* SPI carousels are slow: a complete cycle takes 10–15 minutes, and EPG
  Deutschland sends its SI object only once per cycle. Logos can only be
  assigned to stations after the SI object, so the first logos can take up
  to about 15 minutes. That is not a decoder problem.
* To test the real GUI without touching the user's settings, run it
  offscreen with a copy of the settings:
  `XDG_CONFIG_HOME=<tmp>/cfg XDG_DATA_HOME=<tmp>/data QT_QPA_PLATFORM=offscreen build/welle-io`
  (the user's settings are in `~/.config/welle.io/welle.io.conf`).

## DAB specifications

The relevant ETSI documents: EN 300 401 (DAB: FIC/FIGs, packet mode, FEC
for packet mode), EN 301 234 (MOT), TS 101 756 (registered tables, e.g.
user application types), TS 102 818 (SPI XML) and TS 102 371 (SPI binary
encoding and transport). etsi.org blocks curl and wget; WebFetch saves the
PDF to a file, which `pdftotext -layout` converts to searchable text.

## UI screenshots for AI assistants

There is no display; the tests render offscreen and can save screenshots,
which an assistant can then look at (read the PNG files):

```sh
WELLE_TEST_SCREENSHOT_DIR=/tmp/shots ctest --test-dir build -L gui
# or one test
WELLE_TEST_SCREENSHOT_DIR=/tmp/shots build/src/tests/tst_ui_responsive
```

* The tests save the important states, e.g. `responsive-360x640-dark.png`,
  `playback-cards-800x480.png`, `settings-1200.png`,
  `dashboard-all-views.png`, `navigation-drawer.png`.
* For another state, add `app.resize(width, height)` and
  `app.screenshot("name")` to a test (or write a small one) and run it.
* `WELLE_TEST_LANGUAGE=de_DE.UTF-8` shows the UI in German (default:
  English). `QT_QPA_PLATFORM=xcb` shows the window on a real display instead.
* With OpenGL (the default if Mesa/GL is available) the screenshots look
  like the app on a desktop, including Material shadows. With
  `QT_QUICK_BACKEND=software` shadows and other shader effects are missing;
  that is expected, the UI shows borders instead.
* The harness already handles the offscreen quirks: a 2560 x 1600 offscreen
  screen, the window is created at that size and then resized (the OpenGL
  surface keeps its first size), empty grabs are repeated.
* Check screenshots at small sizes too, e.g. 360 x 640 (phone) and
  800 x 480 (Raspberry Pi 7" display; must work without scrolling).

## User interface (QML)

Structure and conventions: `src/welle-gui/QML/README.md`. In short:

* Material 3 style, design tokens in `controls/Theme.qml` (no hard-coded
  colors or sizes), elevation via `Theme.elevation()` (0 with the software
  renderer).
* No global ids such as `mainWindow` in components: shared state
  (`StationLists`, `Player`, `MotSlideModel`) is created in `MainView.qml`
  and passed down as properties.
* Every element a test uses gets a stable, untranslated `objectName`; every
  icon-only control an `Accessible.name`.
* `Settings` keys (QSettings) of existing installations must not change.
* Responsive: phone layout below 600 px window width, now playing side by
  side from 840 px pane width.

## Translations

`src/welle-gui/i18n/*.ts`. After changing or moving texts:

```sh
cd src/welle-gui
PATH=$HOME/Qt/6.12.0/gcc_64/bin:$PATH lupdate-pro welle-gui.pro
PATH=$HOME/Qt/6.12.0/gcc_64/bin:$PATH lrelease-pro welle-gui.pro
```

`qsTr()` uses the file name as context, so moving a file moves its texts;
lupdate's same-text heuristic keeps the translations. Combo box entries use
`QT_TRANSLATE_NOOP("Settings", ...)`. German translations are maintained,
add them for new texts.

## Pitfalls seen in this code base

* `.gitignore` ignores `ui_*.h` (uic output). New files with that name
  pattern need an exception, otherwise they are silently not committed.
* QTest installs its own message handler in `qExec()`; a handler installed
  before is replaced (the UI harness installs its QML warning collector when
  the UI starts).
* A delegate property named like a model role (`stationName`) hides the role.
* A layout inside a layout has `Layout.fillWidth`/`fillHeight` true by
  default; set it to false for fixed sizes.
* Sizes in a layout that depend on the layout's own size cause "recursive
  rearrange" warnings.
* A string list as model of a ComboBox is no JS `Array` in QML.
* `ChartView.theme` has no change signal.
* Click only after animations (drawer, StackView) have finished:
  `waitForPage()`, wait for `position >= 1`.
* GUI tests run with their own temporary `XDG_CONFIG_HOME` and
  `XDG_DATA_HOME` (`gui_test_main.h`), so they neither read nor change the
  settings and caches (e.g. station logos) of an installed welle.io. New
  persistent data has to go below these directories (`QSettings`,
  `QStandardPaths`).
* `QStandardPaths::AppDataLocation` is `~/.local/share/welle.io/welle.io`
  (organization and application name are both "welle.io"); the station
  logos are in its `logos/` folder.
* `src/libs/fec` is C code: include `fec.h` inside `extern "C" { }`,
  otherwise linking fails.
* FIG 0/3 entries are 5 bytes, 7 only with the CAOrg flag; FIG 0/8 and
  0/13 carry SCIdS, not the component number. Such parsing errors show up
  as packet mode components without subchannel (subchannel -1).
* Packets with address 1022 are packet FEC data without CRC. Without FEC
  handling they look like a burst of CRC errors every 103 packets.
* The example in annex C of TS 102 371 uses wrong values for the
  multimedia `type`; annex F (normative) is right.

---

*This file was generated with an AI assistant (Claude Code).*
