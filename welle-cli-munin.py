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

# How many CIR peaks to add to the graph. This depends
# on the number of SFN transmitters you receive in your
# location.
NUM_CIR_PEAKS=2

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

multigraph fic
graph_title FIC CRC error counter
graph_args --base 1000
graph_vlabel FIC CRC error
graph_category welleio
graph_info This graph shows the FIC CRC errors
crcerr.label FIC CRC Err
crcerr.type DERIVE

multigraph cir
graph_title CIR peaks position
graph_args --base 1000
graph_vlabel CIR peaks
graph_category welleio
graph_info This graph shows the position of the peaks in the channel impulse response
graph_order """

for n in range(NUM_CIR_PEAKS):
    conf_common += "component{} ".format(n)

conf_common += "\n"

for n in range(NUM_CIR_PEAKS):
    conf_common += "component{}.min 0\n".format(n)
    conf_common += "component{}.max 2048\n".format(n)
    conf_common += "component{}.label CIR component {}\n".format(n, n)
    conf_common += "component{}.type GAUGE\n".format(n)

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

conf_errors_template = """
multigraph errors_{sid}
graph_title {sid} {label} error counters
graph_order frame rs aac
graph_args --base 1000
graph_vlabel frame, rs and aac error counters
graph_category welleio
graph_info This graph shows Frame, Reed-Solomon and AAC errors for {label}

frame.label Frame err
frame.type DERIVE
rs.label RS errors
rs.type DERIVE
aac.label AAC errors
aac.type DERIVE
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
            print(conf_errors_template.format(**service))
    sys.exit(0)

def lin_to_db(level):
    if level > 0:
        return int(20 * log10(level / 0x7FFF))
    else:
        return -90

try:
    print("multigraph freqcorr")
    print("freq_corr.value {}".format(muxdata["frequencycorrection"]))
except:
    print("freq_corr.value U")

try:
    print("multigraph snr")
    print("snr.value {}".format(muxdata["snr"]))
except:
    print("snr.value U")

try:
    print("multigraph fic")
    print("crcerr.value {}".format(muxdata["ensemble"]["fic"]["numcrcerrors"]))
except:
    print("crcerr.value U")

if "cir" in muxdata:
    print("multigraph cir")
    for n in range(NUM_CIR_PEAKS):
        try:
            print("component{}.value {}".format(n, muxdata["cir"][n]["index"]))
        except:
            print("component{}.value U".format(n))

service_values_template = """
multigraph audio_level_{sid}
left.value {left}
right.value {right}
multigraph errors_{sid}
frame.value {frame}
rs.value {rs}
aac.value {aac}"""

if "services" in muxdata:
    for service in muxdata["services"]:
        values = {'sid': service['sid'],
                'left': lin_to_db(service['audiolevel']['left']),
                'right': lin_to_db(service['audiolevel']['right']),
                'frame': service['errorcounters']['frameerrors'],
                'rs': service['errorcounters']['rserrors'],
                'aac': service['errorcounters']['aacerrors']}
        print(service_values_template.format(**values))

