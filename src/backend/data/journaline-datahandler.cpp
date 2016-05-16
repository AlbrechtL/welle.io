#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the SDR-J (JSDR).
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include	"journaline-datahandler.h"
#include	"dabdatagroupdecoder.h"


static
void my_callBack (
    const DAB_DATAGROUP_DECODER_msc_datagroup_header_t *header,
    const unsigned long len,
    const unsigned char *buf,
    void *arg) {
}

	journaline_dataHandler::journaline_dataHandler (void) {
	theDecoder	= DAB_DATAGROUP_DECODER_createDec (my_callBack, this);
}

	journaline_dataHandler::~journaline_dataHandler (void) {
	DAB_DATAGROUP_DECODER_deleteDec (theDecoder);
}

void	journaline_dataHandler::add_mscDatagroup (QByteArray &msc) {
int16_t	len	= msc. length ();
uint8_t	*data	= (uint8_t *)(msc. data ());
uint8_t buffer [len / 8];
int16_t	i;
int32_t	res;
	for (i = 0; i < len / 8; i ++)
	   buffer [i] = getBits (data, 8 * i, 8);

	res = DAB_DATAGROUP_DECODER_putData (theDecoder, len / 8, buffer);
	if (res < 0)
	   return;
}


