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
 */
#include	"phasereference.h" 
#include	"string.h"

	phaseReference::phaseReference (DabParams *p, int16_t threshold):
	                                     phaseTable (p -> dabMode) {
int32_t	i;
DSPFLOAT	Phi_k;

	this	-> Tu		= p -> T_u;
	this	-> threshold	= threshold;

	Max			= 0.0;
	refTable		= new DSPCOMPLEX 	[Tu];	//
	fft_processor		= new common_fft 	(Tu);
	fft_buffer		= fft_processor		-> getVector ();
	res_processor		= new common_ifft 	(Tu);
	res_buffer		= res_processor		-> getVector ();
	fft_counter		= 0;

	memset (refTable, 0, sizeof (DSPCOMPLEX) * Tu);

	for (i = 1; i <= p -> K / 2; i ++) {
	   Phi_k =  get_Phi (i);
	   refTable [i - 1] = DSPCOMPLEX (cos (Phi_k), sin (Phi_k));
	   Phi_k = get_Phi (-i);
	   refTable [Tu - i] = DSPCOMPLEX (cos (Phi_k), sin (Phi_k));
	}
}

	phaseReference::~phaseReference (void) {
	delete	refTable;
	delete	fft_processor;
}

int32_t	phaseReference::findIndex (DSPCOMPLEX *v, uint32_t amount) {
int32_t	i;
int32_t	maxIndex	= -1;
float	sum		= 0;

	Max	= 1.0;
	memcpy (fft_buffer, v, amount * sizeof (DSPCOMPLEX));
	memset (&fft_buffer [amount], 0, (Tu - amount) * sizeof (DSPCOMPLEX));

	fft_processor -> do_FFT ();
//
//	back into the frequency domain, now correlate
	for (i = 0; i < Tu; i ++) 
	   res_buffer [i] = fft_buffer [i] * conj (refTable [i]);
//	and, again, back into the time domain
	res_processor	-> do_IFFT ();
//
	for (i = 0; i < Tu; i ++)
	   sum	+= abs (res_buffer [i]);
	Max	= -10000;
	for (i = 0; i < (int)amount; i ++)
	   if (abs (res_buffer [i]) > Max) {
	      maxIndex = i;
	      Max = abs (res_buffer [i]);
	   }

	if (Max < 5 * sum / Tu)
	   return  - abs (Max / (sum / Tu)) - 1;
	else
	   return maxIndex;	// if not found, it will be -1
}
//
