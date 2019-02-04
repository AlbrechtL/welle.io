/*
 *    Copyright (C) 2013
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
#ifndef __CHARSETS_H
#define __CHARSETS_H

#include <cstdint>
#include <string>

/*
 * Codes assigned to character sets, as defined
 * in ETSI TS 101 756 v1.6.1, section 5.2.
 */
enum class CharacterSet : uint8_t {
    EbuLatin = 0x00, // Complete EBU Latin based repertoire - see annex C
    UnicodeUcs2 = 0x06,
    UnicodeUtf8 = 0x0F,
    Undefined,
};

/**
 * Converts the string from the given charset to a UTF-8
 * encoded string.
 *
 * If num_bytes is nonzero, the buffer must be zero
 * terminated.
 */
std::string toUtf8StringUsingCharset(const void* buffer,
        CharacterSet charset, size_t num_bytes = 0);

#endif // CHARSETS_H

