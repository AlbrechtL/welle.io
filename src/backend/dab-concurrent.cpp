#
/*
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
#include	"dab-concurrent.h"
#include	<QThread>
#include	<QMutex>
#include	<QWaitCondition>
#include	"mp2processor.h"
#include	"mp4processor.h"
#include	"deconvolve.h"
#include	"audiosink.h"
#include	"gui.h"
//
//	As an experiment a version of the backend is created
//	that will be running in a separate thread. Might be
//	useful for multicore processors.
//
//	Interleaving is - for reasons of simplicity - done
//	inline rather than through a special class-object
static
int8_t	interleaveDelays [] = {
	     15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0};
//
//
//	fragmentsize == Length * CUSize
	dabConcurrent::dabConcurrent	(uint8_t dabModus,
	                                 int16_t fragmentSize,
	                                 int16_t bitRate,
	                                 int16_t uepFlag,
	                                 int16_t protLevel,
	                                 RadioInterface *mr,
	                                 audioSink *as) {
int32_t i, j;
	this	-> dabModus		= dabModus;
	this	-> fragmentSize		= fragmentSize;
	this	-> bitRate		= bitRate;
	this	-> uepFlag		= uepFlag;
	this	-> protLevel		= protLevel;
	this	-> myRadioInterface	= mr;
	this	-> myAudioSink		= as;

	outV			= new uint8_t [bitRate * 24];
	interleaveData		= new int16_t *[fragmentSize]; // max size
	Data			= new int16_t [fragmentSize];
	for (i = 0; i < fragmentSize; i ++) {
	   interleaveData [i] = new int16_t [16];
	   for (j = 0; j < 16; j ++)
	      interleaveData [i][j] = 0;
	}

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
	                                        bitRate);
	else
	if (dabModus == DAB_PLUS) 
	   our_dabProcessor = new mp4Processor (myRadioInterface,
	                                        myAudioSink,
	                                        bitRate);
	else		// cannot happen
	   our_dabProcessor = new dabProcessor ();

	myAudioSink	-> restart	();
	Buffer		= new RingBuffer<int16_t>(64 * 32768);
	running		= true;
	start ();
}

	dabConcurrent::~dabConcurrent	(void) {
int16_t	i;
	running = false;
	while (this -> isRunning ())
	   usleep (1);
	if (uepProcessor != NULL)
	   delete uepProcessor;
	if (eepProcessor != NULL)
	   delete eepProcessor;
	delete our_dabProcessor;
	delete	Buffer;
	delete[]	outV;
	for (i = 0; i < fragmentSize; i ++) 
	   delete[]  interleaveData [i];
	delete [] interleaveData;
	delete [] Data;
}

int32_t	dabConcurrent::process	(int16_t *v, int16_t cnt) {
int32_t	fr;
	   if (Buffer -> GetRingBufferWriteAvailable () < cnt)
	      fprintf (stderr, "dab-concurrent: buffer full\n");
	   while ((fr = Buffer -> GetRingBufferWriteAvailable ()) <= cnt) {
	      if (!running)
	         return 0;
	      usleep (1);
	   }

	   Buffer	-> putDataIntoBuffer (v, cnt);
	   Locker. wakeAll ();
	   return fr;
}

void	dabConcurrent::run	(void) {
int16_t	i, j;
int32_t	countforInterleaver	= 0;
uint8_t	shiftRegister [9];

	while (running) {
	   while (Buffer -> GetRingBufferReadAvailable () <= fragmentSize) {
	      ourMutex. lock ();
	      Locker. wait (&ourMutex, 1);	// 1 msec waiting time
	      ourMutex. unlock ();
	      if (!running)
	         break;
	   }

	   if (!running) 
	      break;

	   Buffer	-> getDataFromBuffer (Data, fragmentSize);
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
	   if (countforInterleaver <= 15) {
	      countforInterleaver ++;
	      continue;
	   }
//
	   if (uepFlag == 0)
	      uepProcessor -> deconvolve (Data, fragmentSize, outV);
	   else
	      eepProcessor -> deconvolve (Data, fragmentSize, outV);
//
//	and the inline energy dispersal
	   memset (shiftRegister, 1, 9);
	   for (i = 0; i < bitRate * 24; i ++) {
	      uint8_t b = shiftRegister [8] ^ shiftRegister [4];
	      for (j = 8; j > 0; j--)
	         shiftRegister [j] = shiftRegister [j - 1];
	      shiftRegister [0] = b;
	      outV [i] ^= b;
	   }
	   our_dabProcessor -> addtoFrame (outV, 24 * bitRate);
	}
}
//
//	It might take a msec for the task to stop
void	dabConcurrent::stopRunning (void) {
	running = false;
	while (this -> isRunning ())
	   usleep (1);
//	myAudioSink	-> stop ();
}

