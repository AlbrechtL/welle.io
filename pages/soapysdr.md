---
layout: page
header:
  image_fullwidth: banner5.jpg
title: "SoapySDR"
teaser: ""
permalink: /devices/soapysdr
---

The [SoapySDR](https://github.com/pothosware/SoapySDR/wiki) library is an
independent SDR support library that enables welle.io to use a variety of SDR
devices. Currently, the SoapySDR input has been tested with the [LimeSDR](https://www.crowdsupply.com/lime-micro/limesdr) and the HackRF boards.

## Windows set up

There is no windows support yet, but could be added since SoapySDR supports that platform.


## Linux set up

SoapySDR itself does not contain drivers for specific boards.
To add support for a board, one needs to install SoapySDR first, and then install additional dependencies for each desired board (e.g. LimeSuite for the LimeSDR, SoapyHackRF for the HackRF, SoapyAirspy for the AirSpy and so on).

1. SoapySDR itself from https://github.com/pothosware/SoapySDR
2. The plugin for each desired board, e.g. LimeSuite for the LimeSDR from https://github.com/myriadrf/LimeSuite

To verify that both SoapySDR and the desired bindings are properly installed, check that the following command finds the board:

`SoapySDRUtil --make`

For the LimeSDR connect your receiving antenna to the `RX1_W` port.

Then compile welle.io with soapysdr support enabled in the project file `welle.io.pro` or `-DSOAPYSDR` when building with CMake. Run welle.io with the `-D soapysdr` option.

## Known limitations

   * Automatic gain mode behaves erratically.
   * Windows support is missing.
   * No option to change the antenna selection, making this implementation specific to the LimeSDR, but currently it "happens to work by accident" with other boards (see [comment in PR #144](https://github.com/AlbrechtL/welle.io/pull/144#issuecomment-334999163))
