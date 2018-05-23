---
layout: page
header:
  image_fullwidth: banner5.jpg
title: "rtl-sdr"
teaser: ""
permalink: /devices/rtl_sdr
---
The [rtl-sdr](http://osmocom.org/projects/sdr/wiki/rtl-sdr) device is a DVB-T dongle based on the Realtek RTL2832U.  
For more information about the rtl-sdr please visit [http://osmocom.org/projects/sdr/wiki/rtl-sdr](http://osmocom.org/projects/sdr/wiki/rtl-sdr).

## Table of contents
* [Supported Hardware](#supported-hardware)
* [General](#general)
* [Windows set up](#windows-set-up)
* [Linux set up](#linux-set-up)

## Supported Hardware
Basically all rtl-sdrs with the Realtek chipset RTL2832U are supported. These are available under many different names at Amazon or eBay.

## General
If you start welle.io without any options welle.io will detect the rtl-sdr device automatically. After enabling the "Expert mode" you can check if welle.io detects the device successfully.
![welle-io-rtlsdr.png](/images/welle-io-rtlsdr.png)

You can also use the command line option "-d" to force welle.io to use the rtl-sdr device.

**Windows**
  ```
welle-io.exe -d rtl_sdr
  ```

**Linux**
  ```
# welle-io -d rtl_sdr
  ```


## Windows set up
To install the rtl-sdr on your Windows 10 system please follow the steps:
* Navigate to your welle.io install folder e.g. C:\Program Files (x86)\welle.io
* Run "zadig_2.3.exe"
* Choose "Options" and click to "List All Devices"
![Zadig_List_All.png](/images/Zadig_List_All.png)

* Select your rtl-sdr e.g. "RTL2838UHIDIR"
![Zadig_Select_SDR.png](/images/Zadig_Select_SDR.png)

* Make sure that "WinUSB (...)" is selected and click to "Reinstall Driver" or "Replace Driver"
![Zadig_Install_Driver.png](/images/Zadig_Install_Driver.png)

* Now you can check if you device is listed in the device manager
![Zadig_Device_Manager.png](/images/Zadig_Device_Manager.png)

* Run welle.io

## Linux set up
Just install the library librtlsdr.  
For Ubuntu 16.04 LTS you can use the following command
  ```
# sudo apt install librtlsdr0 
  ```

