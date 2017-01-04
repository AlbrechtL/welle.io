#
/*
 *    Copyright (C) 2011, 2012, 2013
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
#ifndef	__NEW_CONVERTER
#define	__NEW_CONVERTER

#include	<math.h>
#include	<complex>
#include	<stdint.h>
#include	<unistd.h>
#include	<limits>
#include	<samplerate.h>
#include	"dab-constants.h"

class	newConverter {
private:
	int32_t		inRate;
	int32_t		outRate;
	double		ratio;
	int32_t		outputLimit;
	int32_t		inputLimit;
	SRC_STATE	*converter;
	SRC_DATA	*src_data;
	float		*inBuffer;
	float		*outBuffer;
	int32_t		inp;
public:
		newConverter (int32_t inRate, int32_t outRate, 
	                      int32_t inSize);

		~newConverter (void);

bool	convert (DSPCOMPLEX v,
	                       DSPCOMPLEX *out, int32_t *amount);

int32_t	getOutputsize (void);
};

#endif

