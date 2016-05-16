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
        dab datagroup decoder implementation
***
***
**/


#ifdef _MSC_VER
#define TIM_DEF
#include <wtypes.h>
#undef min

#pragma warning(disable: 4100)
#pragma warning(disable: 4786)

#endif

#include <algorithm>
#include <time.h>
#ifdef _WIN32
# include <windows.h> // for GetTickCount
#endif

#include "dabdatagroupdecoder.h"
#include "cpplog.h"
#include "newssvcdec_impl.h"

using std::find;

#ifdef __cplusplus
extern "C" {
#endif
int showDdNewsSvcDecInfo;
int showDdNewsSvcDecErr;
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C"
#endif
NEWS_SVC_DEC_decoder_t NEWS_SVC_DEC_createDec(
    NEWS_SVC_DEC_cb *update,
    unsigned long max_memory,
    unsigned long *max_objects,
    unsigned long extended_header_len,
    void    *arg
)
{
    NEWS_SVC_DEC_IMPL_t *dec=new NEWS_SVC_DEC_IMPL_t;
    if(dec==0)
    {
        if(showDdNewsSvcDecErr)
        {
            log_err << "out of memory requesting "
                    << sizeof(*dec) << " bytes" << endmsg;
        }
        return 0;
    }

    dec->d_magicId=NEWS_SVC_MAGIC_ID;
    dec->d_update=update;
    dec->d_arg=arg;
    dec->d_extended_header_len=extended_header_len;
    dec->d_max_memory=max_memory;

    unsigned long obj_possible=max_memory / sizeof(NewsObject);
    if(*max_objects==0)
    {
        *max_objects=obj_possible;
    }
    else
    {
        *max_objects=std::min(*max_objects, obj_possible);
    }

    if(showDdNewsSvcDecInfo)
    {
        log_info << "NEWS_SVC_DEC sizeof object: " << sizeof(NewsObject)
                 << "max memory: " << max_memory << " max_objects: " << *max_objects
                 << endmsg;
    }

    dec->d_max_objects=*max_objects;
    dec->d_object_count=0;

    dec->d_watch_list.clear();
    dec->d_keep_in_cache_list.clear();

    return (const NEWS_SVC_DEC_IMPL_t*)dec;
}


#ifdef __cplusplus
extern "C"
#endif
void NEWS_SVC_DEC_deleteDec(
    NEWS_SVC_DEC_decoder_t decoder
)
{
    NEWS_SVC_DEC_IMPL_t *dec=(NEWS_SVC_DEC_IMPL_t*)(decoder);
    if(dec->d_magicId!=NEWS_SVC_MAGIC_ID)
    {
        if(showDdNewsSvcDecErr)
        {
            log_err << "not a news service decoder" << endmsg;
        }
        return;
    }

    delete dec;
}

#ifdef __cplusplus
extern "C"
#endif
unsigned long NEWS_SVC_DEC_putData(
    NEWS_SVC_DEC_decoder_t  decoder,
    const unsigned long len,
    const unsigned char *buf
)
{
    if(showDdNewsSvcDecInfo)
    {
        log_info << "NEWS_SVC_DEC_putData" << endmsg;
    }

    NEWS_SVC_DEC_IMPL_t *dec=(NEWS_SVC_DEC_IMPL_t*)(decoder);
    if(dec->d_magicId!=NEWS_SVC_MAGIC_ID)
    {
        if(showDdNewsSvcDecErr)
        {
            log_err << "not a news service decoder" << endmsg;
        }
        return 0;
    }

    if(showDdNewsSvcDecInfo)
    {
        log_info << "d_object_count: " << dec->d_object_count <<
                 " d_max_objects: " << dec->d_max_objects << endmsg;
    }

    struct timeval reception_time;

#ifdef _WIN32
    time((time_t *)&(reception_time.tv_sec));
    reception_time.tv_usec=GetTickCount();
#else
    struct timezone tz;
    if(gettimeofday(&reception_time, &tz))
    {
        if(showDdNewsSvcDecErr)
        {
            log_err << "gettimeofday failed" << endmsg;
        }
    }
#endif

#if 0
    log_err << "received object at time:" << reception_time.tv_sec << ":"
            << reception_time.tv_usec << endmsg;
#endif

    // do not throw an exception when out of memory
    NewsObject *obj=new(std::nothrow)NewsObject(len, buf, &reception_time);
    if(obj==0)
    {
        if(showDdNewsSvcDecErr)
        {
            log_err << "out of memory requesting " << sizeof(*obj)
                    << " bytes" << endmsg;
        }
        return 0;
    }


    unsigned long object_id=obj->getObjectId();
    if(obj->isCompressed())
    {
        if(showDdNewsSvcDecInfo)
        {
            log_info << "object with id " << object_id << " is compressed"
                     << endmsg;
        }
    }
    if(showDdNewsSvcDecInfo)
    {
        log_info << "insert try got new object with id "
                 << std::hex << object_id << " revision index: " <<
                 static_cast<unsigned int>(obj->getRevisionIndex())
                 << endmsg;
    }
    if(dec->d_news_map.count(object_id)!=0)
    {
        if(showDdNewsSvcDecInfo)
        {
            log_info << "object already in map" << endmsg;
        }
        NewsObject *old=dec->d_news_map[object_id];
        if(old->getRevisionIndex()==obj->getRevisionIndex())
        {
            // adjust reception time
            old->setReceptionTime(&reception_time);
            delete obj;
            return 1;
        }
        else
        {
            if(showDdNewsSvcDecInfo)
            {
                log_info << "revision index changed" << endmsg;
            }
            // the object has a new revision index
            // so set the updated flag
            //
            obj->setUpdateFlag();
        }

        // have new object -> delete the old one
        //
        dec->d_object_count--;
        delete old;
    }

    if(dec->d_object_count >= dec->d_max_objects)
    {
        if(!NEWS_SVC_DEC_IMPL_garbage_collection(dec, 1))
        {
            if(showDdNewsSvcDecErr)
            {
                log_err << "garabage collection did not free memory" << endmsg;
            }
            return 0;
        }
    }


    // check if we are watching for it
    //
    if(dec->d_update)
    {
        NEWS_SVC_DEC_IMPL_watch_list_t &l=dec->d_watch_list;
        if(find(l.begin(), l.end(), object_id)!=l.end())
        {
            const unsigned long no_of_elem=1;
            NEWS_SVC_DEC_obj_availability_t chg_list[no_of_elem];
            chg_list[0].object_id=static_cast<unsigned short>(object_id);
            if(obj->isUpdated())
            {
                chg_list[0].status=NEWS_SVC_DEC_OBJ_UPDATED;
            }
            else
            {
                chg_list[0].status=NEWS_SVC_DEC_OBJ_RECEIVED;
            }

            dec->d_update(
                no_of_elem,
                chg_list,
                dec->d_arg);
        }
    }


    // insert into map
    if(showDdNewsSvcDecInfo)
    {
        log_info << "insert id: " << object_id << endmsg;
    }
    dec->d_news_map[object_id]=obj;
    dec->d_object_count++;
    return 1;
}

#ifdef __cplusplus
extern "C"
#endif
int NEWS_SVC_DEC_IMPL_garbage_collection(NEWS_SVC_DEC_IMPL_t *dec,
        unsigned long objs_to_free)
{
    for(unsigned long i=0; i<objs_to_free; ++i)
    {
        if(dec->d_news_map.empty())
        {
            if(showDdNewsSvcDecErr)
            {
                log_err << "not enough elements in map" << endmsg;
            }
            break;
        }

        // find the oldest element
        //
        if(showDdNewsSvcDecInfo)
        {
            NEWS_SVC_DEC_IMPL_printObjList(dec);
        }
        FindMinFunctor min(dec);
        NEWS_SVC_DEC_IMPL_map_t::iterator e=std::min_element(
                                                dec->d_news_map.begin(), dec->d_news_map.end(),
                                                min);
        unsigned long obj_id=(e->second)->getObjectId();
        NEWS_SVC_DEC_IMPL_keep_in_cache_list_t &l=dec->d_keep_in_cache_list;
        if(find(l.begin(), l.end(), obj_id)!=l.end())
        {
            if(showDdNewsSvcDecInfo)
            {
                log_info << "should keep all elements in cache" << endmsg;
            }
            return 0;
        }
        if(showDdNewsSvcDecInfo)
        {
            log_info << "erase object with id: " << std::hex <<
                     obj_id << endmsg;
        }

        if(dec->d_update)
        {
            NEWS_SVC_DEC_IMPL_watch_list_t &l=dec->d_watch_list;
            if(find(l.begin(), l.end(), obj_id)!=l.end())
            {
                const unsigned long no_of_elem=1;
                NEWS_SVC_DEC_obj_availability_t chg_list[no_of_elem];
                chg_list[0].object_id=static_cast<unsigned short>(obj_id);
                chg_list[0].status=NEWS_SVC_DEC_OBJ_REMOVED;

                dec->d_update(
                    no_of_elem,
                    chg_list,
                    dec->d_arg);
            }
        }

        dec->d_news_map.erase(e);
        --(dec->d_object_count);
    }
    return 1;
}

#ifdef __cplusplus
extern "C"
#endif
int NEWS_SVC_DEC_get_news_object(
    NEWS_SVC_DEC_decoder_t  decoder,
    unsigned short object_id,
    unsigned long *extended_header_len,
    unsigned long *len,
    unsigned char *nml)
{
    if(showDdNewsSvcDecInfo)
    {
        log_info << "NEWS_SVC_DEC_get_news_object object_id: " << object_id << endmsg;
    }
    NEWS_SVC_DEC_IMPL_t *dec=(NEWS_SVC_DEC_IMPL_t*)(decoder);
    if(dec->d_magicId!=NEWS_SVC_MAGIC_ID)
    {
        if(showDdNewsSvcDecErr)
        {
            log_err << "not a news service decoder" << endmsg;
        }
        return 0;
    }

    if(dec->d_news_map.count(object_id)==0)
    {
        return 0;
    }

    dec->d_news_map[object_id]->copyNml(len, nml);
    *extended_header_len=0;
    return 1;
}


#ifdef __cplusplus
extern "C"
#endif
int NEWS_SVC_DEC_watch_objects(
    NEWS_SVC_DEC_decoder_t  decoder,
    unsigned long number_of_elements,
    NEWS_SVC_DEC_obj_availability_t *watch_list)
{
    NEWS_SVC_DEC_IMPL_t *dec=(NEWS_SVC_DEC_IMPL_t*)(decoder);
    if(dec->d_magicId!=NEWS_SVC_MAGIC_ID)
    {
        if(showDdNewsSvcDecErr)
        {
            log_err << "not a news service decoder" << endmsg;
        }
        return 0;
    }

    if(showDdNewsSvcDecInfo)
    {
        log_info << "NEWS_SVC_DEC_watch_objects()" << endmsg;
    }

    dec->d_watch_list.clear();
    if(number_of_elements==0)
    {
        // just clear the watch list
        if(showDdNewsSvcDecInfo)
        {
            log_info << "NEWS_SVC_DEC_watch_objects(): clear watch list"
                     << endmsg;
        }

        return 1;
    }

    for(unsigned long i=0; i<number_of_elements; ++i)
    {
        if(showDdNewsSvcDecInfo)
        {
            log_info << "NEWS_SVC_DEC_watch_objects() add object " <<
                     watch_list[i].object_id << " to watch list" << endmsg;
        }

        dec->d_watch_list.push_back(watch_list[i].object_id);
    }

    if(!NEWS_SVC_DEC_get_object_availability(decoder,
            number_of_elements, watch_list))
    {
        log_err << "NEWS_SVC_DEC_get_object_availability failed"
                << endmsg;
        return 0;
    }
    return 1;
}

#ifdef __cplusplus
extern "C"
#endif
int NEWS_SVC_DEC_get_object_availability(
    NEWS_SVC_DEC_decoder_t  decoder,
    unsigned long number_of_elements,
    NEWS_SVC_DEC_obj_availability_t *query_list)
{
    NEWS_SVC_DEC_IMPL_t *dec=(NEWS_SVC_DEC_IMPL_t*)(decoder);
    if(dec->d_magicId!=NEWS_SVC_MAGIC_ID)
    {
        if(showDdNewsSvcDecErr)
        {
            log_err << "not a news service decoder" << endmsg;
        }
        return 0;
    }

    if(showDdNewsSvcDecInfo)
    {
        log_info << "NEWS_SVC_DEC_get_object_availability()" << endmsg;
    }
    for(unsigned long i=0; i<number_of_elements; ++i)
    {
        NEWS_SVC_DEC_obj_availability_t *e=query_list+i;
        if(dec->d_news_map.count(e->object_id)==0)
        {
            e->status=NEWS_SVC_DEC_OBJ_NOT_YET_AVAILABLE;
        }
        else
        {
            NewsObject &obj=*dec->d_news_map[e->object_id];
            if(obj.isUpdated())
            {
                e->status=NEWS_SVC_DEC_OBJ_UPDATED;
            }
            else
            {
                e->status=NEWS_SVC_DEC_OBJ_RECEIVED;
            }
        }

    }
    return 1;
}

#ifdef __cplusplus
extern "C"
#endif
int NEWS_SVC_DEC_keep_in_cache(
    NEWS_SVC_DEC_decoder_t  decoder,
    unsigned long number_of_elements,
    unsigned short *object_ids
)
{
    NEWS_SVC_DEC_IMPL_t *dec=(NEWS_SVC_DEC_IMPL_t*)(decoder);
    if(dec->d_magicId!=NEWS_SVC_MAGIC_ID)
    {
        if(showDdNewsSvcDecErr)
        {
            log_err << "not a news service decoder" << endmsg;
        }
        return 0;
    }

    if(showDdNewsSvcDecInfo)
    {
        log_info << "NEWS_SVC_DEC_keep_in_cache()" << endmsg;
    }

    if(number_of_elements > dec->d_max_objects)
    {
        log_err << "keep-in-cache list too big: " << number_of_elements
                << " max objects: " << dec->d_max_objects << endmsg;
        return 0;
    }

    dec->d_keep_in_cache_list.clear();
    if(number_of_elements==0)
    {
        // just clear the keep_in_cache list
        if(showDdNewsSvcDecInfo)
        {
            log_info << "NEWS_SVC_DEC_keep_in_cache(): clear keep-in-cache list"
                     << endmsg;
        }

        return 1;
    }

    for(unsigned long i=0; i<number_of_elements; ++i)
    {
        if(showDdNewsSvcDecInfo)
        {
            log_info << "NEWS_SVC_DEC_keep_in_cache() add object " <<
                     object_ids[i] << " to keep-in-cache list" << endmsg;
        }

        dec->d_keep_in_cache_list.push_back(object_ids[i]);
    }

    return 1;
}

void NEWS_SVC_DEC_IMPL_printObjList(NEWS_SVC_DEC_IMPL_t *dec)
{
    log_info << "print object map:" << std::endl << endmsg;
    NEWS_SVC_DEC_IMPL_map_t::iterator e;
    for(e=dec->d_news_map.begin(); e!=dec->d_news_map.end(); ++e)
    {
        log_info_nots << "object id: " << (*e).first << std::endl << endmsg;
    }
}

FindMinFunctor::FindMinFunctor(NEWS_SVC_DEC_IMPL_t *dec) :
    d_dec(dec)
{
}

FindMinFunctor::FindMinFunctor(const FindMinFunctor &rhs)
{
    d_dec=rhs.d_dec;
}

FindMinFunctor::~FindMinFunctor()
{
}

bool FindMinFunctor::operator()(
    std::pair<const unsigned long, NewsObject*> &rhs,
    std::pair<const unsigned long, NewsObject*> &lhs)
{
    NewsObject *no1=rhs.second;
    NewsObject *no2=lhs.second;

    unsigned long obj_id1=no1->getObjectId();
    unsigned long obj_id2=no2->getObjectId();
#if 1
    if(showDdNewsSvcDecInfo)
    {
        log_info << "compare " << obj_id1 << " with " << obj_id2 << endmsg;
    }
#endif

    NEWS_SVC_DEC_IMPL_keep_in_cache_list_t &l=d_dec->d_keep_in_cache_list;
    if(find(l.begin(), l.end(), obj_id1)!=l.end())
    {
        // special case root menu
        if(showDdNewsSvcDecInfo)
        {
            log_info << "don't erase id " << obj_id1 <<
                     " from keep in cache list" << endmsg;
        }
        return false;
    }
    if(find(l.begin(), l.end(), obj_id2)!=l.end())
    {
        // special case root menu
        if(showDdNewsSvcDecInfo)
        {
            log_info << "don't erase id " << obj_id2 <<
                     " from keep in cache list" << endmsg;
        }
        return true;
    }

    struct timeval t1=no1->getReceptionTime();
    struct timeval t2=no2->getReceptionTime();

    if(t1.tv_sec==t2.tv_sec)
    {
        return t1.tv_usec < t2.tv_usec;
    }

    return t1.tv_sec < t2.tv_sec;
}

