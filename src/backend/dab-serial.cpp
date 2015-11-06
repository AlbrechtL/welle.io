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
#include	"dab-constants.h"
#include	"dab-serial.h"
#include	"mp2processor.h"
#include	"mp4processor.h"
#include	"deconvolve.h"
#include	"audiosink.h"
#include	"gui.h"

//
//	Interleaving is - for reasons of simplicity - done
//	inline rather than through a special class-object
static
int8_t	interleaveDelays [] = {
	     15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0};
//
//
//	fragmentsize == Length * CUSize
	dabSerial::dabSerial	(uint8_t dabModus,
	                         int16_t fragmentSize,
	                         int16_t bitRate,
	                         int16_t uepFlag,
	                         int16_t protLevel,
	                         RadioInterface *mr,
	                         FILE	*mp2file,
	                         FILE	*mp4file,
	                         FILE	*errorLog,
	                         audioSink *as) {
int32_t i, j;
	this	-> dabModus		= dabModus;
	this	-> fragmentSize		= fragmentSize;
	this	-> bitRate		= bitRate;
	this	-> uepFlag		= uepFlag;
	this	-> protLevel		= protLevel;
	this	-> myRadioInterface	= mr;
	this	-> mp2File		= mp2file;
	this	-> mp4File		= mp4file;
	this	-> myAudioSink		= as;
	outV			= new uint8_t [bitRate * 24];
	interleaveData		= new int16_t *[fragmentSize]; // the size
	for (i = 0; i < fragmentSize; i ++) {
	   interleaveData [i] = new int16_t [16];
	   for (j = 0; j < 16; j ++)
	      interleaveData [i][j] = 0;
	}
	countforInterleaver	= 0;

	uepProcessor		= NULL;
	eepProcessor		= NULL;
	if (uepFlag == 0)
	   uepProcessor	= new uep_deconvolve (bitRate,
	                                      protLevel);
	else
	   eepProcessor	= new eep_deconvolve (bitRate,
	                                      protLevel);
//
	
	if (dabModus == DAB) 
	   our_dabProcessor = new mp2Processor (myRadioInterface,
	                                        myAudioSink,
	                                        mp2File,
	                                        bitRate);
	else
	if (dabModus == DAB_PLUS) 
	   our_dabProcessor = new mp4Processor (myRadioInterface,
	                                        myAudioSink,
	                                        errorLog,
	                                        bitRate);
	else		// cannot happen
	   our_dabProcessor = new dabProcessor ();

	myAudioSink	-> restart	();
}

	dabSerial::~dabSerial	(void) {
int16_t	i;
//	myAudioSink	-> stop ();
	if (uepProcessor != NULL)
	   delete uepProcessor;
	if (eepProcessor != NULL)
	   delete eepProcessor;
	delete our_dabProcessor;
	delete[]	outV;
	for (i = 0; i < fragmentSize; i ++) 
	   delete[]  interleaveData [i];
	delete[] interleaveData;
}

int32_t	dabSerial::process	(int16_t *Data, int16_t cnt) {
int16_t	i, j;
uint8_t	shiftRegister [9];
//
//	first: interleaving
	for (i = 0; i < fragmentSize; i ++) {
	   interleaveData [i][interleaveDelays [i & 017]] = Data [i];
	   Data [i] = interleaveData [i] [0];
//	and shift
	   memmove (&interleaveData [i][0],
	            &interleaveData [i][1],
	            interleaveDelays [i & 017] * sizeof (int16_t));
	}
//
//	only continue when de-interleaver is filled
	if (countforInterleaver < 15) {
	   ++countforInterleaver;
	   return 32768;
	}
//
	if (uepFlag == 0)
	   uepProcessor -> deconvolve (Data, fragmentSize, outV);
	else
	   eepProcessor -> deconvolve (Data, fragmentSize, outV);
//
//	and the inline energy dispersal
//	and for now, write out
	memset (shiftRegister, 1, 9);
	for (i = 0; i < bitRate * 24; i ++) {
	   uint8_t b = shiftRegister [8] ^ shiftRegister [4];
	   for (j = 8; j > 0; j--)
	      shiftRegister [j] = shiftRegister [j - 1];
	   shiftRegister [0] = b;
	   outV [i] ^= b;
	}

	our_dabProcessor	-> addtoFrame (outV, 24 * bitRate);
	return 32768;
}
//
void	dabSerial::stopRunning (void) {
//	myAudioSink	-> stop ();
}

void	dabSerial::setFiles	(FILE *f1, FILE *f2) {

	mp2File		= f1;
	if (dabModus == DAB)
	   our_dabProcessor	-> setFile (f1);
	(void)f2;
}

