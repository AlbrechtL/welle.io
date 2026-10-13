#!/bin/bash

#
#    Copyright (C) 2016
#    Albrecht Lohofener <albrechtloh@gmx.de>
#
#    dab_raw_record is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    dab_raw_record is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with dab_raw_record; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

# Help function
ShowHelp()
{
  echo "DAB raw record - Records I/Q RAW SAMPLES for a specific DAB channel"
  echo "Copyright 2016 Albrecht Lohofener <albrechtloh@gmx.de>"
  echo
  echo "Usage:  [-c Sets the DAB channel e.g. 5C]"
  echo "        [-t Sets the recording time in seconds]"
  echo "        [-f Sets the file name (default: YMD_HMS_channel.iq)]"
  echo "        [-h This help]"
  echo
  echo "Example: dab_raw_record -c 5C -t 10"

  # Exit the script
  exit
}

# Converts the channel into a frequency
Channel2Freq()
{
  case "$1" in
    5A) FREQUENCY=174928;;
    5B) FREQUENCY=176640;;
    5C) FREQUENCY=178352;;
    5D) FREQUENCY=180064;;
    6A) FREQUENCY=181936;;
    6B) FREQUENCY=183648;;
    6C) FREQUENCY=185360;;
    6D) FREQUENCY=187072;;
    7A) FREQUENCY=188928;;
    7B) FREQUENCY=190640;;
    7C) FREQUENCY=192352;;
    7D) FREQUENCY=194064;;
    8A) FREQUENCY=195936;;
    8B) FREQUENCY=197648;;
    8C) FREQUENCY=199360;;
    8D) FREQUENCY=201072;;
    9A) FREQUENCY=202928;;
    9B) FREQUENCY=204640;;
    9C) FREQUENCY=206352;;
    9D) FREQUENCY=208064;;
    10A) FREQUENCY=209936;;
    10B) FREQUENCY=211648;;
    10C) FREQUENCY=213360;;
    10D) FREQUENCY=215072;;
    11A) FREQUENCY=216928;;
    11B) FREQUENCY=218640;;
    11C) FREQUENCY=220352;;
    11D) FREQUENCY=222064;;
    12A) FREQUENCY=223936;;
    12B) FREQUENCY=225648;;
    12C) FREQUENCY=227360;;
    12D) FREQUENCY=229072;;
    13A) FREQUENCY=230748;;
    13B) FREQUENCY=232496;;
    13C) FREQUENCY=234208;;
    13D) FREQUENCY=235776;;
    13E) FREQUENCY=237488;;
    13F) FREQUENCY=239200;;
    LA) FREQUENCY=1452960;;
    LB) FREQUENCY=1454672;;
    LC) FREQUENCY=1456384;;
    LD) FREQUENCY=1458096;;
    LE) FREQUENCY=1459808;;
    LF) FREQUENCY=1461520;;
    LG) FREQUENCY=1463232;;
    LH) FREQUENCY=1464944;;
    LI) FREQUENCY=1466656;;
    LJ) FREQUENCY=1468368;;
    LK) FREQUENCY=1470080;;
    LL) FREQUENCY=1471792;;
    LM) FREQUENCY=1473504;;
    LN) FREQUENCY=1475216;;
    LO) FREQUENCY=1476928;;
    LP) FREQUENCY=1478640;;
    *)  echo "Unkown channel"
  esac

  # kHz in Hz
  FREQUENCY=$(($FREQUENCY*1000))
}


# Handle parameters
while getopts 'c:t:f:h' OPTION ; do
  case "$OPTION" in
    c)   CHANNEL=$OPTARG;;
    t)   TIME=$OPTARG;;
    f)   FILENAME=$OPTARG;;
    h)   ShowHelp;;
    *)   ShowHelp
  esac
done

# Sets a filename if no filename is set
if [$FILENAME = ""]; then
  DATETIME=$(date "+%Y%m%d_%H%M%S")
  FILENAME="${DATETIME}_${CHANNEL}.iq"
fi

SAMPLERATE=2048000
SAMPLES=$(($SAMPLERATE*$TIME))

# Converts channel into a frequency
Channel2Freq $CHANNEL

COMMAND="rtl_sdr -f $FREQUENCY -s $SAMPLERATE -n $SAMPLES $FILENAME"
echo
echo "Run $COMMAND"

# Run rtl_sdr
eval $COMMAND

