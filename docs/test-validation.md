# Combined branch validation — 2026-10-29

Source under test: `ec9366e0` (the subsequent changes add documentation and
portable tests, without changing receiver source). Native x86-64 release CLI,
SoapySDR/RTL-SDR/Airspy/FLAC enabled; single compiler job. No running receiver
was replaced. The isolated test build used private dependency libraries.

Passed:

- OFDM handoff: nine scenarios including 1000 ordered frames, delayed/spurious
  notification, stop/unblock, startup race and reset generations.
- Software AGC: range steps, boundaries, float rounding, full-scale conventions,
  hysteresis, isolated peaks, clipping, zero and invalid IQ windows.
- Byte-exact 3 MiB IQ transfer with a slow consumer/full destination, recovery,
  IQ-pair alignment and write counters; concurrent Soapy counter snapshots.
- Pacing model: simulated 24-hour source offsets through +/-5000 ppm, bounded
  correction and scheduler jitter. These are simulations, not 24-hour RF runs.
- ASan/UBSan stability tests: HTTP/SID parsing, bounded audio queues, idle and
  non-reading clients, MOT bounds, concurrent ring-buffer producer/consumer.
- Actual CLI: 240 aborted requests and 80 connection attempts; resources returned
  to six threads/five descriptors. Graceful shutdown with partial HTTP and silent
  FIC took 5.039 seconds. All 300 decoder start/stop cycles completed.
- Actual HTTP fixed/selectable channel pages and SLS router, including query
  parameters, exact PNG bytes and invalid/absent service IDs.
- JavaScript: measured RTL-TCP/CF32 rates, reset/reconnect, invalid/stale input,
  bounded graph history, rolling arithmetic means and initial 2025–2075 scale.
- Chromium against the actual CLI and a loopback RTL-TCP source: split protocol
  header excluded from payload counts, fold/unfold via mouse/keyboard, continuous
  collection while folded, numeric mean, responsive graph, IQ stop/recovery,
  stale HTTP status, zero browser exceptions and clean SIGTERM.
- Actual RTL-TCP client with synthetic +5000 ppm and -5000 ppm input, 40 seconds
  each: both network and decoder drop counters stayed zero. Final network fills
  were 4,456,448 and 4,063,232 bytes, respectively. Graceful shutdown passed.

The common receiver source matches the final WDR source snapshot except for
Soapy integration and the input-format-specific web tooltip. The branch combines
previously separate host builds. ARM, Qt GUI and live RF soak tests of this exact
combined branch remain outstanding. Synthetic tests do not establish RF quality,
USB reliability or uninterrupted reception under arbitrary system load.
