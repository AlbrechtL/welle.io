#
/*
 *
 *    Copyright (C) 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
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
#
#ifndef	DAB_SERIAL
#define	DAB_SERIAL

#include	<stdio.h>
#include	"dab-virtual.h"

class	dabProcessor;
class	uep_deconvolve;
class	eep_deconvolve;
class	audioSink;
class	RadioInterface;

class	dabSerial: public dabVirtual {
public:
	dabSerial	(uint8_t dabModus,
	                 int16_t fragmentSize,
	                 int16_t bitRate,
	                 int16_t uepFlag,
	                 int16_t protLevel,
	                 RadioInterface *mr,
	                 FILE	*,		// errorlog
	                 audioSink *as);
	~dabSerial	(void);
int32_t	process		(int16_t *, int16_t);
void	stopRunning	(void);
private:
	uint8_t	dabModus;
	int16_t	fragmentSize;
	int16_t	bitRate;
	int16_t	uepFlag;
	int16_t	protLevel;
	RadioInterface	*myRadioInterface;
	audioSink	*myAudioSink;
	int32_t	countforInterleaver;
	uint8_t		*outV;
	int16_t		**interleaveData;
	int16_t		*Data;

	uep_deconvolve	*uepProcessor;
	eep_deconvolve	*eepProcessor;
	dabProcessor	*our_dabProcessor;
};

#endif

