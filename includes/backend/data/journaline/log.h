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

#ifndef __LOG__
#define __LOG__

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_ERR       1, __FILE__, __LINE__, 0, 0, 0
#define LOG_ERR_API   1, __FILE__, __LINE__, 1, 0, 0
#define LOG_ERR_DUMP  1, __FILE__, __LINE__, 0, 1, 0

#define LOG_WARN      2, __FILE__, __LINE__, 0, 0, 0
#define LOG_WARN_DUMP 2, __FILE__, __LINE__, 0, 1, 0

#define LOG_INFO      4, __FILE__, __LINE__, 0, 0, 0
#define LOG_INFO_NOTS 4, __FILE__, __LINE__, 0, 0, 1
#define LOG_INFO_DUMP 4, __FILE__, __LINE__, 0, 1, 0

void logit(unsigned long level,
           const char *file,
           const int line,
           int api_err,
           int dumpBuffer,
           int nots,
           const char *format,
           ...);

#ifdef __cplusplus
}
#endif

#endif
