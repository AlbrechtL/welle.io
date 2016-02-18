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
/**
  *	\class phaseReference
  *	Implements the correlation that is used to identify
  *	the "first" element (following the cyclic prefix) of
  *	the first non-null block of a frame
  *	The class inherits from the phaseTable.
  */
	phaseReference::phaseReference (DabParams	*p,
	                                int16_t		blockSize,
	                                int16_t threshold):
	                                     phaseTable (p -> dabMode) {
int32_t	i;
DSPFLOAT	Phi_k;

	this	-> Tu		= p -> T_u;
	this	-> blockSize	= blockSize;
	if (blockSize > Tu)
	   blockSize = Tu;
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
	   refTable [i] = DSPCOMPLEX (cos (Phi_k), sin (Phi_k));
	   Phi_k = get_Phi (-i);
	   refTable [Tu - i] = DSPCOMPLEX (cos (Phi_k), sin (Phi_k));
	}
}

	phaseReference::~phaseReference (void) {
	delete	refTable;
	delete	fft_processor;
}

/**
  *	\brief findIndex
  *	the vector v contains "blockSize" samples that are believed to
  *	belong to the first non-null block of a DAB frame.
  *	We correlate the data in this verctor with the predefined
  *	data, and if the maximum exceeds a threshold value,
  *	we believe that that indicates the first sample we were
  *	looking for.
  */
int32_t	phaseReference::findIndex (DSPCOMPLEX *v) {
int32_t	i;
int32_t	maxIndex	= -1;
float	sum		= 0;

	Max	= 1.0;
	memcpy (fft_buffer, v, blockSize * sizeof (DSPCOMPLEX));
	if (blockSize < Tu)
	   memset (&fft_buffer [blockSize], 0,
	           (Tu - blockSize) * sizeof (DSPCOMPLEX));

	fft_processor -> do_FFT ();
//
//	back into the frequency domain, now correlate
	for (i = 0; i < Tu; i ++) 
	   res_buffer [i] = fft_buffer [i] * conj (refTable [i]);
//	and, again, back into the time domain
	res_processor	-> do_IFFT ();
/**
  *	We compute the average signal value ...
  */
	for (i = 0; i < Tu; i ++)
	   sum	+= abs (res_buffer [i]);
	Max	= -10000;
	for (i = 0; i < blockSize; i ++)
	   if (abs (res_buffer [i]) > Max) {
	      maxIndex = i;
	      Max = abs (res_buffer [i]);
	   }
/**
  *	that gives us a basis for defining the threshold
  */
	if (Max < threshold * sum / blockSize)
	   return  - abs (Max * blockSize / sum) - 1;
	else
	   return maxIndex;	
}
//
