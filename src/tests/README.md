# welle.io tests

Automated tests for the DAB/DAB+ decoder library, the `welle-io` GUI and the
command line tool `welle-cli`.
They are built with CMake and run with CTest.

## Overview

| CTest test | Label | What it checks | Needs recordings |
|---|---|---|---|
| `unit.rawfile` | `unit` | Reading IQ files in the formats u8, s8, s16le, s16be and cf32 | no |
| `unit.dabconstants` | `unit` | DAB mode parameters, label character sets, subchannel bitrates | no |
| `unit.reed_solomon` | `unit` | Reed-Solomon codec used for DAB+ | no |
| `unit.mp2_scf_crc` | `unit` | Scale Factor CRC of MP2 frames: Scale Factors with errors are muted | no |
| `unit.rtl_agc` | `unit` | Software AGC of the RTL-SDR and rtl_tcp inputs with a simulated tuner (also the measured gain steps of a real R820T): finds the gain without clipping and keeps it | no |
| `unit.halfband_decimator` | `unit` | Decimation 4.096 to 2.048 MS/s of the Airspy input: DAB block unchanged, adjacent blocks suppressed by more than 65 dB | no |
| `backend.recording.<clip>` | `backend` | Decodes one recording and compares everything with its expectations, see below | yes |
| `cli.options` | `cli` | welle-cli command line: help, version, invalid options, missing IQ file | no |
| `cli.recording.<clip>` | `cli` | welle-cli plays one recording with its web server; ensemble and service list of `mux.json` | yes |
| `cli.webServer` | `cli` | `-D -w`: audio format, DLS and slide of every service in `mux.json`, MP3 streams, playlist, FIC, measurements, slides, error codes | yes |
| `cli.webServerFlac` | `cli` | `-O flac`: FLAC stream (skipped if welle-cli was built without FLAC) | yes |
| `cli.carousel` | `cli` | `-C 1`: one decoder that switches between the services | yes |
| `cli.dumpAll` | `cli` | `-D` without web server: `dump.fic`, `.msc` and `.wav` file of every service, DLS on stdout, quit with `.` | yes |
| `cli.testMode` | `cli` | `-t 3`: decodes the recording at full speed and prints error statistics | yes |
| `cli.alsaPlayback` | `cli` | `-p`: plays with the ALSA `null` device, selects programmes on stdin (skipped without ALSA) | yes |
| `gui.radiocontroller` | `gui` | The C++ part of the GUI: opens a recording, scans, plays DAB+ and DAB, DLS and slide | yes |
| `gui.qml_smoke` | `gui` | Starts the complete QML user interface, checks the station list and that QML reports no warnings | yes |
| `gui.ui.playback` | `gui` | Scan with the button of the empty list, play every service by clicking it: station, ensemble, DAB/DAB+, audio format, DLS, slide; play/stop button, keyboard shortcuts, volume | yes |
| `gui.ui.stations` | `gui` | Empty state, scan from the menu, favorites, lists after a restart, clearing the list, long list of the recording with the most services | yes |
| `gui.ui.navigation` | `gui` | Navigation drawer: open/close (button, Esc, click outside), station lists, settings and about pages, back, exit | yes |
| `gui.ui.settings` | `gui` | Settings page: existing settings shown and applied, switches and combo boxes by clicking, expert mode, input device stays open, section index, restart | yes |
| `gui.ui.rtltcp` | `gui` | rtl-tcp input: closing the settings keeps the device open, a changed server is opened (no server needed) | no |
| `gui.ui.dashboard` | `gui` | Expert views: old layout taken over, every view added, graphs get data, maximize, move, remove, restart | yes |
| `gui.ui.messages` | `gui` | Snackbar: error until dismissed, info disappears, queue, fits a phone sized window | yes |
| `gui.ui.responsive` | `gui` | Layout from 360 to 1600 px in light and dark theme, mini player and player page | yes |
| `gui.smoke_option` | `gui` | `welle-io --smoke-test` of the build with `smoke/smoke_test.py`, see "Smoke tests of the packages" | yes |

There is one `backend.recording.<clip>` test per recording, e.g.
`backend.recording.odr_dabmod` or `backend.recording.uk_11d_2020`. Without
recordings there is a single `backend.recordings` test, which is skipped.
The GUI tests only exist if welle-io itself is built (`BUILD_WELLE_IO`, on by
default), the `cli` tests only if welle-cli is built (`BUILD_WELLE_CLI`, on by
default). The `cli` tests that need recordings use `odr_dabmod`.

## Getting the recordings

The recordings are kept in the private repository `welle.io-test-data`,
because they contain copyrighted broadcast content. It holds the clips, a
`manifest.json` that describes them and what welle.io has to decode from
them, and the scripts to cut and update them. See its README.

The tests need a checkout of it with unpacked clips:

```sh
git clone <welle.io-test-data repository> ../welle.io-test-data
../welle.io-test-data/scripts/prepare_test_data.py --unpack
```

## Building

```sh
cmake -B build-tests -DBUILD_TESTS=ON \
      -DWELLE_TEST_DATA_DIR=$PWD/../welle.io-test-data
cmake --build build-tests
```

If Qt is not installed system-wide (e.g. with the Qt online installer), add
`-DCMAKE_PREFIX_PATH=$HOME/Qt/<version>/gcc_64`.

CMake reads the list of recordings from the manifest. Run CMake again if the
test data directory changes. A changed manifest is noticed automatically.

## Running the tests with CTest

| Command | Result |
|---|---|
| `ctest --test-dir build-tests` | All tests, one line per test with Passed/Failed |
| `ctest --test-dir build-tests -j8` | The same, 8 tests in parallel (about 20 s instead of 70 s) |
| `ctest --test-dir build-tests --output-on-failure` | Shows the detailed report of every test that failed |
| `ctest --test-dir build-tests -V` | Shows the detailed report of every test |
| `ctest --test-dir build-tests -L unit` | Only the tests with the label `unit` (or `backend`, `gui`) |
| `ctest --test-dir build-tests -R odr_dabmod -V` | Only the tests whose name contains `odr_dabmod`, with details |
| `ctest --test-dir build-tests -N` | Lists the tests without running them |

Without `-V` or `--output-on-failure`, CTest only shows whether a test passed.
`--output-on-failure` is a good default.

### Reading the output of a recording

`ctest -R odr_dabmod -V` shows what was decoded and what was checked:

```
QINFO  : RecordingTests::decodeRecording(odr_dabmod) odr_dabmod (s8, 30.0 s): ODR-DabMux/ODR-DabMod synthetic ensemble ...
  [ OK ] Sync
  [ OK ] Ensemble 0x4fff "OpenDigitalRadio"
  [ OK ] First correct FIB after 0.0 s (max 0.5 s)
  [ OK ] 0x8daa "DAB Test 1": DAB, 128 kbit/s UEP 3, MPEG 1.0 Layer II, 48 kHz Stereo, 29.5 s audio
  [ OK ] 0x8dab "DAB+ Test 2": DAB+, 96 kbit/s EEP 1-A, AAC-LC, 48 kHz Stereo, 29.5 s audio, DLS, 3 slides
  [ OK ] Audio frame errors: 0 of 2456 = 0.0 % (max 2.0 %)
  [ OK ] Uncorrectable DAB+ superframes: 0 of 245 = 0.0 % (max 5.0 %)
  [ OK ] 1 services with DLS (min 1)
  [ OK ] 3 MOT slides (min 2)
PASS   : RecordingTests::decodeRecording(odr_dabmod)
```

* The first line: clip, sample format, length and description of the recording.
* `Sync`: the receiver synchronised to the signal.
* `Ensemble`: ensemble ID and label from the FIC.
* `First correct FIB`: how long the receiver needed to decode the FIC. This
  is mostly the time to correct the frequency offset of the recording.
* One line per service: SId, label, DAB or DAB+, bitrate and protection of
  the subchannel, then for audio services the codec, sample rate, channels
  and the amount of decoded audio, and whether a dynamic label (DLS) or
  slides were received. Data services only show `data`.
* `Audio frame errors`: MP2 and AAC frames with errors, of all audio services.
* `Uncorrectable DAB+ superframes`: DAB+ superframes that Reed-Solomon could
  not correct.
* `services with DLS`, `MOT slides`: dynamic labels and slides.

A failed check is marked `[FAIL]`, and after `--` it says what was expected:

```
  [FAIL] 0x8dab "DAB+ Test 2": DAB+, 96 kbit/s EEP 1-A, AAC-LC, 48 kHz Stereo, 29.5 s audio, DLS, 3 slides -- audio "AAC-LC, 48 kHz Stereo @ 96 kbit/s", expected "HE-AAC v2, 48 kHz Stereo @ 96 kbit/s"
```

The GUI tests print one `QINFO` line per step with what they verified, e.g.
`Playing "DAB+ Test 2": AAC-LC, 48 kHz Stereo @ 96 kbit/s, DLS "...", slide 0001.jpg`.
The warning `No such signal QPlatformNativeInterface::systemTrayWindowChanged`
comes from Qt without a display and can be ignored.

### Summary of all recordings

Running the test program directly tests all recordings in one go and prints
a summary at the end:

```sh
build-tests/src/tests/tst_recordings
```

```
Audio services by audio format:
  passed failed
      12      0   AAC-LC, 48 kHz Stereo
      33      0   HE-AAC v2, 48 kHz Stereo
      52      0   MPEG 1.0 Layer II, 48 kHz Stereo
      12      0   MPEG 2.0 Layer II, 24 kHz Mono
      ...
Audio services by protection:
      ...
Recordings by sample format:
      ...
```

The test programs are QtTest programs and accept its options, e.g. a single
recording: `tst_recordings decodeRecording:odr_dabmod`, or `-help`.

### Debug output

welle.io writes a lot of log messages while decoding. The tests hide them,
so that the results are readable. To see them, set `WELLE_TEST_VERBOSE`:

```sh
WELLE_TEST_VERBOSE=1 ctest --test-dir build-tests -R odr_dabmod -V
```

`welle-test-probe <clip>` prints everything welle.io decoded from a
recording as JSON, e.g. to investigate a failed test.

## How the recordings are tested

`tst_recordings` decodes a recording as fast as possible, with an audio
decoder for every service, and compares the result with the `expect` block
of the recording in the manifest. The expected values and their tolerances
are described in the README of the test data repository.

The decoders are attached from the receiver thread, so they start at the
same position of the signal on every run, and the results hardly depend on
the speed or load of the machine. Errors are only counted after a service
decoded its first audio.

The GUI tests use the `offscreen` Qt platform and their own empty settings,
so they don't need a display and don't touch the settings of an installed
welle.io. They play audio with volume 0.

## welle-cli tests

The `cli.*` tests (`cli/`) test welle-cli as a black box: they start the
built `welle-cli` binary (`$WELLE_CLI`, set by CTest) with its normal command
line options in a temporary directory and only look at its exit code, its
output, the files it writes and its web server. They don't link welle-cli
code and don't need any option that exists only for tests. The helper
`WelleCli` (`cli/welle_cli_process.h`) starts and stops the process, reads
its output and polls the web server.

welle-cli plays IQ files in real time and starts them again at the end, so
these tests take 1–30 s each; only `-t 3` runs at full speed.
`cli.recording.<clip>` waits until `mux.json` matches the manifest (at most
the length of the recording + 20 s) and shows the last state as
`[ OK ]`/`[FAIL]` lines. Audio is checked with `odr_dabmod` only
(`cli.webServer`, `cli.dumpAll`), because decoding and MP3 encoding of every
service of every recording in real time would need a lot of CPU.

In Debug builds welle-cli uses AddressSanitizer. The tests switch off its
leak report (`ASAN_OPTIONS=detect_leaks=0`) and fail if it reports a memory
error.

## UI tests

The `gui.ui.*` tests (`gui/ui/tst_ui_<area>.cpp`) click through the real user
interface (`MainView.qml`) while a recording plays as RAW file input, like a
user would: with mouse clicks and key presses on the window, not by calling
QML functions. What they expect from the recording (station names, ensemble,
DAB or DAB+, bitrate, DLS, slides) comes from the manifest.

Every test function also fails if QML printed a warning or an error.
Generic checks run at many window sizes:

* nothing sticks out of the window (`UiApp::itemsOutsideWindow()`),
* every visible control has an accessible name for screen readers
  (`UiApp::controlsWithoutAccessibleName()`),
* a click only counts if it reaches the item: a test can't click something
  that is covered by a drawer, a popup or is still moving in.

### How the tests find elements

The QML code sets a fixed, untranslated `objectName` on everything a test
looks at or clicks, e.g.

| objectName | Element |
|---|---|
| `navButton`, `backButton`, `navDrawer`, `navItem.<destination>`, `navBadge.<list>` | App bar and navigation drawer. The drawer chooses the station list (`UiTestCase::showList()`) |
| `playStopButton`, `volumeButton`, `volumeSlider` | Playback in the app bar |
| `stationListPane`, `stationListTitle`, `stationListCount`, `stationListView`, `stationRow`, `favoriteButton`, `scanButton` | Station list. A `stationRow` has the properties `sid`, `channel` and `label` |
| `nowPlaying.station`, `.ensemble`, `.dls`, `.dabType`, `.audioMode`, `.slide`, `.signal` | What plays |
| `miniPlayer`, `detailPane`, `homePage` (property `layoutMode`) | Responsive layout |
| `settingsPage`, `setting.<QSettings key>`, `settingsIndex.<section>` | Settings |
| `dashboard`, `card.<view>`, `cardMenuButton`, `addViewButton`, `addView.<view>` | Expert views |
| `snackbar`, `snackbar.text`, `snackbar.close` | Messages |

New UI elements that a test should use get an `objectName` in the same way.
Properties of a delegate must not be named like a role of its model (e.g.
`stationName`), that hides the role.

### Writing a test

`UiTestCase` (`gui/ui/ui_test_case.h`) is the base class, `UiApp`
(`gui/ui/ui_app.h`) starts the user interface and drives it:

```cpp
void UiExampleTests::initTestCase()
{
    UI_START_WITH_CLIP(app, "odr_dabmod");   // Skips without test data
}

void UiExampleTests::playsFirstStation()
{
    QVERIFY(scanStations());
    const QJsonObject service = app.services().first();
    QVERIFY(app.click(stationRow(service["sid"].toInt())));
    QTRY_COMPARE_WITH_TIMEOUT(app.text("nowPlaying.station"), service["label"].toString(), 10000);
    app.resize(360, 640);
    UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
    app.screenshot("example");
}
```

A new file `gui/ui/tst_ui_<area>.cpp` becomes the CTest test `gui.ui.<area>`
when CMake runs again. `UiApp::restart()` closes and starts the user interface
again with the same settings, `UiApp::writeSettings()` before `start()`
simulates an existing installation.

### Running, watching and screenshots

| Command | Result |
|---|---|
| `ctest --test-dir build-tests -L gui` | All GUI tests |
| `ctest --test-dir build-tests -R gui.ui.settings --output-on-failure` | One area |
| `build-tests/src/tests/tst_ui_playback playsEachService` | One test function |
| `QT_QPA_PLATFORM=xcb build-tests/src/tests/tst_ui_navigation` | Shows the window while the test clicks through it |
| `WELLE_TEST_SCREENSHOT_DIR=/tmp/shots ctest --test-dir build-tests -L gui` | Saves screenshots of the important states as PNG |
| `WELLE_TEST_LANGUAGE=de_DE.UTF-8 build-tests/src/tests/tst_ui_settings` | Runs the user interface in German (default: English) |
| `QT_QUICK_BACKEND=software ctest --test-dir build-tests -L gui` | Uses the software renderer of Qt Quick |

The screenshots are not compared with references, because fonts differ
between systems; they are for looking at the user interface, e.g. in a pull
request. GitHub Actions uploads them as the artifact `ui-screenshots`.

The tests render with OpenGL if the system has it (also without a display,
e.g. Mesa), so that they look like welle-io on a desktop with shadows.
Otherwise Qt Quick's software renderer is used, which can't draw shader
effects; welle-io then shows borders instead of shadows. Both have to pass.

## Smoke tests of the packages

The tests above check the build tree. The package workflows also check the
**installed** packages: they install the package the way a user would and
play the recording `odr_dabmod` (DAB and DAB+, DLS) with it. That finds
packaging errors such as a missing QML module, Qt plugin or codec library,
or broken `cmake --install` rules.

| Workflow | What is installed | Tested |
|---|---|---|
| `linux.yml`, job `qtbuild` | Flatpak bundle (`flatpak install --bundle`), x86_64 and arm64 | welle-io under Xvfb |
| `linux.yml`, job `install` | `cmake --install` on Debian 13 (Qt 6.8), build directory removed | welle-io under Xvfb, welle-cli, `.desktop`, AppStream, man pages |
| `macos.yml` | the published `.app` zip, unpacked | welle-io; welle-cli of the build |
| `windows.yml` | the InnoSetup installer, silent installation | welle-io |

The smoke test runs before the packages are uploaded, so a broken package
doesn't reach the nightly server. Its logs are uploaded as artifacts
`smoke-test-*`.

`welle-io --raw-file <file> --raw-format <format> --smoke-test <report.json>`
selects the RAW file as input device, compiles every QML file of the user
interface (parts like the expert views with QtCharts are only loaded when
opened, so a missing QML module would otherwise go unnoticed), scans the
recording, plays every audio service
until audio is decoded (and waits a little for its DLS), then writes a
JSON report and quits: exit code 0 if every service played, every QML file
compiled and QML reported no warnings, 1 otherwise, 2 if `MainView.qml` could not be loaded.
The settings are kept in a temporary directory, so an installed welle.io
keeps its own (on Linux also the data and cache folders). The report
contains the ensemble, every service with its audio mode and DLS, the
stations of the user interface's station list, the QML warnings, and the
Qt version, platform plugin and graphics API.

`smoke/smoke_test.py` (Python standard library only) runs the installed
programs and compares the report and welle-cli's `mux.json` and MP3 streams
with `manifest.json`:

```sh
# installed welle-io and welle-cli
src/tests/smoke/smoke_test.py --test-data ../welle.io-test-data \
    --welle-io /usr/bin/welle-io --welle-cli /usr/bin/welle-cli
# installed Flatpak
src/tests/smoke/smoke_test.py --test-data ../welle.io-test-data --flatpak io.welle.welle-gui
# only check a report, e.g. one copied from another device
src/tests/smoke/smoke_test.py --test-data ../welle.io-test-data --report welle-io-report.json
```

Without a display it sets `QT_QPA_PLATFORM=offscreen`. The workflows use
Xvfb instead, so that the platform plugin of the package is tested too.
The clip is unpacked if needed (needs `zstd`). Exit code 77 means that the
test data is missing.

## GitHub Actions

`.github/workflows/tests.yml` checks out the test data repository with the
secret `WELLE_TEST_DATA_TOKEN` (read access to that repository). The
repository variable `WELLE_TEST_DATA_REPO` can point to another location
than `AlbrechtL/welle.io-test-data`. Without the secret, e.g. for pull
requests from forks, the tests that need recordings are skipped. The package
workflows (`linux.yml`, `macos.yml`, `windows.yml`) use the same secret
for their smoke test and only check out the files of `odr_dabmod`
(sparse checkout); without it the smoke test is skipped with a warning.

---

*This file was generated with an AI assistant (Claude Code).*
