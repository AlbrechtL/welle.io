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
devices. Currently, the SoapySDR input has been tested with the
[LimeSDR](https://www.crowdsupply.com/lime-micro/limesdr), RTL-SDR, Ettus USRP
and the HackRF boards.

SoapySDR will add support for any SDR that allows tuning into the needed
frequencies and is supported by a SoapySDR Driver. In addition to that, it adds
support for all SDRs that are supported by gr-osmosdr and Ettus uhd via the
[SoapyOsmo](https://github.com/pothosware/SoapyOsmo/wiki) and
[SoapyUHD](https://github.com/pothosware/SoapyUHD/wiki) Drivers.

Any supported SDR can also be accessed over the Network via
[SoapyRemote](https://github.com/pothosware/SoapyRemote/wiki).

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

Then compile welle.io with soapysdr support enabled in the project file `welle.io.pro` or `-DSOAPYSDR=1` when building with CMake. Run welle.io with the `-d soapysdr` option.

## Known limitations

   * Automatic gain mode behaves erratically.
   * Windows support is missing.
