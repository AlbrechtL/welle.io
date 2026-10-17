# Supported hardware

welle.io supports the following SDR devices and inputs:

* [rtl-sdr](rtl_sdr.md): DVB-T USB sticks with the Realtek RTL2832U
* [Airspy](airspy.md): Airspy R2 and Airspy Mini
* [rtl_tcp](rtl_tcp.md): an rtl-sdr on another computer in the network
* [SoapySDR](soapysdr.md): all SDR devices that are supported by SoapySDR,
  gr-osmosdr and UHD (LimeSDR, HackRF, Ettus USRP and many more)
* [Raw files](rawfile.md): recorded I/Q samples
* [Linux SDR (V4L2)](v4l2.md): receivers with a Linux kernel driver
  (`/dev/swradioN`), e.g. rtl-sdr, Airspy, HackRF

The SDR device must be able to tune to the DAB frequencies (Band III,
174–240 MHz) with a bandwidth of at least 2 MHz.

If you start welle.io without any options, it tries Airspy, rtl-sdr,
SoapySDR and Linux SDR devices in turn and uses the first one it can open.
rtl_tcp and raw files are selected in the settings (disable *Auto
detect*).

The settings list only the devices that welle.io was built with. Airspy and
rtl-sdr are included by default, and Linux SDR on Linux. SoapySDR has to be
enabled when building (CMake options `AIRSPY`, `RTLSDR`, `SOAPYSDR` and
`V4L2_SDR`, see [Building](../development/building.md)).
