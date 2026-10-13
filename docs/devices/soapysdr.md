# SoapySDR

The [SoapySDR](https://github.com/pothosware/SoapySDR/wiki) library is an
independent SDR support library that enables welle.io to use a variety of SDR
devices. The SoapySDR input has been tested with the
[LimeSDR](https://www.crowdsupply.com/lime-micro/limesdr), rtl-sdr, Ettus
USRP and HackRF boards.

SoapySDR adds support for any SDR that can tune to the DAB frequencies and
has a SoapySDR driver. It also supports all SDRs that are supported by
gr-osmosdr and Ettus UHD, via the
[SoapyOsmo](https://github.com/pothosware/SoapyOsmo/wiki) and
[SoapyUHD](https://github.com/pothosware/SoapyUHD/wiki) drivers.

Any supported SDR can also be used over the network via
[SoapyRemote](https://github.com/pothosware/SoapyRemote/wiki).

## Windows set up

There is no Windows support yet. It could be added, because SoapySDR
supports that platform.

## Linux set up

SoapySDR itself does not contain drivers for specific boards. To add support
for a board, install SoapySDR first and then the driver for each board
(e.g. LimeSuite for the LimeSDR, SoapyHackRF for the HackRF, SoapyAirspy for
the Airspy and so on):

1. SoapySDR itself from <https://github.com/pothosware/SoapySDR>
2. The driver for each board, e.g. LimeSuite for the LimeSDR from
   <https://github.com/myriadrf/LimeSuite>

To check that SoapySDR and the drivers are installed properly, check that the
following command finds the board:

```sh
SoapySDRUtil --make
```

Then build welle.io with SoapySDR support (`-DSOAPYSDR=ON` with CMake, see
[Building](../development/building.md#cmake)). welle.io finds the device
automatically, or you select *SoapySDR* in the settings.

## Configuration

Fill in the fields *Antenna*, *Clock source* and *Driver arguments* in the
SoapySDR settings. To find valid values, run e.g.
`SoapySDRUtil --probe="driver=hackrf"`:

```
$ SoapySDRUtil --probe="driver=hackrf"
(...)
----------------------------------------------------
-- Device identification
----------------------------------------------------
  driver=HackRF
  hardware=HackRF One
  clock source=internal
(...)
----------------------------------------------------
-- RX Channel 0
----------------------------------------------------
  Full-duplex: NO
  Supports AGC: NO
  Stream formats: CS8, CS16, CF32, CF64
  Native format: CS8 [full-scale=128]
  Stream args:
     * Buffer Count - Number of buffers per read.
       [key=buffers, units=buffers, default=15, type=int]
  Antennas: TX/RX
(...)
```

Enter the corresponding values in the settings. Please enter the **full**
driver argument string, e.g. `driver=hackrf`.

Notes for some devices:

* **LimeSDR**: connect the antenna to the `RX1_W` port and set the antenna
  option to `LNAW`. `SoapySDRUtil --probe=driver=lime` shows other possible
  options.
* **USRP**: set the driver arguments to `driver=uhd`. Set the antenna and the
  clock source too. `SoapySDRUtil --probe=driver=uhd` lists the possible
  values.

## Known limitations

* The automatic gain mode behaves erratically.
* Windows support is missing.
