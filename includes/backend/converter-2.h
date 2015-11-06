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
 */

//	special instantiation for pairs of int16_t
//	to be used for the faad decoder
#ifndef	__CONVERTER_2
#define	__CONVERTER_2
//
//	Very straightforward fractional resampler
#include	"dab-constants.h"

class	converter_2 {
private:
	int32_t		rateIn;	
	int32_t		rateOut;
	int32_t		blockLength;
	int32_t		width;
	DSPCOMPLEX	*buffer;
	int32_t		bufferP;
	long double	floatTime;
	long double	inPeriod;
	long double	outPeriod;
//
//	sin (-a) = - sin (--a)
double sincPI (double a) {
	if (a == 0)
	   return 1.0;
	return sin (M_PI * a) / (M_PI * a);
}
//
//	cos (-a) = cos (a)
double HannCoeff (double a, int16_t width) {
DSPFLOAT x = 2 * M_PI * (0.5 + a / width);
	if (x < 0)
	   x = - x;
	return 0.5 - 0.5 * cos (x);
}
//
//	Shannon applied to floatTime
//	We determine the entry in the table acting as zero
DSPCOMPLEX	getInterpolate (double floatTime) {
int32_t	index	= (int32_t)(floor (floatTime * rateIn));
int32_t	i;
DSPCOMPLEX	res	= 0;
double	localTime = floatTime - index * inPeriod;
//
//	Due to rounding of the (floating) computation, it
//	might happen that index + i is sometimes out of bounds
	for (i = - width / 2; i < width / 2; i ++) {
	   if (index + i < 0 || index + i >= blockLength + width)
	      continue;
	   double ag = (localTime - i * inPeriod) / inPeriod;
	   double factor = HannCoeff (ag, width) * sincPI (ag);
	   res	=  res + DSPCOMPLEX (real (buffer [index + i]) * factor,
	                             imag (buffer [index + i]) * factor);
	}

	return res;
}
//
public:
		converter_2 (int32_t	rateIn,
	                     int32_t	rateOut,
	                     int32_t	blockLength,
	                     int16_t	width) {
	this	-> rateIn	= rateIn;
	this	-> rateOut	= rateOut;
	this	-> blockLength	= blockLength;
	this	-> width	= width;
	buffer			= new DSPCOMPLEX [blockLength + width];
	bufferP			= 0;	
	inPeriod		= 1.0 / rateIn;
	outPeriod		= 1.0 / rateOut;
	fprintf (stderr, "converter from %d to %d\n", rateIn, rateOut);
//	the first width / 2 samples are only used in the interpolation
//	furthermore, they are neglected. So, we start with:
	floatTime		= width / 2 * inPeriod;
}

		~converter_2 (void) {
	delete []	buffer;
}
//
//	The buffer consists of three parts
//	width / 2 "old" values
//	blockLength values that will be processed
//	width / 2 "future" values
//
//	Whenever the buffer filling reaches blockLength + width,
//	we map the blockLength samples in the middle
bool	add	(int16_t in_re,  int16_t in_im,
	         int16_t *out, int16_t *nOut) {
int32_t	i;
int	outP		= 0;
double	endTime;

	buffer [bufferP ++] = DSPCOMPLEX (in_re, in_im);
	if (bufferP < blockLength + width)
	   return false;

	endTime		= floatTime + blockLength * inPeriod;
//	floatTime indicates the current time for the
//	output samples, endtime is the time of the last
//	input sample
	while (floatTime < endTime ) {
	   DSPCOMPLEX temp = getInterpolate (floatTime);
	   out [2 * outP]	= real (temp);
	   out [2 * outP + 1]	= imag (temp);
	   outP += 1;
	   floatTime += outPeriod;
	}
//
//	shift the "width" samples at the end of the buffer
//	The first "width / 2" samples of the buffer now are already processed
//	but required for processing the next series
	for (i = 0; i < width; i ++)
	   buffer [i] = buffer [blockLength + i];
//
//	adjust  the begin and end time
	floatTime	-= outP * outPeriod;
	bufferP		= floatTime / inPeriod + width / 2;
	*nOut = outP;
	return true;
}

int32_t	getOutputSize	(void) {
	return rateOut * blockLength / rateIn;
}
};

#endif

