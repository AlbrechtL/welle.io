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
//
//	Pretty straightforward package for galois computing,
//	up to 8 bits symsize

#include	"galois.h"
#include	<stdio.h>

		galois::galois (uint16_t symsize, uint16_t gfpoly) {
uint16_t sr;
uint16_t i;

	this	-> mm		= symsize;
	this	-> gfpoly	= gfpoly;
	this	-> codeLength	= (1 << mm) - 1;
	this	-> d_q		= 1 << mm;
	this	-> alpha_to	= new uint16_t [codeLength + 1];
	this	-> index_of	= new uint16_t [codeLength + 1];
/*	Generate Galois field lookup tables */
	index_of [0] = codeLength;	/* log (zero) = -inf */
	alpha_to [codeLength] = 0;	/* alpha**-inf = 0 */

	sr = 1;
	for (i = 0; i < codeLength; i++){
	   index_of [sr] = i;
	   alpha_to [i] = sr;
	   sr <<= 1;
	   if (sr & (1 << symsize))
	      sr ^= gfpoly;
	   sr &= codeLength;
	}
}

int	galois::modnn (int x){
	while (x >= codeLength) {
	   x -= codeLength;
	   x = (x >> mm) + (x & codeLength);
	}
	return x;
}

		galois::~galois	(void) {
	delete [] alpha_to;
	delete [] index_of;
}

static inline
uint16_t	round_mod (int16_t a, int16_t n) {
	return (a % n < 0) ? (a % n + n) : (a % n);
}

uint16_t	galois::add_poly	(uint16_t a, uint16_t b) {
	return a ^ b;
}

uint16_t	galois::poly2power	(uint16_t a) {
	return index_of [a];
}

uint16_t	galois::power2poly	(uint16_t a) {
	return alpha_to [a];
}

uint16_t	galois::add_power	(uint16_t a, uint16_t b) {
	return index_of [alpha_to [a] ^ alpha_to [b]];
}

uint16_t	galois::multiply_power	(uint16_t a, uint16_t b) {
	return modnn (a + b);
}

uint16_t	galois::multiply_poly (uint16_t a, uint16_t b) {
	if ((a == 0) || (b == 0))
	   return 0;
	return alpha_to [multiply_power (index_of [a], index_of [b])];
}

uint16_t	galois::divide_power (uint16_t a, uint16_t b) {
	return modnn (d_q - 1 + a - b);
}

uint16_t	galois::divide_poly (uint16_t a, uint16_t b) {
	if (a == 0)
	   return 0;
	return alpha_to [divide_power (index_of [a], index_of [b])];
}

uint16_t	galois::inverse_poly	(uint16_t a) {
	return alpha_to [inverse_power (index_of [a])];
}

uint16_t	galois::inverse_power	(uint16_t a) {
	return d_q - 1 - a;
}

uint16_t	galois::pow_poly (uint16_t a, uint16_t n) {
	return alpha_to [pow_power (index_of [a], n)];
}

uint16_t	galois::pow_power (uint16_t a, uint16_t n) {
	return (a == 0) ? 0 : (a * n) % (d_q - 1);
}

