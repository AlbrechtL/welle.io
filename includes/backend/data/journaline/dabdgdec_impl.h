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

#ifndef __DAB_DGDEC_IMPL__
#define __DAB_DGDEC_IMPL__
/**
***             techidee GmbH
*** Projekt:    NewsBox
*** Autor:      Thomas Fruehwald
***
*** Compiler:   gcc
*** Modul:
        dab datagroup decoder
        implementation
***
***
**/
#include "dabdatagroupdecoder.h"
#ifdef __cplusplus
extern "C" {
#endif

extern int showDdDabDgDecInfo;
extern int showDdDabDgDecErr;

#define DAB_DGDEC_MAGIC_ID  0x49871

typedef struct
{
    unsigned long               magicId;
    DAB_DATAGROUP_DECODER_data  *cb;
    void                        *arg;
} DAB_DGDEC_IMPL_t;

int DAB_DGDEC_IMPL_extractMscDatagroupHeader(
    unsigned long len,
    const unsigned char *buf,
    DAB_DATAGROUP_DECODER_msc_datagroup_header_t *header);
void DAB_DGDEC_IMPL_showMscDatagroupHeader(
    DAB_DATAGROUP_DECODER_msc_datagroup_header_t *header);

int DAB_DGDEC_IMPL_checkCrc(const unsigned char *buf,
                            unsigned long len, unsigned short crc_field);

#ifdef __cplusplus
}
#endif

#endif /* __DAB_DGDEC_IMPL__ */
