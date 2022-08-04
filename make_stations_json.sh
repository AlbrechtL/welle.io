#!/bin/bash
cd ~/.config/welle.io
grep stationListSerialize welle.io.conf | sed 's/stationListSerialize=\"// ; s/]\"/]/ ; s/\\//g'  > stations.json
