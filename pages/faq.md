---
layout: page
header:
  image_fullwidth: banner5.jpg
title: "FAQ"
teaser: ""
permalink: /faq
---

## Table of contents
* [What do the three small rectangles in the right above corner mean?](#what-do-the-three-small-rectangles-in-the-right-above-corner-mean)
* [What are the minimum hardware requirements?](#what-are-the-minimum-hardware-requirements)
* [Where are the settings stored?](#where-are-the-settings-stored)
* [Why is my RTL-SDR showed as RTL2838 and not as RTL2832U?](#why-is-my-rtl-sdr-showed-as-rtl2838-and-not-as-rtl2832u)


## What are the minimum hardware requirements?
The main hardware requirement is the CPU because the decoding needs a lot of CPU power.

**Recommened**
* CPU: 4 cores x86 CPU with 2 GHz (Intel i-series)
* Display: High resolution with touchscreen
* Graphic card with OpenGL 2.0 support

**Minimum**
* CPU
 * 4 core x86 CPU with 1.3 GHz (Intel Atom Z8300 or Intel Atom Z3736F)
 * 4 core ARM Cortex-A7 CPU with 900 MHz (Raspberry Pi 2)
* Display: 800 x 480 pixels
* Graphic card with OpenGL 2.0 support

Please report if you are able to run welle.io on other hardware platforms.

## Where are the settings stored?
**Windows**  
The settings are stored inside the Windows registry at the key `HKEY_CURRENT_USER\SOFTWARE\welle.io\welle.io`

**Linux**  
The settings are stored inside your local settings folder `~/.config/welle.io/welle.io.conf`

**Android**  
The settings are stored at the file `/data/data/io.welle.welle/files/.config/welle.io/welle.io.conf`

## How can I delete the settings?
**Windows**
* Run regedit.exe
* Delete the key `HKEY_CURRENT_USER\SOFTWARE\welle.io`

**Linux**
* Delete the file `~/.config/welle.io/welle.io.conf`

**Android**
* Just deinstall welle.io from your phone or tablet

## Why is my RTL-SDR showed as RTL2838 and not as RTL2832U?
Some USB dongles with the product ID (PID) 0x2838 show RTL2838 as chip but Realtek never had a chip with this name. All these devices have also the Realtek RTL2832U inside.
