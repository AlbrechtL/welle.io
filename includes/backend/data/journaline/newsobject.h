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

#ifndef __NEWS_OBJECT__
#define __NEWS_OBJECT__
/**
***             techidee GmbH
*** Projekt:    NewsBox
*** Autor:      Thomas Fruehwald
***
*** Compiler:   gcc
*** Modul:
        news service decoder implementation
***
***
**/
#ifdef _MSC_VER
# ifndef _WINSOCKAPI_
#  include <winsock2.h>
# endif
#else
# include <sys/time.h>
#endif

class NewsObject
{
public:
    NewsObject(unsigned long len, const unsigned char *buf,
               struct timeval *creation_time);
    ~NewsObject();

    typedef enum
    {
        MENU,
        PLAIN_TEXT,
        TITLE_ONLY,
        LIST
    } object_type_id_t;

    unsigned long getObjectId();
    struct timeval getReceptionTime();
    struct timeval getCreationTime();
    void setReceptionTime(struct timeval *time);
    bool isStatic();
    bool isCompressed();
    void setUpdateFlag();
    bool isUpdated();
    unsigned char getRevisionIndex();
    object_type_id_t getObjectType();
    void copyNml(unsigned long *len, unsigned char *nml);

private:
    enum { MAX_NML_BYTES=4092 };
    unsigned long       d_object_id;
    object_type_id_t    d_object_type;
    bool            d_static_flag;
    bool            d_compressed_flag;
    unsigned char       d_revision_index;

    unsigned long       d_len;
    unsigned char       d_nml[MAX_NML_BYTES];

    struct timeval      d_creation_time;
    struct timeval      d_reception_time;

    // that flag is set if an object with the same
    // object id existed previously.
    // It is needed to determine wether an object is
    // first received or updated.
    bool                d_obj_updated;

    object_type_id_t convertObjectType(unsigned char in);
};

#endif
