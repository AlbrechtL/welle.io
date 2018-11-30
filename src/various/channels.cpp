/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
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

#include <iostream>
#include "channels.h"

using namespace std;

string Channels::firstChannel = "5A";

Channels::Channels()
{
    // Band III
    frequencyMap["5A"] = 174928000;
    frequencyMap["5B"] = 176640000;
    frequencyMap["5C"] = 178352000;
    frequencyMap["5D"] = 180064000;
    frequencyMap["6A"] = 181936000;
    frequencyMap["6B"] = 183648000;
    frequencyMap["6C"] = 185360000;
    frequencyMap["6D"] = 187072000;
    frequencyMap["7A"] = 188928000;
    frequencyMap["7B"] = 190640000;
    frequencyMap["7C"] = 192352000;
    frequencyMap["7D"] = 194064000;
    frequencyMap["8A"] = 195936000;
    frequencyMap["8B"] = 197648000;
    frequencyMap["8C"] = 199360000;
    frequencyMap["8D"] = 201072000;
    frequencyMap["9A"] = 202928000;
    frequencyMap["9B"] = 204640000;
    frequencyMap["9C"] = 206352000;
    frequencyMap["9D"] = 208064000;
    frequencyMap["10A"] = 209936000;
    frequencyMap["10B"] = 211648000;
    frequencyMap["10C"] = 213360000;
    frequencyMap["10D"] = 215072000;
    frequencyMap["11A"] = 216928000;
    frequencyMap["11B"] = 218640000;
    frequencyMap["11C"] = 220352000;
    frequencyMap["11D"] = 222064000;
    frequencyMap["12A"] = 223936000;
    frequencyMap["12B"] = 225648000;
    frequencyMap["12C"] = 227360000;
    frequencyMap["12D"] = 229072000;

    // Init with first frequency
    currentChannel = firstChannel;
    currentFrequency = getFrequency(firstChannel);
    currentFrequencyIndex = 0;
}

int Channels::getFrequency(const string& channelName)
{
    int frequency = 0;

    try {
        frequency = frequencyMap.at(channelName);
    }
    catch (const std::out_of_range&) {
        clog << "DABConstants: Frequency doesn't exist" << endl;
        frequency = 0;
    }

    currentFrequency = frequency;
    currentChannel = channelName;

    // Get index of current fequency
    for (int i=0; i<NUMBEROFCHANNELS; i++) {
        if (getChannelNameAtIndex(i) == channelName) {
            currentFrequencyIndex = i;
        }
    }

    return frequency;
}

string Channels::getNextChannel(void)
{
    currentFrequencyIndex++;

    if (currentFrequencyIndex >= NUMBEROFCHANNELS)
        return "";
    else
        return getChannelNameAtIndex(currentFrequencyIndex);
}

string Channels::getCurrentChannel(void)
{
    return currentChannel;
}

int Channels::getCurrentFrequency(void)
{
    return currentFrequency;
}

int Channels::getCurrentIndex()
{
    return currentFrequencyIndex;
}

string Channels::getChannelNameAtIndex(int index)
{
    string channelName = "";

    switch(index)
    {
    // Band III
    case 0: channelName =  "5A"; break;
    case 1: channelName =  "5B"; break;
    case 2: channelName =  "5C"; break;
    case 3: channelName =  "5D"; break;
    case 4: channelName =  "6A"; break;
    case 5: channelName =  "6B"; break;
    case 6: channelName =  "6C"; break;
    case 7: channelName =  "6D"; break;
    case 8: channelName =  "7A"; break;
    case 9: channelName =  "7B"; break;
    case 10: channelName = "7C"; break;
    case 11: channelName = "7D"; break;
    case 12: channelName = "8A"; break;
    case 13: channelName = "8B"; break;
    case 14: channelName = "8C"; break;
    case 15: channelName = "8D"; break;
    case 16: channelName = "9A"; break;
    case 17: channelName = "9B"; break;
    case 18: channelName = "9C"; break;
    case 19: channelName = "9D"; break;
    case 20: channelName = "10A"; break;
    case 21: channelName = "10B"; break;
    case 22: channelName = "10C"; break;
    case 23: channelName = "10D"; break;
    case 24: channelName = "11A"; break;
    case 25: channelName = "11B"; break;
    case 26: channelName = "11C"; break;
    case 27: channelName = "11D"; break;
    case 28: channelName = "12A"; break;
    case 29: channelName = "12B"; break;
    case 30: channelName = "12C"; break;
    case 31: channelName = "12D"; break;

    default: clog << "DABConstants:"
                      << "No channel name at index" <<
                          to_string(index) << endl;
    }

    return channelName;
}

std::string Channels::getChannelForFrequency(int frequency)
{
    for (const auto c_f : frequencyMap) {
        if (c_f.second == frequency) {
            return c_f.first;
        }
    }
    throw out_of_range("frequency is outside channel list");
}
