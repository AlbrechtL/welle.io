# Usage

welle.io is a graphical program: simply start it. It looks for an SDR
device automatically (see [Supported hardware](../devices/index.md)).

## Command line options

| Option | Description |
|---|---|
| `-h`, `--help` | Show the help. |
| `-v`, `--version` | Show the version. |
| `--dump-file <file>` | Records DAB frames (`*.mp2`) or DAB+ superframes with RS coding (`*.dab`). This file can be used to analyse X-PAD data with [XPADxpert](https://www.basicmaster.de/xpadxpert). |
| `--log-file <file>` | Redirects all log output to a file. |
| `--raw-file <file>` | Uses this raw file as input device, like the settings page does. See [Raw files](../devices/rawfile.md). |
| `--raw-format <format>` | Sample format of the raw file: `auto`, `u8`, `s8`, `s16le`, `s16be`, `cf32` (default: `auto`). |
| `--smoke-test <report>` | Plays every service of the raw file with temporary settings, writes the result to a JSON file and quits. Exit code 0 if every service played. |
| `--smoke-timeout <seconds>` | Time limit of the smoke test (default: 120). |

## Keyboard shortcuts

| Keys | Action |
|---|---|
| F1–F12, 1–9, 0, Ctrl+1–9, Ctrl+0 | Play station no. *x* of the station list: `1` for station 1, `0` for station 10, `Ctrl+1` for station 11 … |
| S, Media Play, Media Stop, Media Pause, Media Play/Pause | Start/stop playback |
| N, Media Next | Play the next station of the list |
| P, Media Previous | Play the previous station |
| M, Volume Mute | Mute/unmute |
| Ctrl+Up, Volume Up | Volume up |
| Ctrl+Down, Volume Down | Volume down |
