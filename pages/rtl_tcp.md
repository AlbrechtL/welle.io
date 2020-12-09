---
layout: page
header:
  image_fullwidth: banner5.jpg
title: "rtl_tcp"
teaser: ""
permalink: /devices/rtl_tcp
---

The rtl_tcp input is for advanced users how runs the tool "[rtl_tcp](http://osmocom.org/projects/sdr/wiki/rtl-sdr#rtl_tcp)" on a server.

![rtl_tcp.png](/images/rtl_tcp.png)

Please note that the sample rate is 2048000 samples/s. Each sample consists of a 8-bit I and a 8-bit Q part. So the data rate would be 32.768e6 bit/s = 4.096 MB/s. Please ensure that your network are able to transport this data rate stable.

**Setup**

To set up the rtl-tcp input navigate to the settings, disable `Auto detect` and enable `rtl-tcp` (see picture below). Afterwards you can set the IP address or set the hostname.

![rtl_tcp_input.png](/images/rtl_tcp_input.png)

**Hint**

If you need to start rtl_tcp as a deamon you can use this init.d script "[rtl_tcp_server](../download/rtl_tcp-server)". This script starts rtl_tcp with listen on all Ethernet interfaces on port 1235.
