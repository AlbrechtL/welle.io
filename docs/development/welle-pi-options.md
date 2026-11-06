# welle-pi options in welle-pi.json

Most command line options of welle-pi can also be set in its settings file
(except `-b`, `-c`, and `-p`, which remain command-line only), so a daemon
started by systemd can be configured completely there. The file is the one
described in [welle-pi input sources](welle-pi-input.md#configuration-file):
`$XDG_CONFIG_HOME/welle-pi/welle-pi.json` or
`~/.config/welle-pi/welle-pi.json`, else `/etc/welle-pi/welle-pi.json`.

An option given on the command line overrides the file, and a key missing
from the file leaves the built-in default. A key of the wrong type (for
example `"gain": "auto"`) is reported and welle-pi exits with an error.
Keys of other features (`input`, `display`, `volume`, …) are not touched.

## Keys

| Key | Option | Meaning | Default |
|---|---|---|---|
| `"antenna"` | `-A` | SoapySDR antenna | none |
| `"disable_coarse_corrector"` | `-u` | Disable the coarse corrector | `false` |
| `"frontend"` | `-F` | Input driver and arguments, e.g. `"rtl_tcp,<HOST_IP>:<PORT>"` | `"auto"` |
| `"gain"` | `-g` | Input gain, `-1` for auto gain | `-1` |
| `"iq_file"` | `-f` | Read this IQ file instead of a receiver | none |
| `"logfile"` | `-l` | Append stdout and stderr to this file | none (`/dev/null` as a daemon) |
| `"pcm"` | `-o` | ALSA PCM device | `"default"` |
| `"pidfile"` | `-k` | Write the process id here and remove it on exit | none |
| `"soapysdr_args"` | `-s` | SoapySDR driver arguments | none |
| `"exit_command"` | — | Shell command to run when the menu quits welle-pi | none |

`-F` and `"frontend"` use the same form: `"<driver>"` or
`"<driver>,<arguments>"`. Without a comma the arguments from the file are
cleared, also when the command line sets the driver.

`-u` and `-b` accept an optional boolean value (`1`, `true`, `yes`, or `0`,
`false`, `no`), and default to `true` when given without an argument. This
allows overriding a `"disable_coarse_corrector": true` entry from the file
with `-u 0`.

## Example

```json
{
    "frontend": "rtl_tcp,192.168.1.15:1234",
    "pcm": "hw:2",
    "logfile": "/var/log/welle-pi.log",
    "pidfile": "/run/welle-pi.pid",
    "exit_command": "systemctl poweroff"
}
```

With this file the unit can run `welle-pi` without arguments. A command
line still wins, so `welle-pi -c 5C` tunes 5C while keeping everything
else from the file.

## Exit command

`"exit_command"` replaces the `ALLOW_SHUTDOWN` / `ExecStopPost` power-off
hook of earlier units: set it to `"systemctl poweroff"` (or `"shutdown -h
now"`) to power the host off when the user leaves welle-pi from the menu.
The command runs through `/bin/sh -c` with the rights of the service user,
so the unprivileged `User=welle-pi` variant needs root or a polkit rule
for `systemctl poweroff`.

It runs only after a clean menu exit. A signal (`systemctl stop`,
`SIGTERM`), a failed input or a crash does not run it, so stopping or
restarting the service never powers the host off. The same trust as for
plugins applies: the settings file is read as the service user and must not
be writable by others.
