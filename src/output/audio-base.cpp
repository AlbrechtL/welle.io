#
/*
 *    Copyright (C) 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
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

#include	"audio-base.h"
#include	<stdio.h>

/*
 *	The class is the abstract sink for the data generated
 *	It will handle the "dumping" though
 */
	audioBase::audioBase	(RingBuffer<int16_t> *b):
	                              converter_16 (16000, 48000, 2 * 1600),
	                              converter_24 (24000, 48000, 2 * 2400),
	                              converter_32 (32000, 48000, 4 * 3200) {
	buffer			= b;
}

	audioBase::~audioBase	(void) {
}

void	audioBase::restart	(void) {
}

void	audioBase::stop	(void) {
}
//
//	This one is a hack for handling different baudrates coming from
//	the aac decoder. call is from the GUI, triggered by the
//	aac decoder or the mp3 decoder
void	audioBase::audioOut	(int32_t rate) {
int16_t V [rate / 5];

	while (buffer -> GetRingBufferReadAvailable () > rate / 5) {
	   int16_t amount = buffer -> getDataFromBuffer (V, rate / 5);
	   switch (rate) {
	      case 16000:	
	         audioOut_16000 (V, amount / 2);
	         return;
	      case 24000:
	         audioOut_24000 (V, amount / 2);
	         return;
	      case 32000:
	         audioOut_32000 (V, amount / 2);
	         return;
	      default:
	      case 48000:
	         audioOut_48000 (V, amount / 2);
	         return;
	   }
	}
}
//
//	scale up from 16 -> 48
//	amount gives number of pairs
void	audioBase::audioOut_16000	(int16_t *V, int32_t amount) {
DSPCOMPLEX outputBuffer [converter_16. getOutputsize ()];
float      buffer       [2 * converter_16. getOutputsize ()];
int16_t	i;
int32_t	result;

	for (i = 0; i < amount; i ++)
	   if (converter_16. convert (DSPCOMPLEX (V [2 * i] / 32767.0,
	                                          V [2 * i + 1] / 32767.0),
	                              outputBuffer, &result)) {
	      for (i = 0; i < result; i ++) {
	         buffer [2 * i    ] = real (outputBuffer [i]);
	         buffer [2 * i + 1] = imag (outputBuffer [i]);
	      }
	   
	      audioOutput (buffer, result);
	   }
}

//	scale up from 24000 -> 48000
//	amount gives number of pairs
void	audioBase::audioOut_24000	(int16_t *V, int32_t amount) {
DSPCOMPLEX outputBuffer [converter_24. getOutputsize ()];
float      buffer       [2 * converter_24. getOutputsize ()];
int16_t	i;
int32_t	result;

	for (i = 0; i < amount; i ++)
	   if (converter_24. convert (DSPCOMPLEX (V [2 * i] / 32767.0,
	                                          V [2 * i + 1] / 32767.0),
	                              outputBuffer, &result)) {
	      for (i = 0; i < result; i ++) {
	         buffer [2 * i    ] = real (outputBuffer [i]);
	         buffer [2 * i + 1] = imag (outputBuffer [i]);
	      }

	      audioOutput (buffer, result);
	   }
}
//
//	scale up from 32000 -> 48000
//	amount is number of pairs
void	audioBase::audioOut_32000	(int16_t *V, int32_t amount) {
DSPCOMPLEX outputBuffer [converter_32. getOutputsize ()];
float      buffer       [2 * converter_32. getOutputsize ()];
int32_t	i;
int32_t	result;

	for (i = 0; i < amount; i ++) {
	   if (converter_32. convert (DSPCOMPLEX (V [2 * i] / 32767.0,
	                                          V [2 * i + 1] / 32767.0),
	                              outputBuffer, &result)) {
	      for (i = 0; i < result; i ++) {
	         buffer [2 * i    ] = real (outputBuffer [i]);
	         buffer [2 * i + 1] = imag (outputBuffer [i]);
	      }
	   
	      audioOutput (buffer, result);
	   }
	}
}

void	audioBase::audioOut_48000	(int16_t *V, int32_t amount) {
float *buffer = (float *)alloca (2 * amount * sizeof (float));
int32_t	i;

	for (i = 0; i < amount; i ++) {
	   buffer [2 * i]	= V [2 * i] / 32767.0;
	   buffer [2 * i + 1]	= V [2 * i + 1] / 32767.0;
	}

	audioOutput (buffer, amount);
}

//
//	The audioOut function is the one that really should be
//	reimplemented in the offsprings of this class
void	audioBase::audioOutput	(float *v, int32_t amount) {
	(void)v;
	(void)amount;
}

