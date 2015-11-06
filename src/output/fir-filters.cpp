#
/*
 *    Copyright (C) 2010, 2011, 2012
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

#include	"fir-filters.h"
#ifndef	__MINGW32__
#include	"alloca.h"
#endif

//FIR LowPass

	LowPassFIR::LowPassFIR (int16_t firsize,
	                        int32_t Fc, int32_t fs){
DSPFLOAT	f	= (DSPFLOAT)Fc / fs;
DSPFLOAT	sum	= 0.0;
int16_t		i;
DSPFLOAT	*temp 	= (DSPFLOAT *)alloca (firsize * sizeof (DSPFLOAT));

	filterSize	= firsize;
	filterKernel	= new DSPCOMPLEX [filterSize];
	Buffer		= new DSPCOMPLEX [filterSize];
	ip		= 0;

	for (i = 0; i < filterSize; i ++) {
	   filterKernel [i]	= 0;
	   Buffer [i]		= 0;
	}

	for (i = 0; i < filterSize; i ++) {
	   if (i == filterSize / 2)
	      temp [i] = 2 * M_PI * f;
	   else 
	      temp [i] =
	         sin (2 * M_PI * f * (i - filterSize/2))/ (i - filterSize/2);
//
//	Blackman window
	   temp [i]  *= (0.42 -
		    0.5 * cos (2 * M_PI * (DSPFLOAT)i / filterSize) +
		    0.08 * cos (4 * M_PI * (DSPFLOAT)i / filterSize));

	   sum += temp [i];
	}

	for (i = 0; i < filterSize; i ++)
	   filterKernel [i] = DSPCOMPLEX (temp [i] / sum, 0);
}

	LowPassFIR::~LowPassFIR () {
	delete[]	filterKernel;
	delete[]	Buffer;
}
//
//	we process the samples backwards rather than reversing
//	the kernel
DSPCOMPLEX	LowPassFIR::Pass (DSPCOMPLEX z) {
int16_t	i;
DSPCOMPLEX	tmp	= 0;

	Buffer [ip]	= z;
	for (i = 0; i < filterSize; i ++) {
	   int16_t index = ip - i;
	   if (index < 0)
	      index += filterSize;
	   tmp		+= Buffer [index] * filterKernel [i];
	}

	ip = (ip + 1) % filterSize;
	return tmp;
}

DSPFLOAT LowPassFIR::Pass (DSPFLOAT v) {
int16_t		i;
DSPFLOAT	tmp	= 0;

	Buffer [ip] = DSPCOMPLEX (v, 0);
	for (i = 0; i < filterSize; i ++) {
	   int16_t index = ip - i;
	   if (index < 0)
	      index += filterSize;
	   tmp += real (Buffer [index]) * real (filterKernel [i]);
	}

	ip = (ip + 1) % filterSize;
	return tmp;
}


