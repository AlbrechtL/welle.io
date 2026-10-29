# dg8ygz receiver fixes

This branch collects our receiver changes on top of upstream welle.io 2.7
(commit `512558d1f8ac4c524d3c63e97510ea36c1bd7a70`). It combines changes that
were previously deployed in different receiver builds. It does not contain
site configuration, credentials, installer scripts or driver binaries.

## What changes

- OFDM frame handoff uses a condition predicate and explicit ownership. The
  producer cannot overwrite an unprocessed frame, and stopping wakes waiters.
- CLI connections and audio queues have bounded resource use and timeouts.
  Shutdown joins workers; malformed HTTP, invalid service IDs and oversized
  MOT segments are checked. Fixed-channel pages retain correct Play/SLS URLs.
- Software AGC follows the device gain range and step in both directions.
  It evaluates 250 ms IQ windows, rejects isolated peaks, waits after a gain
  change and holds gain for zero/invalid input. A narrowly identified legacy
  Miri interface uses its different full-scale convention. This is receiver
  software AGC, not a modification of a hardware driver.
- RTL-TCP keeps queued IQ bytes until the decoder has room. Transfer pacing
  follows reservoir occupancy within +/-1% of the nominal byte rate. It does
  not resample, invent or intentionally discard samples. A finite buffer can
  still overflow during a sufficiently long stall; counters expose this.
- Input diagnostics distinguish accepted/dropped bytes and driver errors.
  The web interface measures input using byte counters and monotonic time,
  rather than displaying the requested sample rate as a measurement.
- The collapsible graph keeps at most 15 minutes of browser history, displays
  a 2048 ksample/s reference, and shows historical arithmetic rolling means.
  Its initial vertical bounds are 2025–2075 ksample/s for 60 seconds. Later
  scaling follows the data. Missing/stale input creates gaps.
- RTL-TCP uses two bytes per IQ pair; Soapy CF32 uses eight. The displayed
  CF32 bit rate is internal sample representation, not USB bus throughput.

The source-clock pacing fix is for RTL-TCP. It cannot repair samples already
lost in USB, a driver or an upstream process. Neither a good SNR nor matching
OFDM submitted/processed counters proves that the entire upstream path is
lossless. The combined branch still needs hardware-specific soak testing.

## Build

Use upstream's build prerequisites and CMake workflow. For a CLI-only build:

```sh
cmake -S . -B build -DBUILD_WELLE_IO=OFF -DBUILD_WELLE_CLI=ON -DSOAPYSDR=ON -DRTLSDR=ON
cmake --build build --target welle-cli --parallel 2
```

This branch leaves upstream build defaults unchanged. Enable other input
backends only with their development libraries installed. Do not replace a
running receiver without preserving its binary, libraries and configuration.

## Regression tests

Tests use synthetic data and loopback sockets; they do not open an SDR.
The receiver tests require Python 3, g++ and Node.js:

```sh
python3 tests/receiver/run.py
python3 tests/stability/run.py
python3 tests/stability/http-server.py build/welle-cli
python3 tests/stability/channel-http.py build/welle-cli
python3 tests/stability/decoder-stop.py build
python3 tests/stability/slide-http.py build
python3 tests/receiver/test-tcp-clock-fast.py build/welle-cli
python3 tests/receiver/test-tcp-clock-slow.py build/welle-cli
```

The stability suite additionally needs libmp3lame development headers/libraries.
It uses ASan/UBSan by default. Link-object tests expect CMake's Makefile generator.
Set the normal library search environment if dependencies are in a private prefix.

For the real browser test, install Playwright and its Chromium browser in your
test environment, then run:

```sh
node tests/receiver/test-browser.cjs "$PWD/build/welle-cli"
```

Its screenshots and receiver log are stored in a temporary directory printed by
that test. The browser test checks real TCP payload accounting, graph controls,
mean display, stale input, recovery, and graceful receiver shutdown.

On 2026-10-29 the combined branch was built as a native x86-64 CLI. Validation
results are recorded in the branch's test-validation document. Qt GUI builds,
ARM builds and live RF acceptance are separate checks; passing synthetic tests
is not a promise of uninterrupted reception.
