# CLI stability regressions

These tests open only loopback test connections and use synthetic I/Q.
They do not access an SDR or an Icecast installation.

- `python3 tests/stability/run.py`: actual HTTP parser, SID handling, audio
  queue, idle disconnect, non-reading client, MOT boundaries and concurrent
  ring buffer. Requires g++, libmp3lame-dev; ASan/UBSan enabled by default.
- `WELLE_SANITIZERS=undefined WELLE_FLAC_TEST=1 python3 tests/stability/run.py`:
  ARM test variant including actual FLAC output; requires libflac++-dev.
  The tested Pi kernel cannot reserve this GCC ASan runtime's address range.
- `python3 tests/stability/http-server.py /path/to/build/welle-cli`: actual
  server, 240 aborted requests, 80 connections, resource cleanup, graceful
  shutdown with silent FIC and unfinished headers.
- `python3 tests/stability/decoder-stop.py /path/to/build`: links the actual
  CLI build objects into a 300-cycle decoder start/stop test.

The tests assert behavior and terminate on failure. Use a normal CMake CLI
build for the last two tests. The decoder test reads CMake's Makefile-generator
link.txt, so it currently expects that generator. A test timeout is a failure.
These tests do not replace a live SDR/Icecast soak test.

Current bounds: 64 concurrent HTTP workers, 5 s total request parsing,
8192 bytes per line, 32768 header bytes, at most 99 header lines, 1 MiB POST
body; each audio client has at most 1 MiB queued, 5 s queue-age bound and
1 s send deadline. Only the offending audio client is disconnected.
Four-digit SIDs are canonical hex; longer all-digit SIDs are decimal.
Explicit 0x hex and 0d decimal prefixes are supported, with four-digit
canonical hex taking priority (e.g. 0d12 remains hexadecimal).

- `node tests/stability/player-service-id.js`: real rendered Play handler and slide URL retain explicit hexadecimal SIDs (including SUNSHINE LIVE).
- `python3 tests/stability/soapy-gain.py`: real Soapy gain-step methods against a fake device; monotonic gain reduction/increase and boundary checks with UBSan.

- `python3 tests/stability/slide-http.py /path/to/build`: actual SLS dispatcher and MOT cache with PNG fixture, cachebreak parameters, invalid/missing SID and byte-exact payload; uses null input, no SDR.
- `node tests/stability/channel-ui.js`: actual page startup with selectable channel versus fixed text; change action and refresh.
