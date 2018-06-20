#!/usr/bin/env python3
#
# A munin plugin that uses the welle-io mux.json to make graphs.
# Expects the munin webserver on localhost port 7979
#
#
#    Copyright (C) 2018
#    Matthias P. Braendli (matthias.braendli@mpb.li)
#
#    This file is part of the welle.io.
#    Many of the ideas as implemented in welle.io are derived from
#    other work, made available through the GNU general Public License.
#    All copyrights of the original authors are recognized.
#
#    welle.io is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    welle.io is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with welle.io; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


import sys
import urllib.request
import json
from math import log10

conf_common = """
multigraph freqcorr
graph_title Frequency Correction
graph_args --base 1000
graph_vlabel Frequency correction in Hz
graph_category welleio
graph_info This graph shows the receiver frequency correction
freq_corr.label Freq corr

multigraph snr
graph_title Signal-to-Noise Ratio
graph_args --base 1000
graph_vlabel SNR 
graph_category welleio
graph_info This graph shows the receiver SNR
snr.label SNR
"""

conf_audio_level_template = """
multigraph audio_level_{sid}
graph_title {sid} {label} audio level
graph_order left right
graph_args --base 1000
graph_vlabel audio level
graph_category welleio
graph_info This graph shows audio levels for {label}

left.info Audio level for {label}
right.min -90
right.max 0
right.label right
right.type GAUGE
left.label left
left.min -90
left.max 0
left.type GAUGE
"""

url = 'http://localhost:7979/mux.json'
conn = urllib.request.urlopen(url)
bdata = conn.read()
muxdata = json.loads(bdata.decode())

if len(sys.argv) > 1 and sys.argv[1] == "config":
    print(conf_common)
    if "services" in muxdata:
        for service in muxdata["services"]:
            print(conf_audio_level_template.format(**service))
    sys.exit(0)

def lin_to_db(level):
    if level > 0:
        return int(20 * log10(level / 0x7FFF))
    else:
        return -90

if "frequencycorrection" in muxdata:
    print("multigraph freqcorr")
    print("freq_corr.value {}".format(muxdata["frequencycorrection"]))
if "snr" in muxdata:
    print("multigraph snr")
    print("snr.value {}".format(muxdata["snr"]))
if "services" in muxdata:
    for service in muxdata["services"]:
        values = {'sid': service['sid'],
                'left': lin_to_db(service['audiolevel']['left']),
                'right': lin_to_db(service['audiolevel']['right'])}
        print("multigraph audio_level_{sid}\nleft.value {left}\nright.value {right}".format(**values))

