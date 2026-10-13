# Supported hardware

welle.io supports the following SDR devices and inputs:

* [rtl-sdr](rtl_sdr.md): DVB-T USB sticks with the Realtek RTL2832U
* [Airspy](airspy.md): Airspy R2 and Airspy Mini
* [rtl_tcp](rtl_tcp.md): an rtl-sdr on another computer in the network
* [SoapySDR](soapysdr.md): all SDR devices that are supported by SoapySDR,
  gr-osmosdr and UHD (LimeSDR, HackRF, Ettus USRP and many more)
* [Raw files](rawfile.md): recorded I/Q samples

The SDR device must be able to tune to the DAB frequencies (Band III,
174–240 MHz) with a bandwidth of at least 2 MHz.

If you start welle.io without any options, it tries Airspy, rtl-sdr and
SoapySDR devices in turn and uses the first one it can open. rtl_tcp and raw
files are selected in the settings (disable *Auto detect*).
