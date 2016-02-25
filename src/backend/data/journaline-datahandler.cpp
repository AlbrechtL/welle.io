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

	journaline_dataHandler::journaline_dataHandler (void) {
}

	journaline_dataHandler::~journaline_dataHandler (void) {
}

void	journaline_dataHandler::add_mscDatagroup (QByteArray &msc) {
uint8_t *data		= (uint8_t *)(msc. data ());
bool	extensionFlag	= getBits_1 (data, 0) != 0;
bool	crcFlag		= getBits_1 (data, 1) != 0;
bool	segmentFlag	= getBits_1 (data, 2) != 0;
bool	userAccessFlag	= getBits_1 (data, 3) != 0;
uint8_t	groupType	= getBits_4 (data, 4);
uint8_t	CI		= getBits_4 (data, 8);
int16_t	next		= 16;		// bits
bool	lastSegment	= false;
uint16_t segmentNumber	= 0;
bool transportIdFlag	= false;
uint16_t transportId	= 0;
uint8_t	lengthInd;
int16_t	i;

	(void)CI;
	if (msc. size () == 0)
	   return;

	if (crcFlag && !check_CRC_bits (data, msc.size ())) 
	   return;

	if (extensionFlag)
	   next += 16;

	if (segmentFlag) {
	   lastSegment	= getBits_1 (data, next) != 0;
	   segmentNumber = getBits (data, next + 1, 15);
	   next += 16;
	}

	if (userAccessFlag) {
	   transportIdFlag	= getBits_1 (data, next + 3);
	   lengthInd		= getBits_4 (data, next + 4);
	   next	+= 8;
	   if (transportIdFlag) {
	      transportId = getBits (data, next, 16);
	   }
	   next	+= lengthInd * 8;
	}

	uint16_t	ipLength	= 0;
	int16_t		sizeinBits	=
	              msc. size () - next - (crcFlag != 0 ? 16 : 0);

	if (transportIdFlag) {
	   QByteArray journalineVector;
	   journalineVector. resize (sizeinBits / 8);
	   for (i = 0; i < sizeinBits / 8; i ++)
	      journalineVector [i] = getBits_8 (data, next + 8 * i);

	   processJournaline (journalineVector,
	                      groupType,
	                      lastSegment,
	                      segmentNumber,
	                      transportId);
	}
}

//
//	Journaline is handled in a separate class, here
//	we merely collect the data
void	journaline_dataHandler::processJournaline (QByteArray	&d,
	                                           uint8_t	groupType,
	                                           bool		lastSegment,
	                                           int16_t	segmentNumber,
	                                           uint16_t	transportId)  {
}

