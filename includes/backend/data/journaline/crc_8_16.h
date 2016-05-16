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



/*!
 * @brief       CRC calculation
 *
 * functions for CRC calculation
 *
 * @file        crc_8_16.h
 *
 * $Id: crc_8_16.h,v 1.2 2008/12/26 17:18:08 jcable Exp $
 *
 * Author:      Monica Redon Segrera and Nuria Llombart Juan
 *
 * Copyright:  (C) 2003-2001-2014 by Fraunhofer IIS-A, IT-Services, Erlangen
 *
 * Created:     2001-01-02
 *
 * Version:     2001-2014-03-02
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* calculates the CRC-16 value */
unsigned short CRC_Build_16(const unsigned char *cDataPointer,
                            const unsigned long iLength);

/* calculates the CRC-8 value */
unsigned char CRC_Build_8(const unsigned char *cDataPointer,
                          const unsigned long iLength);

/* checks if there are errors in the received data */
char CRC_Check_16(const unsigned char *cDataPointer,
                  const unsigned long iLength,
                  const unsigned short iCRC_16);

/* checks if there are errors in the received data */
char CRC_Check_8(const unsigned char *cDataPointer,
                 const unsigned long iLength,
                 const unsigned char iCRC_8);


#ifdef __cplusplus
}
#endif /* __cplusplus */
