# welle.io tests

Automated tests for the DAB/DAB+ decoder library and the `welle-io` GUI.
They are built with CMake and run with CTest.

## Overview

| CTest test | Label | What it checks | Needs recordings |
|---|---|---|---|
| `unit.rawfile` | `unit` | Reading IQ files in the formats u8, s8, s16le, s16be and cf32 | no |
| `unit.dabconstants` | `unit` | DAB mode parameters, label character sets, subchannel bitrates | no |
| `unit.reed_solomon` | `unit` | Reed-Solomon codec used for DAB+ | no |
| `backend.recording.<clip>` | `backend` | Decodes one recording and compares everything with its expectations, see below | yes |
| `gui.radiocontroller` | `gui` | The C++ part of the GUI: opens a recording, scans, plays DAB+ and DAB, DLS and slide | yes |
| `gui.qml_smoke` | `gui` | Starts the complete QML user interface, checks the station list and that QML reports no warnings | yes |

There is one `backend.recording.<clip>` test per recording, e.g.
`backend.recording.odr_dabmod` or `backend.recording.uk_11d_2020`. Without
recordings there is a single `backend.recordings` test, which is skipped.
The GUI tests only exist if welle-io itself is built (`BUILD_WELLE_IO`, on by
default).

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

## GitHub Actions

`.github/workflows/tests.yml` checks out the test data repository with the
secret `WELLE_TEST_DATA_TOKEN` (read access to that repository). The
repository variable `WELLE_TEST_DATA_REPO` can point to another location
than `AlbrechtL/welle.io-test-data`. Without the secret, e.g. for pull
requests from forks, the tests that need recordings are skipped.

---

*This file was generated with an AI assistant (Claude Code).*
