/*
 *
 * This file is part of the 'NewsService Journaline(R) Decoder'
 *
 * Copyright (c) 2003, 2001-2014 by Fraunhofer IIS, Erlangen, Germany
 *
 * --------------------------------------------------------------------
 *
 * For NON-COMMERCIAL USE,
 * the 'NewsService Journaline(R) Decoder' is free software;
 * you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The 'NewsService Journaline(R) Decoder' is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the 'NewsService Journaline(R) Decoder';
 * if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * If you use this software in a project with user interaction, please
 * provide the following text to the user in an appropriate place:
 * "Features NewsService Journaline(R) decoder technology by
 * Fraunhofer IIS, Erlangen, Germany.
 * For more information visit http://www.iis.fhg.de/dab"
 *
 * --------------------------------------------------------------------
 *
 * To use the 'NewsService Journaline(R) Decoder' software for
 * COMMERCIAL purposes, please contact Fraunhofer IIS for a
 * commercial license (see below for contact information)!
 *
 * --------------------------------------------------------------------
 *
 * Contact:
 *   Fraunhofer IIS, Department 'Broadcast Applications'
 *   Am Wolfsmantel 33, 91058 Erlangen, Germany
 *   http://www.iis.fraunhofer.de/dab
 *   mailto:bc-info@iis.fraunhofer.de
 *
 */

//////////////////////////////////////////////////////////////////////////////
/// different string splitting routines
///
/// @file       Splitter.cpp
/// @author     Michael Reichenbächer <rbr@iis.fraunhofer.de>
///
/// $Id: Splitter.cpp,v 1.2 2008/12/26 17:18:08 jcable Exp $
///
/// Module:     Journaline(R)
///
/// Copyright:  (C) 2003-2001-2014 by Fraunhofer IIS-A, IT-Services, Erlangen
///
/// Compiler:   gcc version 3.3 20030226 (prerelease)
///             Microsoft Visual C++ .NET
///
//////////////////////////////////////////////////////////////////////////////

#include "Splitter.h"


Splitter::Splitter()
{
    SetLineBreakCharacter('\n');
}


Splitter::~Splitter()
{
}


/// @brief set line break character for Split
/// sets the LineBreakCharacter used by the Split
/// method
/// @param lbc  line break character
/// @return     always returns true
bool Splitter::SetLineBreakCharacter(char lbc)
{
    if (lbc==0) return false;
    _LineBreak = lbc;
    return true;
}


/// @brief split a string into a vector of lines
/// splits a string consisting of several lines
/// (seperated by the LineBreakCharacter) into
/// an array containing the lines
/// (excluding the LineBreakCharacter)
/// @attention
///   An empty string will split into an array
///   consisting of only the empty string
/// @param dest  vector of lines
/// @param src   source string
/// @return      always returns true
bool Splitter::Split(std::vector<std::string> & dest,
                     const std::string & src) const
{
    std::string::size_type left, right;
    dest.clear();

    if (src.size()==0)
    {
        dest.push_back("");
        return true;
    }

    for (left=0,right=0;;)
    {
        // search first non-delimiter
        left = src.find_first_not_of(_LineBreak, right);
        if (left==std::string::npos)
        {
            // rest of the string are delimiters
            //dest.push_back("");
            break;
        }
        // search first delimiter after series of non-delimiters
        // => right>left (or right=npos!)
        right = src.find_first_of(_LineBreak, left);
        if (right==std::string::npos)
        {
            // rest of the string are non-delimiters
            dest.push_back(src.substr(left));
            break;
        }
        // substring src[left,right[ are non-delimiters
        dest.push_back(src.substr(left, right - left));
    }

    return true;
}
