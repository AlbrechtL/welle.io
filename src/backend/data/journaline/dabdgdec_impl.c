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
*** 			techidee GmbH
*** Projekt:	NewsBox
*** Autor:		Thomas Fruehwald
***
*** Compiler:	gcc
*** Modul:
		dab datagroup decoder implementation
***
***
**/
#ifdef _MSC_VER
# include <malloc.h>
# include <memory.h>
#else
# include <stdlib.h>
#endif

#include "dabdatagroupdecoder.h"
#include "dabdgdec_impl.h"
#include "log.h"
#include "crc_8_16.h"

#ifdef __cplusplus
extern "C"
#endif

int showDdDabDgDecInfo;
int showDdDabDgDecErr;

DAB_DATAGROUP_DECODER_t DAB_DATAGROUP_DECODER_createDec(
	DAB_DATAGROUP_DECODER_data *data,
	void	*arg
	)
{
	DAB_DGDEC_IMPL_t *dec=(DAB_DGDEC_IMPL_t*)malloc(
		sizeof(DAB_DGDEC_IMPL_t));
	if(dec==0)
	{
		if(showDdDabDgDecErr)
		{
			logit(LOG_ERR, "out of memory requesting %d bytes",
				sizeof(*dec));
		}
		return 0;
	}

	dec->magicId=DAB_DGDEC_MAGIC_ID;
	dec->cb=data;
	dec->arg=arg;
	return (const DAB_DATAGROUP_DECODER_t)dec;
}

#ifdef __cplusplus
extern "C"
#endif
void DAB_DATAGROUP_DECODER_deleteDec
	(const DAB_DATAGROUP_DECODER_t decoder)
{
	DAB_DGDEC_IMPL_t *dec=(DAB_DGDEC_IMPL_t*)decoder;
	if(dec==0)
	{
		if(showDdDabDgDecErr)
		{
			logit(LOG_ERR, "invalid parameter");
		}
		return;
	}

	if(dec->magicId!=DAB_DGDEC_MAGIC_ID)
	{
		if(showDdDabDgDecErr)
		{
			logit(LOG_ERR, "invalid parameter: not a datagroup decoder");
		}
		return;
	}

	free(dec);
}

#ifdef __cplusplus
extern "C"
#endif
unsigned long DAB_DATAGROUP_DECODER_putData(
		const DAB_DATAGROUP_DECODER_t	decoder,
		const unsigned long	len,
		const unsigned char *buf
	)
{
	DAB_DGDEC_IMPL_t *dec=(DAB_DGDEC_IMPL_t*)decoder;

	DAB_DATAGROUP_DECODER_msc_datagroup_header_t header;
	unsigned long session_header_len=0;	/* we expect segment flag to be 0 */
	unsigned long header_len;
	const unsigned char *data_field;
	unsigned long data_len;
	unsigned short crc_field;
	unsigned char c;
 

#if 0
	logit(LOG_ERR_DUMP, "data:", buf, len);
#endif

	if(dec->magicId!=DAB_DGDEC_MAGIC_ID)
	{
		if(showDdDabDgDecErr)
		{
			logit(LOG_ERR, "invalid parameter: not a datagroup decoder");
		}
		return 0;
	}

	if(!DAB_DGDEC_IMPL_extractMscDatagroupHeader(len, buf, &header))
	{
		if(showDdDabDgDecErr)
		{
			logit(LOG_ERR, "invalid datagroup header");
		}
		return 0;
	}

	if(showDdDabDgDecInfo)
	{
		DAB_DGDEC_IMPL_showMscDatagroupHeader(&header);
	}

	if(header.segment_flag)
	{
		if(showDdDabDgDecErr)
		{
			logit(LOG_ERR, "decoder does not work with segement_flag");
		}
		return 0;
	}

	header_len=2+header.extension_flag*2+session_header_len;
	data_field=buf+header_len;
	data_len=len-header_len;

	if(data_len==0)
	{
		if(showDdDabDgDecErr)
		{
			logit(LOG_ERR, "length of datafield is zero!!!");
		}
		return 0;
	}

	if(header.crc_flag)
	{
		if(data_len<2)
		{
			if(showDdDabDgDecErr)
			{
				logit(LOG_ERR, "length of datafield is wrong!!!");
			}
			return 0;
		}
		data_len-=2;

		c=buf[len-2];
		crc_field=(unsigned short)(c<<8);
		c=buf[len-1];
		crc_field=(unsigned short)(crc_field+c);
		if(!DAB_DGDEC_IMPL_checkCrc(buf, len-2, crc_field))
		{
			if(showDdDabDgDecErr)
			{
				logit(LOG_ERR, "crc error");
			}
			return 0;
		}

    if (header.datagroup_type!=0)
    {
			if(showDdDabDgDecInfo)
			{
				logit(LOG_INFO, "unexpected type %d", header.datagroup_type);
			}
      return 0;
    }
	}
	
	dec->cb(&header, data_len, data_field, dec->arg);
	return 1;
}

int DAB_DGDEC_IMPL_checkCrc(const unsigned char *buf,
	unsigned long len, unsigned short crc_field)
{
  return CRC_Check_16(buf, len, crc_field);
}

int DAB_DGDEC_IMPL_extractMscDatagroupHeader(
	unsigned long len,
	const unsigned char *buf,
	DAB_DATAGROUP_DECODER_msc_datagroup_header_t *header)
{
	unsigned char c;
	if(len < 2)
	{
		if(showDdDabDgDecErr)
		{
			logit(LOG_ERR, "not enough data");
		}
		return 0;
	}

	c=buf[0];
	header->extension_flag=(unsigned char)((c&0x80)!=0);
	header->crc_flag=(unsigned char)((c&0x40)!=0);
	header->segment_flag=(unsigned char)((c&0x20)!=0);
	header->user_access_flag=(unsigned char)((c&0x10)!=0);
	header->datagroup_type=(unsigned char)(c&0x0F);

	c=buf[1];
	header->continuity_index=(unsigned char)((c&0xF0)>>4);
	header->repetition_index=(unsigned char)(c&0x0F);

	if(header->extension_flag)
	{
		if(len<4)
		{
			if(showDdDabDgDecErr)
			{
				logit(LOG_ERR, "not enough data");
			}
			return 0;
		}

		c=buf[2];
		header->extension_field=(unsigned short)(c<<8);
		c=buf[3];
    /* type behaviour of vc6.0 is not conformant to */
    /* header->extension_field+=(unsigned short)c; */
    /* Therefore the uncommon syntax is used here. */
		header->extension_field=(unsigned short)(header->extension_field+c);
	}
	else
	{
		header->extension_field=0xFFFF;
	}
	
	return 1;
}

void DAB_DGDEC_IMPL_showMscDatagroupHeader(
	DAB_DATAGROUP_DECODER_msc_datagroup_header_t *header)
{
	logit(LOG_INFO,
		"extension_flag: %d\n"
		"crc_flag: %d\n"
		"segment_flag: %d\n"
		"user_access_flag: %d\n"
		"datagroup_type: 0x%x\n"
		"continuity_index: 0x%x\n"
		"repetition_index: 0x%x\n"
		"extension_field: 0x%x\n",

		header->extension_flag,
		header->crc_flag,
		header->segment_flag,
		header->user_access_flag,
		header->datagroup_type,
		header->continuity_index,
		header->repetition_index,
		header->extension_field
	);
}
