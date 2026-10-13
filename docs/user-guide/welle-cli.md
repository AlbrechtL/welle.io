# welle-cli

If you build welle.io with [CMake](../development/building.md#cmake), you
also get an executable called **welle-cli**, the welle.io **c**ommand
**l**ine **i**nterface. It doesn't need Qt.

## Usage

Receive with an rtl-sdr and play with ALSA:

```sh
welle-cli -c <channel> -p <programme>
```

Read an I/Q file and play with ALSA (the file format is u8, unless the file
name ends with `<format>.iq`, see [Raw files](../devices/rawfile.md)):

```sh
welle-cli -f <file> -p <programme>
```

Use `-D` to dump the FIC and all programmes to files:

```sh
welle-cli -c <channel> -D
```

Use `-w` to enable the web server and decode a programme on demand:

```sh
welle-cli -c <channel> -w <port>
```

Use `-Dw` to enable the web server and decode all programmes:

```sh
welle-cli -c <channel> -Dw <port>
```

Use `-C 1 -w` to enable the web server and decode the programmes one by one
in a carousel, `-C N -w` to decode them N by N. This is useful if your
computer cannot decode all programmes at the same time, but you still want an
overview of the ensemble. By default, welle-cli switches every 10 seconds.
With the `-P` option, welle-cli switches once DLS and a slide were decoded,
and stays at most 80 seconds on a programme.

```sh
welle-cli -c <channel> -C 1 -w <port>
welle-cli -c <channel> -PC 1 -w <port>
```

Example: `welle-cli -c 12A -C 1 -w 7979` enables the web server on channel
12A. Open <http://localhost:7979/> to see the details of every service of the
ensemble and the slideshows, play the audio (click the play button) and check
the spectrum, constellation, TII information and the channel impulse
response. See [Web interface](welle-cli-web-interface.md).

### Streaming output options

In web server mode, `welle-cli` streams MP3 by default. With the `-O` option
you can choose between `mp3` and `flac` (lossless), if FLAC support was
enabled at build time.

### Backend options

`-u` disables the coarse frequency corrector, for receivers with a low
frequency offset.

Use `-t <test number>` to run a test. To understand what the tests do,
please see the source code.

### Driver options

By default, `welle-cli` tries all enabled drivers in turn and uses the first
device it can open.

Use `-F <driver>[,<driver args>]` to select a specific driver and pass
arguments to it. This selects the `rtl_tcp` driver (which is not detected
automatically) and passes the host name or IP address and the port of the
rtl_tcp server:

```sh
welle-cli -c 10B -p GRRIF -F rtl_tcp,192.168.12.34:1234
welle-cli -c 10B -p GRRIF -F rtl_tcp,my.rtl-tcp.local:9876
```

At the moment `rtl_tcp` is the only driver that accepts options.

### Examples

```sh
welle-cli -c 10B -p GRRIF
welle-cli -f ./ofdm.iq -p GRRIF
welle-cli -f ./ofdm.iq -t 1
```

## Running welle-cli as a web server service

This section explains how to run welle-cli as a background service that
starts automatically at boot and restarts if it crashes or hangs.

### 1. Build and install welle-cli (without Qt)

```sh
sudo apt install git build-essential libfaad-dev libmpg123-dev libfftw3-dev \
    librtlsdr-dev libusb-1.0-0-dev libmp3lame-dev libflac++-dev xxd

git clone https://github.com/AlbrechtL/welle.io.git
cd welle.io
mkdir build && cd build
cmake .. -DBUILD_WELLE_IO=OFF -DRTLSDR=ON -DFLAC=ON
make -j$(nproc)
sudo make install
```

### 2. Install the systemd service

```sh
sudo cp scripts/welle-cli.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable welle-cli
sudo systemctl start welle-cli
```

The default service file starts welle-cli on channel 9A in carousel mode with
the web server on port 7777:

```ini
ExecStart=/usr/local/bin/welle-cli -c 9A -PC 1 -w 7777
```

Edit `/etc/systemd/system/welle-cli.service` to change the channel or the
port, then run `sudo systemctl restart welle-cli`.

The service is configured with `Restart=always` and `RestartSec=3`, so it
restarts after 3 seconds if it stops for any reason (including hangs of the
rtl-sdr driver).

### 3. Open the web interface

Open `http://<server-ip>:7777/` in your browser. The web interface lets you

- browse all services of the DAB ensemble,
- play audio streams,
- monitor SNR, spectrum, constellation and CIR,
- view TII transmitter information,
- change the DAB channel,
- restart the service remotely (↺ button in the footer).

### 4. (Optional) Add a transmitter site database

If you have an FMLIST-compatible CSV database (semicolon-separated, with the
columns `eid`, `tii`, `location`), you can add it to the web interface to
show the transmitter site names next to the TII identifiers:

```sh
python3 scripts/generate-tii-db.py /path/to/dab-tx-list.csv
make -j$(nproc) -C build welle-cli
sudo make -C build install
sudo systemctl restart welle-cli
```

!!! note
    Do not commit `src/welle-cli/index.js` after running this script if the
    CSV data is under copyright (e.g. FMLIST data requires prior written
    authorisation for public redistribution).

### Updating

```sh
cd welle.io
git checkout src/welle-cli/index.js   # reset local TII data if previously injected
git pull
# re-inject TII data if needed (see step 4 above)
make -j$(nproc) -C build welle-cli
sudo make -C build install
sudo systemctl restart welle-cli
```
