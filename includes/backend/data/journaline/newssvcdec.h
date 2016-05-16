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

#ifndef __NEWS_SVC_DEC__
#define __NEWS_SVC_DEC__

/*!
 * @brief Journaline(R) news service decoder interface
 *
 * @file newssvcdec.h
 *
 *         techidee GmbH
 *
 * Projekt:    NewsBox
 *
 * Autor:      Thomas Fruehwald
 *
 * Compiler:    gcc
 *
 * Modul: news service decoder
 *
 * Creation date:  2003-08-02
 *
 * Last modified:  2001-2014-02-16 (rbr)
 *
 * The Journaline(R) news service decoder will accept
 * Journaline(R) objects conforming to the
 * news service "Journaline(R)" specification and store them
 * (subject to specified memory limits) for later retrieval by
 * a Journaline(R) application.
 *
 * To use it, call createDec to create an instance,
 * putData to put Journaline(R) objects into the decoder
 * and in the end, use deleteDec to destroy the instance.
 *
 * To make use of the watch functionality for monitoring object availability
 * information, you must implement the object availability callback and pass
 * it to createDec.
 *
 * @attention
 * The Journaline(R) news service decoder is not thread safe.
 */

#ifdef __cplusplus
extern "C" {
#endif


/*! @brief news service decoder instance type */
typedef const void* NEWS_SVC_DEC_decoder_t;


/*! @brief object availability status type */
typedef enum
{
    NEWS_SVC_DEC_OBJ_NOT_YET_AVAILABLE, /*!< object is not yet available */
    NEWS_SVC_DEC_OBJ_REMOVED,           /*!< object has been removed because
                                           of memory restrictions */
    NEWS_SVC_DEC_OBJ_RECEIVED,       /*!< object has been received */
    NEWS_SVC_DEC_OBJ_UPDATED         /*!< update of already received object */
} NEWS_SVC_DEC_obj_availability_status_t;

/*! @brief object availability type */
typedef struct
{
    unsigned short object_id;                      /*!< NML object id */
    NEWS_SVC_DEC_obj_availability_status_t status; /*!< current availability
                                                      status */
} NEWS_SVC_DEC_obj_availability_t;


/*!
 * @brief object availability callback function
 *
 * The object availability callback function will be called when
 * the object availability status of one or more of the objects
 * in the watch list changes.
 *
 * @param number_of_elements
 *   number of elements in change list
 * @param chg_list
 *   change list
 * @param arg
 *   user specified data pointer (as specified in createDec)
 */
typedef void(NEWS_SVC_DEC_cb)(
    unsigned long number_of_elements,
    NEWS_SVC_DEC_obj_availability_t *chg_list,
    void *arg
);




/****************************************************************************
 *
 * public function interface
 *
 ****************************************************************************/


/*******************************************
 * 1. object lifetime control              *
 *******************************************/

/*!
 * @brief Create a news service decoder instance
 *
 * call this before anything else
 *
 * @param update
 *   callback function (may be 0 to indicate watching is not used)
 * @param max_memory
 *   value in bytes for the news-service
 *   the value is truncated to the nearest multiple
 *   of the (internal) news object representation
 *   approx. 2200 bytes (4092 bytes is the longest nml)
 * @param max_objects
 *   maximum number of objects.
 *   if 0 the whole max_news_service_memory is used.
 *   otherwise the sharper restriction limits the
 *   object count (e.g. max_news_service_memory -> 10
 *   objects, max_object_count=5 -> 5 objects)
 * @param extended_header_len
 *   length of NML extended header in bytes (signalled in SDC)
 * @param arg
 *   user specified data pointer (will only be passed to callback)
 *
 * @return created news service decoder instance
 * @retval 0 on failure
 */
NEWS_SVC_DEC_decoder_t NEWS_SVC_DEC_createDec(
    NEWS_SVC_DEC_cb update,
    unsigned long max_memory,
    unsigned long *max_objects,
    unsigned long extended_header_len,
    void  *arg
);

/*!
 * @brief Delete a news service decoder instance
 *
 * call this at shutdown time
 *
 * @param decoder
 *   news service decoder instance
 *   (as obtained by NEWS_SVC_DEC_createDec)
 *
 * @retval 0 on failure
 * @retval 1 on success
 */
void NEWS_SVC_DEC_deleteDec(
    NEWS_SVC_DEC_decoder_t decoder
);


/*******************************************
 * 2. news service pull functions          *
 *******************************************/

/*!
 * @brief Get a news object by id
 *
 * The object with id object_id will be written to nml
 * and len will be set to its actual length in bytes (at most 4092).
 *
 * @param decoder
 *   news service decoder instance (as obtained by NEWS_SVC_DEC_createDec)
 * @param object_id
 *   NML object id (0000-FFFF)
 * @param extended_header_len (out)
 *   length in bytes of extended header field of this news service
 *   (as signalled in SDC)
 * @param len (out)
 *   length in bytes of NML object
 * @param nml (out)
 *   the complete NML object (including NML header)
 *
 * @retval 0  on failure
 */
int NEWS_SVC_DEC_get_news_object(
    NEWS_SVC_DEC_decoder_t decoder,
    unsigned short object_id,
    unsigned long *extended_header_len,
    unsigned long *len,
    unsigned char *nml
);


/*!
 * @brief Query the availability of objects
 *
 * Specify a list of objects (only the object ids have to be filled in)
 * you want to query in query_list.
 *
 * on return the status field of query_list will be set.
 *
 * @param decoder
 *   news service decoder instance (as obtained by NEWS_SVC_DEC_createDec)
 * @param number_of_elements
 *   length of SVCMGR_UI_object_availability_t array
 * @param query_list
 *   (inout) list of objects for which the status is queried
 *
 * @retval 0  on failure
 */
int NEWS_SVC_DEC_get_object_availability(
    NEWS_SVC_DEC_decoder_t decoder,
    unsigned long number_of_elements,
    NEWS_SVC_DEC_obj_availability_t *query_list
);


/*******************************************
 * 3. data provision                       *
 *******************************************/

/*!
 * @brief Put data into news decoder
 *
 * The input for the news decoder consists of one complete Journaline(R) object.
 *
 * @param decoder
 *   news service decoder instance (as obtained by NEWS_SVC_DEC_createDec)
 * @param len
 *   length in bytes of Journaline(R) object
 * @param buf
 *   Journaline(R) object
 *
 * @retval 0  on failure
 */
unsigned long NEWS_SVC_DEC_putData(
    NEWS_SVC_DEC_decoder_t  decoder,
    const unsigned long  len,
    const unsigned char *buf
);


/*******************************************
 * 4. watch and cache management           *
 *******************************************/

/*!
 * @brief Set a watch on the specified objects
 *
 * When the status of an object with an id
 * that is being watched on changes, a callback is done.
 * To avoid nested upcalls the status at the calling
 * time is returned in the watch_list's status field.
 *
 * @note Every call to this function overwrites the last watch list.
 *
 * @param number_of_elements
 *   length of SVCMGR_UI_object_availability_t array
 * @param decoder
 *   news service decoder instance (as obtained by
 *   NEWS_SVC_DEC_createDec)
 * @param watch_list
 *   (inout) list of objects for which a watch is set.
 *
 *   On input, the object_id field has to be specified.
 *
 *   On output, the status field will be filled.
 *
 * @retval 0  on failure
 */
int NEWS_SVC_DEC_watch_objects(
    NEWS_SVC_DEC_decoder_t decoder,
    unsigned long number_of_elements,
    NEWS_SVC_DEC_obj_availability_t *watch_list
);

/*!
 * @brief Set ids of objects to be kept in cache
 *
 * Tell the memory management that the listed elements
 * should be kept in memory (if possible).
 *
 * Candidates for keeping in memory are (in order of importance):
 *   -# currently viewed object
 *   -# parents of currently viewed object
 *   -# children of currently viewed (menu) object
 *   -# "favorite" objects
 *
 * @note Every call to this function overwrites the last keep-in-memory list.
 *
 * @param decoder
 *   news service decoder instance (as obtained by NEWS_SVC_DEC_createDec)
 * @param number_of_elements
 *   length of object_id array
 * @param object_ids
 *   list of ids of objects which should be kept in cache
 *
 * @retval 0  on failure
 */
int NEWS_SVC_DEC_keep_in_cache(
    NEWS_SVC_DEC_decoder_t decoder,
    unsigned long number_of_elements,
    unsigned short *object_ids
);

#ifdef __cplusplus
}
#endif

#endif /* __NEWS_SVC_DEC__ */
