/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <QDebug>

#include "CChannels.h"

#define NUMBEROFCHANNELS 54
QString CChannels::FirstChannel = "5A";

CChannels::CChannels()
{
    // Band III
    FrequencyMap["5A"] = 174928000;
    FrequencyMap["5B"] = 176640000;
    FrequencyMap["5C"] = 178352000;
    FrequencyMap["5D"] = 180064000;
    FrequencyMap["6A"] = 181936000;
    FrequencyMap["6B"] = 183648000;
    FrequencyMap["6C"] = 185360000;
    FrequencyMap["6D"] = 187072000;
    FrequencyMap["7A"] = 188928000;
    FrequencyMap["7B"] = 190640000;
    FrequencyMap["7C"] = 192352000;
    FrequencyMap["7D"] = 194064000;
    FrequencyMap["8A"] = 195936000;
    FrequencyMap["8B"] = 197648000;
    FrequencyMap["8C"] = 199360000;
    FrequencyMap["8D"] = 201072000;
    FrequencyMap["9A"] = 202928000;
    FrequencyMap["9B"] = 204640000;
    FrequencyMap["9C"] = 206352000;
    FrequencyMap["9D"] = 208064000;
    FrequencyMap["10A"] = 209936000;
    FrequencyMap["10B"] = 211648000;
    FrequencyMap["10C"] = 213360000;
    FrequencyMap["10D"] = 215072000;
    FrequencyMap["11A"] = 216928000;
    FrequencyMap["11B"] = 218640000;
    FrequencyMap["11C"] = 220352000;
    FrequencyMap["11D"] = 222064000;
    FrequencyMap["12A"] = 223936000;
    FrequencyMap["12B"] = 225648000;
    FrequencyMap["12C"] = 227360000;
    FrequencyMap["12D"] = 229072000;
    FrequencyMap["13A"] = 230748000;
    FrequencyMap["13B"] = 232496000;
    FrequencyMap["13C"] = 234208000;
    FrequencyMap["13D"] = 235776000;
    FrequencyMap["13E"] = 237488000;
    FrequencyMap["13F"] = 239200000;

    // Band L
    FrequencyMap["LA"] = 1452960000;
    FrequencyMap["LB"] = 1454672000;
    FrequencyMap["LC"] = 1456384000;
    FrequencyMap["LD"] = 1458096000;
    FrequencyMap["LE"] = 1459808000;
    FrequencyMap["LF"] = 1461520000;
    FrequencyMap["LG"] = 1463232000;
    FrequencyMap["LH"] = 1464944000;
    FrequencyMap["LI"] = 1466656000;
    FrequencyMap["LJ"] = 1468368000;
    FrequencyMap["LK"] = 1470080000;
    FrequencyMap["LL"] = 1471792000;
    FrequencyMap["LM"] = 1473504000;
    FrequencyMap["LN"] = 1475216000;
    FrequencyMap["LO"] = 1476928000;
    FrequencyMap["LP"] = 1478640000;

    // Init with first frequency
    CurrentChannel = FirstChannel;
    CurrentFrequency = getFrequency(FirstChannel);
    CurrentFrequencyIndex = 0;
}

int CChannels::getFrequency(QString ChannelName)
{
    int Frequency = 0;

    try {
        Frequency = FrequencyMap.at(ChannelName);
    }

    catch (...) {
        qDebug() << "DABConstants:"
                 << "Frequency doesn't exists";
        Frequency = 0;
    }

    CurrentFrequency = Frequency;
    CurrentChannel = ChannelName;

    // Get index of current fequency
    for(int i=0; i<NUMBEROFCHANNELS; i++)
        if(getChannelNameAtIndex(i) == ChannelName) CurrentFrequencyIndex = i;

    return Frequency;
}

QString CChannels::getNextChannel(void)
{
    CurrentFrequencyIndex++;

    if(CurrentFrequencyIndex >= NUMBEROFCHANNELS)
        return QString();
    else
        return getChannelNameAtIndex(CurrentFrequencyIndex);
}

QString CChannels::getCurrentChannel(void)
{
    return CurrentChannel;
}

int CChannels::getCurrentFrequency(void)
{
    return CurrentFrequency;
}

int CChannels::getCurrentIndex()
{
    return CurrentFrequencyIndex;
}


QString CChannels::getChannelNameAtIndex(int Index)
{
    QString ChannelName = "";

    switch(Index)
    {
    // Band III
    case 0: ChannelName =  "5A"; break;
    case 1: ChannelName =  "5B"; break;
    case 2: ChannelName =  "5C"; break;
    case 3: ChannelName =  "5D"; break;
    case 4: ChannelName =  "6A"; break;
    case 5: ChannelName =  "6B"; break;
    case 6: ChannelName =  "6C"; break;
    case 7: ChannelName =  "6D"; break;
    case 8: ChannelName =  "7A"; break;
    case 9: ChannelName =  "7B"; break;
    case 10: ChannelName = "7C"; break;
    case 11: ChannelName = "7D"; break;
    case 12: ChannelName = "8A"; break;
    case 13: ChannelName = "8B"; break;
    case 14: ChannelName = "8C"; break;
    case 15: ChannelName = "8D"; break;
    case 16: ChannelName = "9A"; break;
    case 17: ChannelName = "9B"; break;
    case 18: ChannelName = "9C"; break;
    case 19: ChannelName = "9D"; break;
    case 20: ChannelName = "10A"; break;
    case 21: ChannelName = "10B"; break;
    case 22: ChannelName = "10C"; break;
    case 23: ChannelName = "10D"; break;
    case 24: ChannelName = "11A"; break;
    case 25: ChannelName = "11B"; break;
    case 26: ChannelName = "11C"; break;
    case 27: ChannelName = "11D"; break;
    case 28: ChannelName = "12A"; break;
    case 29: ChannelName = "12B"; break;
    case 30: ChannelName = "12C"; break;
    case 31: ChannelName = "12D"; break;
    case 32: ChannelName = "13A"; break;
    case 33: ChannelName = "13B"; break;
    case 34: ChannelName = "13C"; break;
    case 35: ChannelName = "13D"; break;
    case 36: ChannelName = "13E"; break;
    case 37: ChannelName = "13F"; break;

    // Band L
    case 38: ChannelName = "LA"; break;
    case 39: ChannelName = "LB"; break;
    case 40: ChannelName = "LC"; break;
    case 41: ChannelName = "LD"; break;
    case 42: ChannelName = "LE"; break;
    case 43: ChannelName = "LF"; break;
    case 44: ChannelName = "LG"; break;
    case 45: ChannelName = "LH"; break;
    case 46: ChannelName = "LI"; break;
    case 47: ChannelName = "LJ"; break;
    case 48: ChannelName = "LK"; break;
    case 49: ChannelName = "LL"; break;
    case 50: ChannelName = "LM"; break;
    case 51: ChannelName = "LN"; break;
    case 52: ChannelName = "LO"; break;
    case 53: ChannelName = "LP"; break;
    default: qDebug() << "DABConstants:"
                      << "No channel name at index";
    }

    return ChannelName;
}
