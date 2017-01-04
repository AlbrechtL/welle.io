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
	                            f_16000 (5,  8000, 48000),
	                            f_24000 (5, 12000, 48000),
	                            f_32000 (5, 16000, 96000) {
	buffer			= b;
	dumpFile		= NULL;
}

	audioBase::~audioBase	(void) {
}

void	audioBase::restart	(void) {
}

void	audioBase::stop	(void) {
}
//
//	This one is a hack for handling different baudrates coming from
//	the aac decoder
void	audioBase::audioOut	(int32_t rate) {
int16_t V [rate / 8];

	while (buffer -> GetRingBufferReadAvailable () > rate / 8) {
	   int16_t amount = buffer -> getDataFromBuffer (V, rate / 8);
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
void	audioBase::audioOut_16000	(int16_t *V, int32_t amount) {
float *buffer = (float *)alloca (3 * 2 * amount * sizeof (float));
int32_t	i;

	for (i = 0; i < amount; i ++) {
	   DSPCOMPLEX help = DSPCOMPLEX (float (V [2 * i]) / 32767.0,
	                      float (V [2 * i + 1]) / 32767.0);
	   help	= f_16000. Pass (help);
	   buffer [6 * i] = real (help);
	   buffer [6 * i + 1] = imag (help);
	   help = f_16000. Pass (DSPCOMPLEX (0, 0));
	   buffer [6 * i + 2] = real (help);
	   buffer [6 * i + 3] = imag (help);
	   help = f_16000. Pass (DSPCOMPLEX (0, 0));
	   buffer [6 * i + 4] = real (help);
	   buffer [6 * i + 5] = imag (help);
	}

	myLocker. lock ();
	if (dumpFile != NULL)
	   sf_writef_float (dumpFile, (float *)buffer, 3 * amount);
	myLocker. unlock ();

	audioOutput (buffer, 3 * amount);
}

//
//	amount gives number of pairs
void	audioBase::audioOut_24000	(int16_t *V, int32_t amount) {
float *buffer = (float *)alloca (4 * amount * sizeof (float));
int32_t	i;

	for (i = 0; i < amount; i ++) {
	   DSPCOMPLEX help = DSPCOMPLEX (float (V [2 * i]) / 32767.0,
	                                 float (V [2 * i + 1]) / 32767.0);
	   help = f_24000. Pass (help);
	   buffer [4 * i]	= real (help);
	   buffer [4 * i + 1]	= imag (help);
	   help = f_24000. Pass (DSPCOMPLEX (0, 0));
	   buffer [4 * i + 2]	= real (help);
	   buffer [4 * i + 3]	= imag (help);
	}

	myLocker. lock ();
	if (dumpFile != NULL)
	   sf_writef_float (dumpFile, (float *)buffer, 2 * amount);
	myLocker. unlock ();

	audioOutput (buffer, 2 * amount);
}
//
//	Conversion from 32 -> 48 is by first converting to 96000
//	and then back to 48000
void	audioBase::audioOut_32000	(int16_t *V, int32_t amount) {
DSPCOMPLEX *buffer_1 = (DSPCOMPLEX *)alloca (3 * amount * sizeof (DSPCOMPLEX));
float	*buffer = (float *)alloca (2 * 3 * amount / 2 * sizeof (float));
int32_t	i;

	for (i = 0; i < amount; i ++) {
	   DSPCOMPLEX help = DSPCOMPLEX (float (V [2 * i]) / 32767.0,
	                                 float (V [2 * i + 1]) / 32767.0);
	   buffer_1 [3 * i + 0] = f_32000. Pass (help);
	   buffer_1 [3 * i + 1] = f_32000. Pass (DSPCOMPLEX (0, 0));
	   buffer_1 [3 * i + 2] = f_32000. Pass (DSPCOMPLEX (0, 0));
	}
//
//	Note that although we use "2 * i" for index left and right
//	they mean different things
	for (i = 0; i < 3 * amount / 2; i ++) {
	   buffer [2 * i]	= real (buffer_1 [2 * i]);
	   buffer [2 * i + 1]	= imag (buffer_1 [2 * i]);
	}
	   
	myLocker. lock ();
	if (dumpFile != NULL)
	   sf_writef_float (dumpFile, (float *)buffer, 3 * amount / 2);
	myLocker. unlock ();

	audioOutput (buffer, 3 * amount / 2);
}

void	audioBase::audioOut_48000	(int16_t *V, int32_t amount) {
float *buffer = (float *)alloca (2 * amount * sizeof (float));
int32_t	i, n;

	for (i = 0; i < amount; i ++) {
	   buffer [2 * i]	= V [2 * i] / 32767.0;
	   buffer [2 * i + 1]	= V [2 * i + 1] / 32767.0;
	}

	myLocker. lock ();
	if (dumpFile != NULL)
	   sf_writef_float (dumpFile, (float *)buffer, amount);
	myLocker. unlock ();
	audioOutput (buffer, amount);
}
//
void	audioBase::startDumping	(SNDFILE *f) {
	myLocker. lock ();
	dumpFile	= f;
	myLocker. unlock ();
}

void	audioBase::stopDumping	(void) {
	myLocker. lock ();
	dumpFile	= NULL;
	myLocker. unlock ();
}

//
//	The audioOut function is the one that really should be
//	reimplemented in the offsprings of this class
void	audioBase::audioOutput	(float *v, int32_t amount) {
	(void)v;
	(void)amount;
}

