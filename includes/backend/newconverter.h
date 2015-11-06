#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J
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
//
//	temporary solution for a very simple converter from 51200 -> 48000
//	as needed (for now) in the generation of aac samples
//	The faad library generates samples for a rate of 51200,
//	we need samples with a rate of 48000
#ifndef	__AAC_CONVERTER
#define	__AAC_CONVERTER

#include	"dab-constants.h"

class	newConverter {
private:
	int16_t		*inBuffer;
	float		*mapTable;
	int32_t		inp;
public:
		newConverter (void) {
int16_t	i;
	this	-> mapTable	= new float [480];
	for (i = 0; i < 480; i ++)
	   mapTable [i] = float (i) * 512.0 / 480.0;
	this	-> inBuffer	= new int16_t [2 * (512 + 1)];
	inp			= 1;
}

		~newConverter (void) {
	delete []	inBuffer;
	delete []	mapTable;
}
//
//	for the faad decoder, we have an entry with "int16_t's"
//	as input
//	we simply map 512 incoming samples (not packed, so appearing
//	as 2 * 512) into 480 outgoing samples
bool	add (int16_t left, int16_t right, DSPCOMPLEX *out, int16_t *amount) {
int16_t	i;
float temp [2 * 480];
	inBuffer [2 * inp]	= left;
	inBuffer [2 * inp + 1]	= right;
	inp ++;
	if (inp <= 512)
	   return false;

	for (i = 0; i < 480; i ++) {
	   int	theIndex	= int (floor (mapTable [i]));
	   float theFraction	= mapTable [i] - theIndex;
	   temp [2 * i] = theFraction * inBuffer [2 * (theIndex + 1)] +
	                 (1.0 - theFraction) * inBuffer [2 * theIndex];
	   temp [2 * i + 1] = theFraction * inBuffer [2 * (theIndex + 1) + 1] +
	                 (1.0 - theFraction) * inBuffer [2 * theIndex + 1];
	   out [i]	= DSPCOMPLEX (temp [2 * i] / 32768.0,
	                              temp [2 * i + 1] / 32768.0);
	}

	inBuffer [0] = inBuffer [2 * 512];
	inBuffer [1] = inBuffer [2 * 512 + 1];
	*amount      = 480;
	inp	     = 1;
	return true;
}
	
int32_t	getOutputSize	(void) {
	return 480;
}
};

#endif

