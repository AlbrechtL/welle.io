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
    case 0: TypeName = tr("none"); break;
    case 1: TypeName = tr("News"); break;
    case 2: TypeName = tr("Current Affairs"); break;
    case 3: TypeName = tr("Information"); break;
    case 4: TypeName = tr("Sport"); break;
    case 5: TypeName = tr("Education"); break;
    case 6: TypeName = tr("Drama"); break;
    case 7: TypeName = tr("Arts"); break;
    case 8: TypeName = tr("Science"); break;
    case 9: TypeName = tr("Talk"); break;
    case 10: TypeName = tr("Pop Music"); break;
    case 11: TypeName = tr("Rock Music"); break;
    case 12: TypeName = tr("Easy Listening"); break;
    case 13: TypeName = tr("Light classical"); break;
    case 14: TypeName = tr("Classical Music"); break;
    case 15: TypeName = tr("Other Music"); break;
    case 16: TypeName = tr("Weather"); break;
    case 17: TypeName = tr("Finance"); break;
    case 18: TypeName = tr("Children\'s"); break;
    case 19: TypeName = tr("Factual"); break;
    case 20: TypeName = tr("Religion"); break;
    case 21: TypeName = tr("Phone In"); break;
    case 22: TypeName = tr("Travel"); break;
    case 23: TypeName = tr("Leisure"); break;
    case 24: TypeName = tr("Jazz and Blues"); break;
    case 25: TypeName = tr("Country Music"); break;
    case 26: TypeName = tr("National Music"); break;
    case 27: TypeName = tr("Oldies Music"); break;
    case 28: TypeName = tr("Folk Music"); break;
    case 29: TypeName = tr("entry 29 not used"); break;
    case 30: TypeName = tr("entry 30 not used"); break;
    case 31: TypeName = tr("entry 31 not used"); break;
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
    case 0: LanguageName = tr("Unknown"); break;
    case 1: LanguageName = tr("Albanian"); break;
    case 2: LanguageName = tr("Breton"); break;
    case 3: LanguageName = tr("Catalan"); break;
    case 4: LanguageName = tr("Croatian"); break;
    case 5: LanguageName = tr("Welsh"); break;
    case 6: LanguageName = tr("Czech"); break;
    case 7: LanguageName = tr("Danish"); break;
    case 8: LanguageName = tr("German"); break;
    case 9: LanguageName = tr("English"); break;
    case 10: LanguageName = tr("Spanish"); break;
    case 11: LanguageName = tr("Esperanto"); break;
    case 12: LanguageName = tr("Estonian"); break;
    case 13: LanguageName = tr("Basque"); break;
    case 14: LanguageName = tr("Faroese"); break;
    case 15: LanguageName = tr("French"); break;
    case 16: LanguageName = tr("Frisian"); break;
    case 17: LanguageName = tr("Irish"); break;
    case 18: LanguageName = tr("Gaelic"); break;
    case 19: LanguageName = tr("Galician"); break;
    case 20: LanguageName = tr("Icelandic"); break;
    case 21: LanguageName = tr("Italian"); break;
    case 22: LanguageName = tr("Lappish"); break;
    case 23: LanguageName = tr("Latin"); break;
    case 24: LanguageName = tr("Latvian"); break;
    case 25: LanguageName = tr("Luxembourgian"); break;
    case 26: LanguageName = tr("Lithuanian"); break;
    case 27: LanguageName = tr("Hungarian"); break;
    case 28: LanguageName = tr("Maltese"); break;
    case 29: LanguageName = tr("Dutch"); break;
    case 30: LanguageName = tr("Norwegian"); break;
    case 31: LanguageName = tr("Occitan"); break;
    case 32: LanguageName = tr("Polish"); break;
    case 33: LanguageName = tr("Portuguese"); break;
    case 34: LanguageName = tr("Romanian"); break;
    case 35: LanguageName = tr("Romansh"); break;
    case 36: LanguageName = tr("Serbian"); break;
    case 37: LanguageName = tr("Slovak"); break;
    case 38: LanguageName = tr("Slovene"); break;
    case 39: LanguageName = tr("Finnish"); break;
    case 40: LanguageName = tr("Swedish"); break;
    case 41: LanguageName = tr("Turkish"); break;
    case 42: LanguageName = tr("Flemish"); break;
    case 43: LanguageName = tr("Walloon"); break;
    case 64: LanguageName = tr("Background sound/clean feed"); break;
    case 69: LanguageName = tr("Zulu"); break;
    case 70: LanguageName = tr("Vietnamese"); break;
    case 71: LanguageName = tr("Uzbek"); break;
    case 72: LanguageName = tr("Urdu"); break;
    case 73: LanguageName = tr("Ukranian"); break;
    case 74: LanguageName = tr("Thai"); break;
    case 75: LanguageName = tr("Telugu"); break;
    case 76: LanguageName = tr("Tatar"); break;
    case 77: LanguageName = tr("Tamil"); break;
    case 78: LanguageName = tr("Tadzhik"); break;
    case 79: LanguageName = tr("Swahili"); break;
    case 80: LanguageName = tr("Sranan Tongo"); break;
    case 81: LanguageName = tr("Somali"); break;
    case 82: LanguageName = tr("Sinhalese"); break;
    case 83: LanguageName = tr("Shona"); break;
    case 84: LanguageName = tr("Serbo-Croat"); break;
    case 85: LanguageName = tr("Rusyn"); break;
    case 86: LanguageName = tr("Russian"); break;
    case 87: LanguageName = tr("Quechua"); break;
    case 88: LanguageName = tr("Pushtu"); break;
    case 89: LanguageName = tr("Punjabi"); break;
    case 90: LanguageName = tr("Persian"); break;
    case 91: LanguageName = tr("Papiamento"); break;
    case 92: LanguageName = tr("Oriya"); break;
    case 93: LanguageName = tr("Nepali"); break;
    case 94: LanguageName = tr("Ndebele"); break;
    case 95: LanguageName = tr("Marathi"); break;
    case 96: LanguageName = tr("Moldavian"); break;
    case 97: LanguageName = tr("Malaysian"); break;
    case 98: LanguageName = tr("Malagasay"); break;
    case 99: LanguageName = tr("Macedonian"); break;
    case 100: LanguageName = tr("Laotian"); break;
    case 101: LanguageName = tr("Korean"); break;
    case 102: LanguageName = tr("Khmer"); break;
    case 103: LanguageName = tr("Kazakh"); break;
    case 104: LanguageName = tr("Kannada"); break;
    case 105: LanguageName = tr("Japanese"); break;
    case 106: LanguageName = tr("Indonesian"); break;
    case 107: LanguageName = tr("Hindi"); break;
    case 108: LanguageName = tr("Hebrew"); break;
    case 109: LanguageName = tr("Hausa"); break;
    case 110: LanguageName = tr("Gurani"); break;
    case 111: LanguageName = tr("Gujurati"); break;
    case 112: LanguageName = tr("Greek"); break;
    case 113: LanguageName = tr("Georgian"); break;
    case 114: LanguageName = tr("Fulani"); break;
    case 115: LanguageName = tr("Dari"); break;
    case 116: LanguageName = tr("Chuvash"); break;
    case 117: LanguageName = tr("Chinese"); break;
    case 118: LanguageName = tr("Burmese"); break;
    case 119: LanguageName = tr("Bulgarian"); break;
    case 120: LanguageName = tr("Bengali"); break;
    case 121: LanguageName = tr("Belorussian"); break;
    case 122: LanguageName = tr("Bambora"); break;
    case 123: LanguageName = tr("Azerbaijani"); break;
    case 124: LanguageName = tr("Assamese"); break;
    case 125: LanguageName = tr("Armenian"); break;
    case 126: LanguageName = tr("Arabic"); break;
    case 127: LanguageName = tr("Amharic"); break;
    default: qDebug() << "DABConstants:"
                      << "Unknown language type: " << Language;
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
