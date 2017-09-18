---
layout: page
header:
  image_fullwidth: banner5.jpg
title: "rawfile"
teaser: ""
permalink: /devices/rawfile
---

The RAW files input is for developers how have recorded I/Q samples files. You can use this input to analyse your RAW files or to test welle.io. By default welle.io uses raw file in the u8 format (see below).

**Windows**
  ```
welle-io.exe -D rawfile -F yourfile
  ```

**Linux**
  ```
# welle-io -D rawfile -F yourfile
  ```

## File Format
welle.io supports different rawfiles formats. You can chnage the raw file format with the option "-B". Please read the next sections below for more details.

### u8 - 8 Bit unsigned
The I/Q samples have to be in 8-bit unsigned in the following format.
![rawfile_format_u8.png](/images/rawfile_format_u8.png)
* Size: 8-bit unsigned per I and Q sample
* Sample rate: 2048000 samples/s

**Example**
  ```
# welle-io -D rawfile -F yourfile -B u8
  ```

**Sources**
* rtl_tcp
* qt-dab (*.raw)

### s8 - 8 Bit signed
The I/Q samples have to be in 8-bit signed. For the format please see the format u8.
* Size: 8-bit signed per I and Q sample
* Sample rate: 2048000 samples/s

**Example**
  ```
# welle-io -D rawfile -F yourfile -B s8
  ```

**Sources**
* odr-dabmod

### s16le - 16 Bit signed little endian
The I/Q samples have to be in 16-bit signed little endian in the following format.
![rawfile_format_s16le.png](/images/rawfile_format_s16le.png)
* Size: 16-bit signed little endian per I and Q sample
* Sample rate: 2048000 samples/s

**Example**
  ```
# welle-io -D rawfile -F yourfile -B s16le
  ```

**Sources**
* qt-dab (*.sdr)

## Record an RAW file
There are several options to create a RAW file.

### rtl_sdr (Format u8)
You can use the command line tool "rtl_sdr" to record a file with a rtl_sdr device.
  ```
# rtl_sdr -f frequency -s 2048000 -n samplecount yourfile
e.g.
# rtl_sdr -f 174928 -s 2048000 -n 10000 yourfile.iq
  ```
To have a more convenient way you can use the script ["dab_raw_record.sh"](../download/dab_raw_record.sh).  
The following command records a 10 s file from the channel "5C" with the file name "CurrentDataAndTime_5C.iq"
  ```
# dab_raw_record -c 5C -t 10
  ```

### odr-dabmod (Format s8)
If you would like to test your [Opendigitalradio](http://www.opendigitalradio.org/) broadcasting set up without having broadcast hardware you can create an I/Q file.  
Use "odr-dabmod" with this [INI-file](../download/DabMod.ini) and the following command.
  ```
# odr-dabmod DabMod.ini
  ```
The input file has to be "DabMux.eti" and the output file is "DabMod.iq".
