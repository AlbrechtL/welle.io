/******************************************************************************\
 * Copyright (c) 2017 Albrecht Lohofener <albrechtloh@gmx.de>
 *
 * Author(s):
 * Albrecht Lohofener
 *
 * Description:
 * Just a wrapper class to compile "pad_decoder.cpp" and "mot_manager.cpp"
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "fic_decoder.h"
#include "charsets.h"

std::string FICDecoder::ConvertTextToUTF8(const uint8_t *data, size_t len, int charset)
{
    QString segmentText = toQStringUsingCharset (
                               (const char *)data,
                               (CharacterSet) charset,
                               len);

    return segmentText.toStdString();
}

