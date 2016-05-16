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

/**
***             techidee GmbH
*** Projekt:    NewsBox
*** Autor:      Thomas Fruehwald
***
*** Compiler:   gcc
*** Modul:
        news object
***
***
**/
#include "newsobject.h"
#include "cpplog.h"
#include <cstdlib>
#include <cstring>

NewsObject::NewsObject(unsigned long len, const unsigned char *buf,
                       struct timeval *creation_time) :
    d_creation_time(*creation_time),
    d_reception_time(*creation_time),
    d_obj_updated(false)
{
    d_object_id=(buf[0]<<8)+buf[1];
#if 0
    log_err << "insert id: " << d_object_id << "buf[0]: " <<
            static_cast<unsigned int>(buf[0]) << "buf[1]: " <<
            static_cast<unsigned int>(buf[1]) << endmsg;
#endif

    d_object_type=convertObjectType(static_cast<unsigned char>((buf[2]&0xE0)>>5));
    d_static_flag=((buf[2]&0x10)!=0);
    d_compressed_flag=((buf[2]&0x08)!=0);
    d_revision_index=static_cast<unsigned char>(buf[2]&0x07);

    if(len > MAX_NML_BYTES)
    {
        log_err << "length of nml too big:" << len << endmsg;
    }
    d_len=len;
    memcpy(d_nml, buf, len);
}

NewsObject::~NewsObject()
{
}

unsigned long NewsObject::getObjectId()
{
    return d_object_id;
}

struct timeval NewsObject::getReceptionTime()
{
    return d_reception_time;
}

struct timeval NewsObject::getCreationTime()
{
    return d_creation_time;
}

void NewsObject::setReceptionTime(struct timeval *time)
{
    d_reception_time=*time;
}

bool NewsObject::isStatic()
{
    return d_static_flag;
}

bool NewsObject::isCompressed()
{
    return d_compressed_flag;
}

bool NewsObject::isUpdated()
{
    return d_obj_updated;
}

void NewsObject::setUpdateFlag()
{
    d_obj_updated=true;
}

unsigned char NewsObject::getRevisionIndex()
{
    return d_revision_index;
}

NewsObject::object_type_id_t NewsObject::getObjectType()
{
    return d_object_type;
}

NewsObject::object_type_id_t NewsObject::convertObjectType(unsigned char in)
{
    switch(in)
    {
    case 0x01:
        return MENU;

    case 0x02:
        return PLAIN_TEXT;

    case 0x03:
        return TITLE_ONLY;

    case 0x04:
        return LIST;

    default:
        log_err << "unknown object type: " <<
                std::hex << static_cast<unsigned int>(in) << endmsg;
    }
    return PLAIN_TEXT;
}

void NewsObject::copyNml(unsigned long *len, unsigned char *nml)
{
    memcpy(nml, d_nml, d_len);
    *len=d_len;
}

