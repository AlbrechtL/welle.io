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


CDABParams::CDABParams()
{
    setMode(1);
}

CDABParams::CDABParams(int Mode)
{
    setMode(Mode);
}

void CDABParams::setMode(int Mode)
{

    switch(Mode)
    {
    case 1: setMode1(); break;
    case 2: setMode2(); break;
    case 3: setMode3(); break;
    case 4: setMode4(); break;
    default: qDebug() << "DABConstants:"
                      << "Unknown mode";
    }
}

void CDABParams::setMode1()
{
    dabMode = 1;
    L = 76;
    K = 1536;
    T_F = 196608;
    T_null = 2656;
    T_s = 2552;
    T_u = 2048;
    guardLength = 504;
    carrierDiff = 1000;
}


void CDABParams::setMode2()
{
    dabMode = 2;
    L = 76;
    K = 384;
    T_null = 664;
    T_F = 49152;
    T_s = 638;
    T_u = 512;
    guardLength = 126;
    carrierDiff = 4000;
}


void CDABParams::setMode3()
{
    dabMode = 3;
    L = 153;
    K = 192;
    T_F = 49152;
    T_null = 345;
    T_s = 319;
    T_u = 256;
    guardLength = 63;
    carrierDiff = 2000;
}

void CDABParams::setMode4()
{
    dabMode = 4;
    L = 76;
    K = 768;
    T_F = 98304;
    T_null = 1328;
    T_s = 1276;
    T_u = 1024;
    guardLength = 252;
    carrierDiff = 2000;
}
