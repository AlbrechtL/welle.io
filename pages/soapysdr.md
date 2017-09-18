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
devices. Currently, the SoapySDR input has been tested with the [LimeSDR](https://www.crowdsupply.com/lime-micro/limesdr) board.

## Windows set up

There is no windows support yet, but could be added since SoapySDR supports that platform.


## Linux set up

SoapySDR itself does not contain drivers for specific boards.
To support the LimeSDR, one needs to install SoapySDR first, and then install the LimeSuite.

1. SoapySDR itself from https://github.com/pothosware/SoapySDR
1. The LimeSuite for the LimeSDR from https://github.com/myriadrf/LimeSuite

To verify that both SoapySDR and the Lime bindings are properly installed, check that the following command finds the LimeSDR board:

`SoapySDRUtil --make`

Connect your receiving antenna to the `RX1_W` port.

Then compile welle.io with soapysdr support enabled in the project file `welle.io.pro`. Run welle.io with the `-D soapysdr` option.

## Known limitations

   * Automatic gain mode behaves erratically.
   * Windows support is missing.
   * No option to change the antenna selection, making this implementation specific to the LimeSDR.
