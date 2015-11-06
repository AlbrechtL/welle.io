#
/*
 *    Copyright (C) 2013 2015
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
 *	Once the bits are "in", interpretation and manipulation
 *	should reconstruct the data blocks.
 *	Ofdm_decoder is called once every Ts samples, and
 *	its invocation results in 2 * Tu bits
 */
#include	"ofdm-decoder.h"
#include	"gui.h"
#include	"phasetable.h"
#include	"fic-handler.h"
#include	"msc-handler.h"

	ofdm_decoder::ofdm_decoder	(DabParams	*p,
	                                 RadioInterface *mr,
	                                 ficHandler	*my_ficHandler,
	                                 mscHandler	*my_mscHandler) {
	this	-> params		= p;
	this	-> myRadioInterface	= mr;
	this	-> my_ficHandler	= my_ficHandler;
	this	-> my_mscHandler	= my_mscHandler;
	this	-> T_s			= params	-> T_s;
	this	-> T_u			= params	-> T_u;
	this	-> carriers		= params	-> K;
	ibits				= new int16_t [2 * this -> carriers];

	this	-> T_g			= T_s - T_u;
	fft_handler			= new common_fft (T_u);
	fft_buffer			= fft_handler -> getVector ();
	phaseReference			= new DSPCOMPLEX [T_u];
	myMapper			= new permVector (params);
	coarseOffset			= 0;
//
	connect (this, SIGNAL (show_snr (int)),
	         mr, SLOT (show_snr (int)));
	snrCount		= 0;
	snr			= 0;	
	strength		= 0;

#ifdef	MULTI_CORE
//	a command is a combination <blockno, blockdata>
//	daar kunnen we dus ook een matrix van makena
	command			= new DSPCOMPLEX * [params -> L + 1];
	int16_t	i;
	for (i = 0; i < params -> L + 1; i ++)
	   command [i] = new DSPCOMPLEX [T_u];
	amount		= 0;
	start ();
#endif
}

	ofdm_decoder::~ofdm_decoder	(void) {
#ifdef	MULTI_CORE
int16_t	i;
	running	= false;
	commandHandler. wakeAll ();
	usleep (1000);
	while (!isFinished () && isRunning ());
	   usleep (100);
#endif
	delete	fft_handler;
	delete	phaseReference;
	delete	myMapper;
#ifdef	MULTI_CORE
	for (i = 0; i < params -> L + 1; i ++)
	   delete[] command [i];
	delete[] command;
#endif
}
//
//
#ifdef	MULTI_CORE
void	ofdm_decoder::run	(void) {
int16_t	currentBlock	= 0;

	running		= true;
	while (running) {
	   helper. lock ();
	   commandHandler. wait (&helper, 100);
	   helper. unlock ();
	   while ((amount > 0) && running) {
	      memcpy (fft_buffer, command [currentBlock], T_u * sizeof (DSPCOMPLEX));
	      if (currentBlock == 0)
	         processBlock_0 ();
	      else
	      if (currentBlock < 4)
	         decodeFICblock (currentBlock + 1);
	      else
	         decodeMscblock (currentBlock + 1);
	      helper. lock ();
	      currentBlock = (currentBlock + 1) % (params -> L);
	      amount -= 1;
	      helper. unlock ();
	   }
	}
}
//
void	ofdm_decoder::processBlock_0 (DSPCOMPLEX *vi) {
	memcpy (command [0], vi, sizeof (DSPCOMPLEX) * T_u);
	helper. lock ();
	amount ++;
	commandHandler. wakeOne ();
	helper. unlock ();
}

void	ofdm_decoder::decodeFICblock (DSPCOMPLEX *vi, int32_t blkno) {
	memcpy (command [blkno], &vi [T_g], sizeof (DSPCOMPLEX) * T_u);
	helper. lock ();
	amount ++;
	commandHandler. wakeOne ();
	helper. unlock ();
}

void	ofdm_decoder::decodeMscblock (DSPCOMPLEX *vi, int32_t blkno) {
	memcpy (command [blkno], &vi [T_g], sizeof (DSPCOMPLEX) * T_u);
	helper. lock ();
	amount ++;
	commandHandler. wakeOne ();
	helper. unlock ();
}
#endif

//	in practice, we use the "incoming" block
//	and use its data to generate the prs
#ifndef	MULTI_CORE
void	ofdm_decoder::processBlock_0 (DSPCOMPLEX *vi) {
	memcpy (fft_buffer, vi, T_u * sizeof (DSPCOMPLEX));
#else
void	ofdm_decoder::processBlock_0 (void) {
	memcpy (fft_buffer, command [0], T_u * sizeof (DSPCOMPLEX));
#endif
DSPCOMPLEX	*v = (DSPCOMPLEX *)alloca (T_u * sizeof (DSPCOMPLEX));
	fft_handler	-> do_FFT ();
//	The + and - frequencies are still to be switched, for simple
//	strength and middle computation:
	memcpy (v, &fft_buffer [T_u / 2], T_u / 2 * sizeof (DSPCOMPLEX));
	memcpy (&v [T_u / 2], fft_buffer, T_u / 2 * sizeof (DSPCOMPLEX));
	coarseOffset 	= getMiddle	(v);
	snr		= 0.7 * snr + 0.3 * get_snr (v);
	strength	= 0.7 * strength + 0.3 * newStrength (v);
	if (++snrCount > 10) {
	   show_snr (snr);
	   snrCount = 0;
	}
//	we are now in the frequency domain, and we keep the carriers
//	from -Tu / 2 .. Tu / 2 \ {0} in the phaseReference 
	memcpy (phaseReference, fft_buffer, T_u * sizeof (DSPCOMPLEX));
//	memcpy (phaseReference, v, T_u * sizeof (DSPCOMPLEX));
}
//	for the other blocks of data, the first step is to go from
//	time to frequency domain, to get the carriers.
//
//	we distinguish between FIC blocks and other blocks,
//	just to save a test, the mapping code is the same
#ifndef	MULTI_CORE
void	ofdm_decoder::decodeFICblock (DSPCOMPLEX *vi, int32_t blkno) {
int16_t	i;
	memcpy (fft_buffer, &vi [T_g], T_u * sizeof (DSPCOMPLEX));
#else
void	ofdm_decoder::decodeFICblock (int32_t blkno) {
int16_t	i;
	memcpy (fft_buffer, command [blkno], T_u * sizeof (DSPCOMPLEX));
#endif
	fft_handler -> do_FFT ();
//
//	Note that "mapIn" maps to -carriers / 2 .. carriers / 2
//	we did not set the fft output to low .. high
	for (i = 0; i < carriers; i ++) {
	   int16_t	index	= myMapper -> mapIn (i);
	   if (index < 0) 
	      index += T_u;
	      
	   DSPCOMPLEX	r1 = fft_buffer [index] * conj (phaseReference [index]);
	   phaseReference [index] = fft_buffer [index];
	   DSPFLOAT ab1	= jan_abs (r1);
//	Recall:  positive = 0, negative = 1
	   ibits [i]		= real (r1) / ab1 * 255.0;
	   ibits [carriers + i] = imag (r1) / ab1 * 255.0;
	}

	my_ficHandler -> process_ficBlock (ibits, blkno);
}
#ifndef	MULTI_CORE
void	ofdm_decoder::decodeMscblock (DSPCOMPLEX *vi, int32_t blkno) {
int16_t	i;
	memcpy (fft_buffer, &vi [T_g], T_u * sizeof (DSPCOMPLEX));
#else
void	ofdm_decoder::decodeMscblock (int32_t blkno) {
int16_t	i;
	memcpy (fft_buffer, command [blkno], T_u * sizeof (DSPCOMPLEX));
#endif
	fft_handler -> do_FFT ();
//
//	Note that "mapIn" maps to -carriers / 2 .. carriers / 2
//	we did not set the fft output to low .. high
	for (i = 0; i < carriers; i ++) {
	   int16_t	index	= myMapper -> mapIn (i);
	   if (index < 0) 
	      index += T_u;
	      
	   DSPCOMPLEX	r1 = fft_buffer [index] * conj (phaseReference [index]);
	   phaseReference [index] = fft_buffer [index];
	   DSPFLOAT ab1	= jan_abs (r1);
//	Recall:  positive = 0, negative = 1
//	we make the bits into softbits in the range -255 .. 255
	   ibits [i]		= real (r1) / ab1 * 255.0;
	   ibits [carriers + i] = imag (r1) / ab1 * 255.0;
	}

	my_mscHandler -> process_mscBlock (ibits, blkno);
}

int16_t	ofdm_decoder::coarseCorrector (void) {
	return coarseOffset;
}

int16_t	ofdm_decoder::getMiddle (DSPCOMPLEX *v) {
int16_t		i;
DSPFLOAT	sum = 0;
int16_t		maxIndex = 0;
DSPFLOAT	oldMax	= 0;
//
//	basic sum over K carriers that are - most likely -
//	in the range
//	The range in which the carrier should be is
//	T_u / 2 - K / 2 .. T_u / 2 + K / 2
//	We first determine an initial sum
	for (i = 10; i < carriers + 10; i ++)
	   sum += abs (v [i]);
//
//	Now a moving sum, look for a maximum within a reasonable
//	range (around (T_u - K) / 2, the start of the useful frequencies)
	for (i = 10; i < T_u - carriers - 10; i ++) {
	   sum -= abs (v [i]);
	   sum += abs (v [i + carriers]);
	   if (sum > oldMax) {
	      sum = oldMax;
	      maxIndex = i;
	   }
	}
	return maxIndex - (T_u - carriers) / 2;
}
//
//
//	for the snr we have a full T_u wide vector, with in the middle
//	K carriers
int16_t	ofdm_decoder::get_snr (DSPCOMPLEX *v) {
int16_t	i;
DSPFLOAT	noise 	= 0;
DSPFLOAT	signal	= 0;
int16_t	low	= T_u / 2 -  carriers / 2;
int16_t	high	= low + carriers;

	for (i = 10; i < low - 20; i ++)
	   noise += abs (v [i]);

	for (i = high + 20; i < T_u - 10; i ++)
	   noise += abs (v [i]);

	noise	/= (low - 30 + T_u - high - 30);
	for (i = T_u / 2 - carriers / 4;  i < T_u / 2 + carriers / 4; i ++)
	   signal += abs (v [i]);

	return get_db (signal / (carriers / 2)) - get_db (noise);
}

int16_t	ofdm_decoder::newStrength (DSPCOMPLEX *v) {
int16_t	i;
DSPFLOAT	signal	= 0;

	for (i = T_u / 2 - carriers / 4; i < T_u / 2 + carriers / 4; i ++)
	   signal += abs (v [i]);
	return get_db (signal / (carriers / 2));
}

int16_t	ofdm_decoder::getStrength (void) {
	return strength;
}

