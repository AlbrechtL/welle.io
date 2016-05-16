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
/// @file       Splitter.h
/// @author     Michael Reichenbächer <rbr@iis.fraunhofer.de>
///
/// $Id: Splitter.h,v 1.3 2008/12/26 20:04:53 jcable Exp $
///
/// Module:     Journaline(R)
///
/// Copyright:  (C) 2003-2001-2014 by Fraunhofer IIS-A, IT-Services, Erlangen
///
/// Compiler:   gcc version 3.3 20030226 (prerelease)
///             Microsoft Visual C++ .NET
///
//////////////////////////////////////////////////////////////////////////////


#ifndef _SPLITTER_H_
#define _SPLITTER_H_

#ifdef _MSC_VER
#pragma warning(push,3)
#pragma warning(disable:4514)
#pragma warning(disable:4786)
#endif

#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(disable:4786)
#pragma warning(disable:4514)
#endif



/// interface for string splitting routines
class StringSplitter
{
public:
    virtual bool Split(std::vector<std::string> & dest,
                       const std::string & src) const = 0;
    virtual ~StringSplitter() {}
};


/// line splitter algorithm
class Splitter : public StringSplitter
{
public:
    Splitter();
    virtual ~Splitter();

    virtual bool SetLineBreakCharacter(char lbc);
    virtual bool Split(std::vector<std::string> & dest,
                       const std::string & src) const;
private:
    char _LineBreak;
};


#endif

