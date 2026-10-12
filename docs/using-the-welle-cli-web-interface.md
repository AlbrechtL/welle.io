# Using the welle-cli web interface

welle-cli (see README for compilation instructions) offers a web interface to the DAB receiver. This wiki page explains how to set it up, and what endpoints it offers.

It can be used with the internal webpage, or integrated into another system which accesses data URLs directly.

## Setup

1. Compile welle-cli according to README instructions;
1. Make sure `index.html` and `index.js` are in the same folder as the `welle-cli` binary, and run `welle-cli` from that folder so that it finds the two files;
1. Decide how many programmes you want to decode simultaneously. This depends on your system capacity: a modern PC can easily decode all programmes simultaneously (option `-D`), but a single-board ARM computer like the Raspberry Pi might only be able to decode a couple (option `-C 2` for two programmes simultaneously). Find an appropriate number for your system by trial and error.
1. If you use `-C`, decide if you want to stay 10 seconds on every programme (default, use this for ensembles that don't use slideshow because 10s is a bit short for receiving a slide), or if you want to switch once DLS and a slide were decoded, staying at most 80 seconds on a given programme. (add `-P`)
1. Run welle-cli with the webserver on port 7979, receiving channel 10A: `welle-cli -c 10A -w 7979 -C 2`
1. Open http://localhost:7979 in your browser.

From top to botton, the page will show:
* Ensemble and receiver information
* A table with all services in the ensemble. The 2 programmes being decoded will have the audiolevel indication updated, and DLS will be decoded.
* All decoded slideshow images
* Spectrum (NULL in green, data symbols in red)
* Channel Impulse Response (green numbers indicate location of calculated peaks)
* Constellation (vertical is phase from 0 to 360 degrees, horizontal is carrier index `k`)
* A list of detected TII patterns

## Data URLs available
The webpage your browser shows retrieves data from `welle-cli` in regular intervals through several HTTP URLs:

* `/mux.json` contains most information about the data in the received ensemble in json format.
* `/channel` returns the currently tuned channel as `text/plain`. It accepts a POST request to switch channels.
* `/spectrum` returns a sequence of float32 values that corresponds to the magnitude of the FFT, for displaying the spectrum.
* `/nullspectrum` is the same for the spectrum of the NULL symbol, where TII is visible.
* `/impulseresponse` contains a sequence of float32 values used to plot the channel impulse response.
* `/constellation` contains a sequence of float32 values representing the phase of the constellation points, in degrees.
* `/fic` will return an endless stream of FIC data, including CRC. FIBs with invalid CRC will also be included. This isn't used by the web page, but can be used in combination with [ETISnoop](https://github.com/Opendigitalradio/etisnoop)
* `/mp3/<SID>` will send an endless stream of mp3 audio for the given SId, which can be either `0xABCD` or in decimal.
