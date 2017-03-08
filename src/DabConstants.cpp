/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
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

#include <qdebug.h>

#include "DabConstants.h"

std::map<QString, int> CDABConstants::FrequencyMap;
bool CDABConstants::isFrequencyMapFiled = false;

int CDABConstants::getFrequency(QString ChannelName)
{
    if (!isFrequencyMapFiled)
        fillFrequencyMap();

    int Frequency = 0;

    try {
        Frequency = FrequencyMap.at(ChannelName);
    }

    catch (...) {
        qDebug() << "DABConstants:"
                 << "Frequency doesn't exists";
        Frequency = 0;
    }

    return Frequency;
}

QString CDABConstants::getChannelNameAtIndex(int Index)
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

QString CDABConstants::getProgramTypeName(int Type)
{
    QString TypeName = "";

    switch(Type)
    {
    case 0: TypeName = "none"; break;
    case 1: TypeName = "news"; break;
    case 2: TypeName = "current affairs"; break;
    case 3: TypeName = "information"; break;
    case 4: TypeName = "sport"; break;
    case 5: TypeName = "education"; break;
    case 6: TypeName = "dram"; break;; break;
    case 7: TypeName = "arts"; break;
    case 8: TypeName = "science"; break;
    case 9: TypeName = "talk"; break;
    case 10: TypeName = "pop music"; break;; break;
    case 11: TypeName = "rock music"; break;; break;
    case 12: TypeName = "easy listening"; break;
    case 13: TypeName = "light classical"; break;
    case 14: TypeName = "classical music"; break;; break;
    case 15: TypeName = "other music"; break;; break;
    case 16: TypeName = "wheather"; break;
    case 17: TypeName = "finance"; break;
    case 18: TypeName = "children\'s"; break;
    case 19: TypeName = "factual"; break;
    case 20: TypeName = "religion"; break;
    case 21: TypeName = "phone in"; break;
    case 22: TypeName = "travel"; break;
    case 23: TypeName = "leisure"; break;
    case 24: TypeName = "jazz and blues"; break;
    case 25: TypeName = "country music"; break;; break;
    case 26: TypeName = "national music"; break;; break;
    case 27: TypeName = "oldies music"; break;; break;
    case 28: TypeName = "folk music"; break;; break;
    case 29: TypeName = "entry 29 not used"; break;; break;
    case 30: TypeName = "entry 30 not used"; break;; break;
    case 31: TypeName = "entry 31 not used"; break;; break;
    default: qDebug() << "DABConstants:"
                      << "Unknown program type";
    }

    return TypeName;
}

QString CDABConstants::getLanguageName(int Language)
{
    QString LanguageName = "";

    switch(Language)
    {
    case 0: LanguageName = "Unknown"; break;
    case 1: LanguageName = "Albanian"; break;
    case 2: LanguageName = "Breton"; break;
    case 3: LanguageName = "Catalan"; break;
    case 4: LanguageName = "Croatian"; break;
    case 5: LanguageName = "Welsh"; break;
    case 6: LanguageName = "Czech"; break;
    case 7: LanguageName = "Danish"; break;
    case 8: LanguageName = "German"; break;
    case 9: LanguageName = "English"; break;
    case 10: LanguageName = "Spanish"; break;
    case 11: LanguageName = "Esperanto"; break;
    case 12: LanguageName = "Estonian"; break;
    case 13: LanguageName = "Basque"; break;
    case 14: LanguageName = "Faroese"; break;
    case 15: LanguageName = "French"; break;
    case 16: LanguageName = "Frisian"; break;
    case 17: LanguageName = "Irish"; break;
    case 18: LanguageName = "GaeliC"; break;; break;
    case 19: LanguageName = "Galician"; break;
    case 20: LanguageName = "IcelandiC"; break;; break;
    case 21: LanguageName = "Italian"; break;
    case 22: LanguageName = "Lappish"; break;
    case 23: LanguageName = "Latin"; break;
    case 24: LanguageName = "Latvian"; break;
    case 25: LanguageName = "Luxembourgian"; break;
    case 26: LanguageName = "Lithuanian"; break;
    case 27: LanguageName = "Hungarian"; break;
    case 28: LanguageName = "Maltese"; break;
    case 29: LanguageName = "Dutch"; break;
    case 30: LanguageName = "Norwegian"; break;
    case 31: LanguageName = "Occitan"; break;
    case 32: LanguageName = "Polish"; break;
    case 33: LanguageName = "Postuguese"; break;
    case 34: LanguageName = "Romanian"; break;
    case 35: LanguageName = "Romansh"; break;
    case 36: LanguageName = "Serbian"; break;
    case 37: LanguageName = "Slovak"; break;
    case 38: LanguageName = "Slovene"; break;
    case 39: LanguageName = "Finnish"; break;
    case 40: LanguageName = "Swedish"; break;
    case 41: LanguageName = "Tuskish"; break;
    case 42: LanguageName = "Flemish"; break;
    case 43: LanguageName = "Walloon"; break;
    default: qDebug() << "DABConstants:"
                      << "Unknown language type";
    }

    return LanguageName;
}

void CDABConstants::fillFrequencyMap(void)
{
    // Band III
    FrequencyMap["5A"] = 174928;
    FrequencyMap["5B"] = 176640;
    FrequencyMap["5C"] = 178352;
    FrequencyMap["5D"] = 180064;
    FrequencyMap["6A"] = 181936;
    FrequencyMap["6B"] = 183648;
    FrequencyMap["6C"] = 185360;
    FrequencyMap["6D"] = 187072;
    FrequencyMap["7A"] = 188928;
    FrequencyMap["7B"] = 190640;
    FrequencyMap["7C"] = 192352;
    FrequencyMap["7D"] = 194064;
    FrequencyMap["8A"] = 195936;
    FrequencyMap["8B"] = 197648;
    FrequencyMap["8C"] = 199360;
    FrequencyMap["8D"] = 201072;
    FrequencyMap["9A"] = 202928;
    FrequencyMap["9B"] = 204640;
    FrequencyMap["9C"] = 206352;
    FrequencyMap["9D"] = 208064;
    FrequencyMap["10A"] = 209936;
    FrequencyMap["10B"] = 211648;
    FrequencyMap["10C"] = 213360;
    FrequencyMap["10D"] = 215072;
    FrequencyMap["11A"] = 216928;
    FrequencyMap["11B"] = 218640;
    FrequencyMap["11C"] = 220352;
    FrequencyMap["11D"] = 222064;
    FrequencyMap["12A"] = 223936;
    FrequencyMap["12B"] = 225648;
    FrequencyMap["12C"] = 227360;
    FrequencyMap["12D"] = 229072;
    FrequencyMap["13A"] = 230748;
    FrequencyMap["13B"] = 232496;
    FrequencyMap["13C"] = 234208;
    FrequencyMap["13D"] = 235776;
    FrequencyMap["13E"] = 237488;
    FrequencyMap["13F"] = 239200;

    // Band L
    FrequencyMap["LA"] = 1452960;
    FrequencyMap["LB"] = 1454672;
    FrequencyMap["LC"] = 1456384;
    FrequencyMap["LD"] = 1458096;
    FrequencyMap["LE"] = 1459808;
    FrequencyMap["LF"] = 1461520;
    FrequencyMap["LG"] = 1463232;
    FrequencyMap["LH"] = 1464944;
    FrequencyMap["LI"] = 1466656;
    FrequencyMap["LJ"] = 1468368;
    FrequencyMap["LK"] = 1470080;
    FrequencyMap["LL"] = 1471792;
    FrequencyMap["LM"] = 1473504;
    FrequencyMap["LN"] = 1475216;
    FrequencyMap["LO"] = 1476928;
    FrequencyMap["LP"] = 1478640;

    isFrequencyMapFiled = true;
}
