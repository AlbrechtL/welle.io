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

#ifndef CCHANNELS_H
#define CCHANNELS_H

#include <map>

#include <QString>

#define NUMBEROFCHANNELS 54

class CChannels
{
public:
    CChannels();
    int getFrequency(QString ChannelName);
    QString getNextChannel(void);
    QString getCurrentChannel(void);
    int getCurrentFrequency(void);
    int getCurrentIndex(void);

    static QString FirstChannel;

private:
    QString getChannelNameAtIndex(int Index);

    std::map<QString, int> FrequencyMap;
    int CurrentFrequencyIndex;
    QString CurrentChannel;
    int CurrentFrequency;
};

#endif // CCHANNELS_H
