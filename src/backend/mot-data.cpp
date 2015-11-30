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
#include	"mot-data.h"
#include	"gui.h"

		motHandler::motHandler (RadioInterface *mr) {
int16_t	i, j;

	for (i = 0; i < 16; i ++) {
	   table [i]. ordernumber = -1;
	   for (j = 0; j < 100; j ++)
	      table [i]. marked [j] = false;
	}
	ordernumber	= 1;
	connect (this, SIGNAL (pictureReady (QByteArray)),
	         mr, SLOT (showMOT (QByteArray)));
}

	 	motHandler::~motHandler (void) {
}

void		motHandler::processHeader (int16_t	transportId,
	                                   uint8_t	*segment,
	                                   int16_t	segmentSize,
	                                   int16_t	headerSize,
	                                   int32_t	bodySize,
	                                   bool		lastFlag) {
	   uint8_t contentType	= ((segment [5] >> 1) & 0x3F);
	   uint8_t contentsubType = ((segment [5] & 0x01) << 8) | segment [6];
//	   fprintf (stderr, "transId %d, segS %d bodyS %d header %d type %d subt %d lastFlag %d\n",
//	                 transportId,
//	                 segmentSize,
//	                 bodySize,
//	                 headerSize,
//	                 contentType,
//	                 contentsubType,
//	                 lastFlag);
int16_t	pointer	= 7;

	while (pointer < headerSize) {
	   uint8_t PLI = (segment [pointer] & 0300) >> 6;
	   uint8_t paramId = (segment [pointer] & 077);
	   uint16_t	length;
//	   fprintf (stderr, "PLI = %d, paramId = %d\n", PLI, paramId);
	   switch (PLI) {
	      case 00:
	         pointer += 1;
	         break;
	      case 01:
//	         if (paramId == 10)
//	            fprintf (stderr, "priority = %d\n",
//	                              segment [pointer + 1]);
	      
	         pointer += 2;
	         break;
	      case 02:
//	         if (paramId == 5) 
//	            fprintf (stderr, "triggertime = %d\n",
//	                             segment [pointer + 1] << 24 |
//	                             segment [pointer + 2] << 16 |
//	                             segment [pointer + 3] <<  8 |
//	                             segment [pointer + 4]);
	         pointer += 5;
	         break;
	      case 03:
	         if ((segment [pointer + 1] & 0200) != 0) {
	            length = (segment [pointer + 1] & 0177) << 8 |
	                      segment [pointer + 2];
	            pointer += 3;
	         }
	         else {
	            length = segment [pointer + 1] & 0177;
	            pointer += 2;
	         }
	         if (paramId == 12) {
	            char temp [length];
	            int16_t i;
	            fprintf (stderr, "charset = %d\n",
	                                (segment [pointer] & 0xFF00) >> 8);
	            for (i = 0; i < length - 1; i ++) 
	               temp [i] = segment [pointer + i + 1];
	            temp [length - 1] = '\0';
	            fprintf (stderr, "Name is %s\n", temp);
	         }
	         pointer += length;
	   } 
	}
	if (getHandle (transportId) != NULL)
	   return;
	if (lastFlag)	{ // single header
	   newEntry (transportId, bodySize, contentType, contentsubType);
	   return;
	}
//	header segment contains header + segment data
	newEntry (transportId,
	          bodySize - headerSize,
	          contentType,
	          contentsubType);
	processSegment (transportId, 
	                &segment [headerSize],
	                0,
	                segmentSize - headerSize,
	                false);
}

void		motHandler::processSegment	(int16_t	transportId,
	                                 uint8_t	*segment,
	                                 int16_t	segmentNumber,
	                                 int16_t	segmentSize,
	                                 bool		lastFlag) {
int16_t	i;
//	   fprintf (stderr, "transportId %d segment number %d (size %d) (flags = %d)\n",
//	                     transportId,
//	                     segmentNumber,
//	                     segmentSize, lastFlag);

	motElement *handle = getHandle (transportId);
	if (handle == NULL)
	   return;
	if (handle -> marked [segmentNumber])
	   return;
//	Note that the last segment may have a different size
	if (!lastFlag && (handle -> segmentSize == -1))
	   handle -> segmentSize = segmentSize;
	if (handle -> segmentSize != -1) {
//	sanity check
	   if (segmentNumber * handle -> segmentSize + segmentSize >
	                                handle -> bodySize)
	      return;
	   for (i = 0; i < segmentSize; i ++)
	      handle -> body [segmentNumber *
	                                  handle -> segmentSize + i] =
	           segment [i];
	   handle -> marked [segmentNumber] = true;
	   fprintf (stderr, "%d -> segment %d set\n", transportId, segmentNumber);
	   if (lastFlag) {
	      handle -> numofSegments = segmentNumber;
	      fprintf (stderr, "aantal segmenten = %d\n", segmentNumber);
	   }
	   if (isComplete (handle)) {
	      fprintf (stderr, "slide %d is complete\n", transportId);
	      handleComplete (handle);
	   }
	}
}

void	motHandler::handleComplete (motElement *p) {
	if (p -> contentType != 2)
	   return;
	pictureReady (p -> body);
}

bool	motHandler::isComplete (motElement *p) {
int16_t	i;

	if (p -> numofSegments == -1)
	   return false;
	for (i = 0; i < p ->  numofSegments; i ++)
	   if (!p -> marked [i])
	      return false;

	return true;
}

motElement	*motHandler::getHandle (uint16_t transportId) {
int16_t	i;

	for (i = 0; i < 16; i ++)
	   if (table [i]. ordernumber != -1 && table [i]. transportId == transportId)
	      return &table [i];
	return NULL;
}

void	motHandler::newEntry (uint16_t	transportId,
	                      int16_t	size,
	                      int16_t	contentType,
	                      int16_t	contentsubType) {
int16_t		i;
uint16_t	lowest;
int16_t		lowIndex;

	for (i = 0; i < 16; i ++) {
	   if (table [i]. ordernumber == -1) {
	      table [i]. ordernumber	= ordernumber ++;
	      table [i]. transportId	= transportId;
	      table [i]. body. resize (size);
	      table [i]. bodySize	= size;
	      table [i]. contentType	= contentType;
	      table [i]. contentsubType	= contentsubType;
	      table [i]. segmentSize	= -1;
	      table [i]. numofSegments	= -1;
	      return;
	   }
	}
//
//	table full, delete the oldest one
//
	lowest		= 65377;
	lowIndex	= -1;
	for (i = 0; i < 16; i ++) {
	   if (table [i]. ordernumber < lowest) {
	      lowIndex = i;
	      lowest = table [i]. ordernumber;
	   }
	}
	
	table [i]. ordernumber	= ordernumber ++;
	table [i]. transportId	= transportId;
	table [i]. body. resize (size);
	table [i]. bodySize	= size;
	table [i]. contentType	= contentType;
	table [i]. contentsubType	= contentsubType;
	table [i]. segmentSize	= -1;
	table [i]. numofSegments	= -1;
}

