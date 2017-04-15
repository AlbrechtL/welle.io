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
    case 1: TypeName = tr("news"); break;
    case 2: TypeName = tr("current affairs"); break;
    case 3: TypeName = tr("information"); break;
    case 4: TypeName = tr("sport"); break;
    case 5: TypeName = tr("education"); break;
    case 6: TypeName = tr("dram"); break;
    case 7: TypeName = tr("arts"); break;
    case 8: TypeName = tr("science"); break;
    case 9: TypeName = tr("talk"); break;
    case 10: TypeName = tr("pop music"); break;
    case 11: TypeName = tr("rock music"); break;
    case 12: TypeName = tr("easy listening"); break;
    case 13: TypeName = tr("light classical"); break;
    case 14: TypeName = tr("classical music"); break;
    case 15: TypeName = tr("other music"); break;
    case 16: TypeName = tr("wheather"); break;
    case 17: TypeName = tr("finance"); break;
    case 18: TypeName = tr("children\'s"); break;
    case 19: TypeName = tr("factual"); break;
    case 20: TypeName = tr("religion"); break;
    case 21: TypeName = tr("phone in"); break;
    case 22: TypeName = tr("travel"); break;
    case 23: TypeName = tr("leisure"); break;
    case 24: TypeName = tr("jazz and blues"); break;
    case 25: TypeName = tr("country music"); break;
    case 26: TypeName = tr("national music"); break;
    case 27: TypeName = tr("oldies music"); break;
    case 28: TypeName = tr("folk music"); break;
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
    case 18: LanguageName = tr("GaeliC"); break;
    case 19: LanguageName = tr("Galician"); break;
    case 20: LanguageName = tr("IcelandiC"); break;
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
    case 33: LanguageName = tr("Postuguese"); break;
    case 34: LanguageName = tr("Romanian"); break;
    case 35: LanguageName = tr("Romansh"); break;
    case 36: LanguageName = tr("Serbian"); break;
    case 37: LanguageName = tr("Slovak"); break;
    case 38: LanguageName = tr("Slovene"); break;
    case 39: LanguageName = tr("Finnish"); break;
    case 40: LanguageName = tr("Swedish"); break;
    case 41: LanguageName = tr("Tuskish"); break;
    case 42: LanguageName = tr("Flemish"); break;
    case 43: LanguageName = tr("Walloon"); break;
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
