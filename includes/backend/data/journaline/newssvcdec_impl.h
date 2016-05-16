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

#ifndef __NEWS_SVC_DEC_IMPL__
#define __NEWS_SVC_DEC_IMPL__
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
#include "newsobject.h"
#include "newssvcdec.h"


#ifdef _MSC_VER
#pragma warning(push,3)
#pragma warning(disable:4100)
#pragma warning(disable:4514)
#endif

#include <map>
#include <list>

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(disable:4100)
#endif


#ifdef __cplusplus
extern "C" {
#endif

extern int showDdNewsSvcDecInfo;
extern int showDdNewsSvcDecErr;

#define NEWS_SVC_MAGIC_ID   0x786245

typedef std::map<unsigned long, NewsObject*> NEWS_SVC_DEC_IMPL_map_t;
typedef std::list<unsigned short> NEWS_SVC_DEC_IMPL_watch_list_t;
typedef std::list<unsigned short> NEWS_SVC_DEC_IMPL_keep_in_cache_list_t;

typedef struct
{
    unsigned long   d_magicId;
    NEWS_SVC_DEC_cb *d_update;
    void            *d_arg;
    unsigned long   d_extended_header_len;
    unsigned long   d_max_memory;
    unsigned long   d_max_objects;
    unsigned long   d_object_count;

    NEWS_SVC_DEC_IMPL_map_t d_news_map;

    NEWS_SVC_DEC_IMPL_watch_list_t d_watch_list;
    NEWS_SVC_DEC_IMPL_keep_in_cache_list_t d_keep_in_cache_list;
} NEWS_SVC_DEC_IMPL_t;

int NEWS_SVC_DEC_IMPL_garbage_collection(NEWS_SVC_DEC_IMPL_t *dec,
        unsigned long objs_to_free);

#ifdef __cplusplus
}
#endif

/* internal functions */
void NEWS_SVC_DEC_IMPL_printObjList(NEWS_SVC_DEC_IMPL_t *dec);

class FindMinFunctor
{
public:
    FindMinFunctor(NEWS_SVC_DEC_IMPL_t *dec);
    FindMinFunctor(const FindMinFunctor &rhs);
    ~FindMinFunctor();

    bool operator()(
        std::pair<const unsigned long, NewsObject*> &rhs,
        std::pair<const unsigned long, NewsObject*> &lhs);

private:
    NEWS_SVC_DEC_IMPL_t *d_dec;
};


#endif /* __NEWS_SVC_DEC_IMPL */
