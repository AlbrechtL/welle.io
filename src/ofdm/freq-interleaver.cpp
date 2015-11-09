#
/*
 *
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
 *
 */
#
#include	<stdint.h>
#include	<stdio.h>
#include	"freq-interleaver.h"

/**
  *	\brief createMapper
  *	create the mapping table  for the (de-)interleaver
  *	formulas according to section 14.6 (Frequency interleaving)
  *	of the DAB standard
  */

int16_t	*createMapper (int16_t T_u, int16_t V1, 
	               int16_t lwb, int16_t upb, int16_t *v) {
int16_t	*tmp	= (int16_t *)alloca (T_u * sizeof (int16_t));
int16_t	index	= 0;
int16_t	i;

	tmp [0]	= 0;
	for (i = 1; i < T_u; i ++)
	   tmp [i] = (13 * tmp [i - 1] + V1) % T_u;
	for (i = 0; i < T_u; i ++) {
	   if (tmp [i] == T_u / 2)
	      continue;
	   if ((tmp [i] < lwb) ||
	       (tmp [i] > upb)) 
	      continue;
//	we now have a table with values from lwb .. upb
//
	   v [index ++] = tmp [i] - T_u / 2;
//	we now have a table with values from lwb - T_u / 2 .. lwb + T_u / 2
	}

	return v;
}

	interLeaver::interLeaver (DabParams *p) {

	switch (p -> dabMode) {
	   case 1:
	   default:		// shouldn't happen
	      permTable	= createMapper (p -> T_u,
	                                511, 256, 256 + p -> K,
	                                new int16_t [p -> T_u]);
	      break;
	   case 2:
	      permTable = createMapper (p -> T_u,
	                                127, 64, 64 + p -> K,
	                                new int16_t [p -> T_u]);
	      break;

	   case 3:
	      permTable = createMapper (p -> T_u,
	                                63, 32, 32 + p -> K,
	                                new int16_t [p -> T_u]);
	      break;

	   case 4:
	      permTable = createMapper (p -> T_u,
	                                255, 128, 128 + p -> K,
	                                new int16_t [p -> T_u]);
	      break;
	}
}
//
//
	interLeaver::~interLeaver (void) {
	delete	permTable;
}
//
//	according to the standard, the map is a function from
//	0 .. 1535 -> -768 .. 768 (with exclusion of {0})
int16_t	interLeaver::mapIn (int16_t n) {
	return permTable [n];
}


