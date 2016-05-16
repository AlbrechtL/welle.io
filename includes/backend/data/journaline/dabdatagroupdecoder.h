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

#ifndef __DAB_DATAGROUP_DECODER__
#define __DAB_DATAGROUP_DECODER__

/*!
 * @brief Journaline(R) news service DAB data group decoder interface
 *
 * @file dabdatagroupdecoder.h
 *
 *          techidee GmbH
 *
 * Project:   NewsBox
 *
 * Author:    Thomas Fruehwald
 *
 * Compiler:    gcc
 *
 * Module:    DAB data group decoder
 *
 * Creation date:  2003-08-02
 *
 * Last modified:  2001-2014-02-25 (rbr)
 *
 * This decoder will accept DAB data groups, check their validity and
 * pass on the valid (Journaline) data groups.
 *
 * To use it, you have to implement the callback for handling
 * the output in your application.
 * Then use createDec to create an instance,
 * putData to put DAB data groups into the decoder
 * and in the end, use deleteDec to destroy the instance.
 *
 * @attention
 * only the features needed for the news service "Journaline(R)"
 * are implemented in this DAB data group decoder, i.e.
 * - extension_flag=0 (no extension field will be processed)
 * - crc_flag=1 (Journaline data groups always have a CRC)
 * - segment_flag=0 (Journaline data groups are never segmented)
 * - user_access_flag=0 (no user access field will be processed)
 * - data_group_type=0 (only "general data" data groups are processed)
 *
 * @attention
 * The DAB data group decoder is not thread safe.
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

/*! @brief dab data group decoder instance type */
typedef const void*  DAB_DATAGROUP_DECODER_t;

/*!
  @brief MSC data group header
  @sa ETSI EN 300 401 V1.3.3 (2001-05) section 5.3.3.1
*/
typedef struct
{
    unsigned char  extension_flag;    /*!< extension flag (1 bit) */
    unsigned char  crc_flag;          /*!< CRC flag (1 bit) */
    unsigned char  segment_flag;      /*!< segment flag (1 bit) */
    unsigned char  user_access_flag;  /*!< user access flag (1 bit) */
    unsigned char  datagroup_type;    /*!< data group type (4 bit) */
    unsigned char  continuity_index;  /*!< continuity index (4 bit) */
    unsigned char  repetition_index;  /*!< repitition index (4 bit) */
    unsigned short extension_field;   /*!< extension field (16 bit), only
                                         present when extension flag is 1 */
} DAB_DATAGROUP_DECODER_msc_datagroup_header_t;


/*!
 * @brief DAB data group callback function
 *
 * A callback function of this type must be registered at creation time
 * of the DAB data group decoder instance.
 *
 * It will be called whenever a DAB data group
 * fed into the decoder using DAB_DATAGROUP_DECODER_putData
 * has been processed successfully, i.e. it is a valid
 * (Journaline specific) data group and can fed into the news service
 * decoder.
 *
 * @param header
 *   MSC data group header
 * @param len
 *   length in bytes of MSC data group data field
 * @param buf
 *   MSC data group data field
 * @param arg
 *   user specified data pointer (as specified in createDec)
 */
typedef void(DAB_DATAGROUP_DECODER_data)
(
    const DAB_DATAGROUP_DECODER_msc_datagroup_header_t *header,
    const unsigned long len,
    const unsigned char *buf,
    void *arg
);



/*******************************************
 * 1. object lifetime control              *
 *******************************************/

/*!
 * @brief Create a DAB data group decoder instance
 *
 * call this before anything else
 *
 * @param data
 *   DAB data group decoder callback function
 * @param arg
 *   user specified data pointer (will only be passed to callback)
 * @return DAB data group decoder instance
 */
DAB_DATAGROUP_DECODER_t DAB_DATAGROUP_DECODER_createDec(
    DAB_DATAGROUP_DECODER_data *data,
    void  *arg
);


/*!
 * @brief Delete a DAB data group decoder instance
 *
 * call this at shutdown time
 *
 * @param decoder
 *   DAB data group decoder instance
 */
void DAB_DATAGROUP_DECODER_deleteDec(const DAB_DATAGROUP_DECODER_t decoder);



/*******************************************
 * 2. data provision                       *
 *******************************************/

/*!
 * @brief Put data into DAB data group decoder
 *
 * The input for the DAB data group decoder consists of
 * complete DAB data groups.
 *
 * A CRC check is done if the CRC flag is set in the
 * data group header.
 *
 * @param decoder
 *   DAB data group decoder instance
 *   (as obtained by DAB_DATAGROUP_DECODER_createDec)
 * @param len
 *   length in bytes of MSC data group
 * @param buf
 *   DAB data group
 *
 * @retval 0  on failure
 * @retval 1  on success
 */
unsigned long DAB_DATAGROUP_DECODER_putData(
    const DAB_DATAGROUP_DECODER_t  decoder,
    const unsigned long  len,
    const unsigned char *buf
);


#ifdef __cplusplus
}
#endif

#endif /* __DAB_DATAGROUP_DECODER__ */
