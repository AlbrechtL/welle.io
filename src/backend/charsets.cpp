/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J (JSDR).
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  This charset handling was kindly added by Przemyslaw Wegrzyn
 *  all rights acknowledged
 */
#include "charsets.h"
#include <cstdint>
#include <locale>
#include <codecvt>
/**
 * This table maps "EBU Latin" charset to corresponding
 * Unicode (UCS2-encoded) characters.
 * See ETSI TS 101 756 v1.8.1, Annex C
 */
static const unsigned short ebuLatinToUcs2[] = {
/* 0x00 - 0x07 */   0x00,  0x118,  0x12e,  0x172,  0x102,  0x116,  0x10e,  0x218,
/*  0x8 -  0xf */  0x21a,  0x10a,    0xa,    0xb,  0x120,  0x139,  0x17b,  0x143,
/* 0x10 - 0x17 */  0x105,  0x119,  0x12f,  0x173,  0x103,  0x117,  0x10f,  0x219,
/* 0x18 - 0x1f */  0x21b,  0x10b,  0x147,  0x11a,  0x121,  0x13a,  0x17c,   0x82,
/* 0x20 - 0x27 */   0x20,   0x21,   0x22,   0x23,  0x142,   0x25,   0x26,   0x27,
/* 0x28 - 0x2f */   0x28,   0x29,   0x2a,   0x2b,   0x2c,   0x2d,   0x2e,   0x2f,
/* 0x30 - 0x37 */   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,   0x36,   0x37,
/* 0x38 - 0x3f */   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,   0x3f,
/* 0x40 - 0x47 */   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
/* 0x48 - 0x4f */   0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,
/* 0x50 - 0x57 */   0x50,   0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,
/* 0x58 - 0x5f */   0x58,   0x59,   0x5a,   0x5b,  0x16e,   0x5d,  0x141,   0x5f,
/* 0x60 - 0x67 */  0x104,   0x61,   0x62,   0x63,   0x64,   0x65,   0x66,   0x67,
/* 0x68 - 0x6f */   0x68,   0x69,   0x6a,   0x6b,   0x6c,   0x6d,   0x6e,   0x6f,
/* 0x70 - 0x77 */   0x70,   0x71,   0x72,   0x73,   0x74,   0x75,   0x76,   0x77,
/* 0x78 - 0x7f */   0x78,   0x79,   0x7a,   0xab,  0x16f,   0xbb,  0x13d,  0x126,
/* 0x80 - 0x87 */   0xe1,   0xe0,   0xe9,   0xe8,   0xed,   0xec,   0xf3,   0xf2,
/* 0x88 - 0x8f */   0xfa,   0xf9,   0xd1,   0xc7,  0x15e,   0xdf,   0xa1,  0x178,
/* 0x90 - 0x97 */   0xe2,   0xe4,   0xea,   0xeb,   0xee,   0xef,   0xf4,   0xf6,
/* 0x98 - 0x9f */   0xfb,   0xfc,   0xf1,   0xe7,  0x15f,  0x11f,  0x131,   0xff,
/* 0xa0 - 0xa7 */  0x136,  0x145,   0xa9,  0x122,  0x11e,  0x11b,  0x148,  0x151,
/* 0xa8 - 0xaf */  0x150, 0x20ac,   0xa3,   0x24,  0x100,  0x112,  0x12a,  0x16a,
/* 0xb0 - 0xb7 */  0x137,  0x146,  0x13b,  0x123,  0x13c,  0x130,  0x144,  0x171,
/* 0xb8 - 0xbf */  0x170,   0xbf,  0x13e,   0xb0,  0x101,  0x113,  0x12b,  0x16b,
/* 0xc0 - 0xc7 */   0xc1,   0xc0,   0xc9,   0xc8,   0xcd,   0xcc,   0xd3,   0xd2,
/* 0xc8 - 0xcf */   0xda,   0xd9,  0x158,  0x10c,  0x160,  0x17d,   0xd0,  0x13f,
/* 0xd0 - 0xd7 */   0xc2,   0xc4,   0xca,   0xcb,   0xce,   0xcf,   0xd4,   0xd6,
/* 0xd8 - 0xdf */   0xdb,   0xdc,  0x159,  0x10d,  0x161,  0x17e,  0x111,  0x140,
/* 0xe0 - 0xe7 */   0xc3,   0xc5,   0xc6,  0x152,  0x177,   0xdd,   0xd5,   0xd8,
/* 0xe8 - 0xef */   0xde,  0x14a,  0x154,  0x106,  0x15a,  0x179,  0x164,   0xf0,
/* 0xf0 - 0xf7 */   0xe3,   0xe5,   0xe6,  0x153,  0x175,   0xfd,   0xf5,   0xf8,
/* 0xf8 - 0xff */   0xfe,  0x14b,  0x155,  0x107,  0x15b,  0x17a,  0x165,  0x127
};


//The above table can be automatically generated from the table in ODR-PadEnc (ODR-PadEnc/src/charset.cpp) using the following source code:
// Compile like this: g++ -fPIC -DODRTABLE  -I  /usr/include/x86_64-linux-gnu/qt5/  -I /usr/include/x86_64-linux-gnu/qt5/QtCore/ -lQt5Core -o chartable charsets.cpp
#ifdef ODRTABLE

#include <iostream>
#include <string>
#include <iomanip>

#include "../../../ODR-PadEnc/src/charset.cpp"

int main(){
  std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> ucs2conv;
  std::cout << "/* 0x00 - 0x07 */   0x00, ";
  std::cout << std::setbase(16) << std::setfill(' ');
  for (int i=0;i<CHARSET_TABLE_ENTRIES;i++){
    std::u16string ucs2 = ucs2conv.from_bytes(utf8_encoded_EBU_Latin[i]);
    std::cout << std::showbase << std::setw(6) << ucs2[0];
    if ((i+2) % 8) {
      std::cout << ", ";
    }
    else{
      if (i == (CHARSET_TABLE_ENTRIES - 1)) std::cout << "\n";
      else {
        std::cout <<  ",\n/* "    << std::setw(4) << i+2 << " - " << std::setw(4) <<  i + 1 + 8 <<  " */ ";
      }
    }
  }
}

#endif

std::string toUtf8StringUsingCharset(const void* buffer,
        CharacterSet charset, size_t num_bytes)
{
    if (buffer == nullptr) {
        throw std::logic_error("Cannot convert charset of empty buffer");
    }

    switch (charset) {
        case CharacterSet::UnicodeUcs2:
            {
                std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8conv;

                if (num_bytes == 0) {
                    std::u16string buffer_char16(reinterpret_cast<const char16_t*>(buffer));
                    return utf8conv.to_bytes(buffer_char16);
                }
                else {
                    const char16_t* start = reinterpret_cast<const char16_t*>(buffer);
                    const char16_t* end = start + num_bytes/2;
                    std::u16string buffer_char16(start, end);
                    return utf8conv.to_bytes(buffer_char16);
                }
            }
            break;

        case CharacterSet::UnicodeUtf8:
            {
                if (num_bytes == 0) {
                    std::string buffer_char8(reinterpret_cast<const char*>(buffer));
                    return buffer_char8;
                }
                else {
                    const char* start = reinterpret_cast<const char*>(buffer);
                    const char* end = start + num_bytes;
                    std::string buffer_char8(start, end);
                    return buffer_char8;
                }
            }
            break;

        case CharacterSet::EbuLatin:
        default:
            {
                std::u16string s;
                const uint8_t* buf = reinterpret_cast<const uint8_t*>(buffer);

                std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8conv;

                if (num_bytes == 0) {
                    while (*buf) {
                        s += ebuLatinToUcs2[*buf];
                        buf++;
                    }
                }
                else {
                    for (size_t i = 0; i < num_bytes; i++) {
                        s += ebuLatinToUcs2[buf[i]];
                    }
                }

                return utf8conv.to_bytes(s);
            }
    }
}
