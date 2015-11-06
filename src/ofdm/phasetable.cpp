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
 */
#include	"phasetable.h" 

static
struct phasetableElement modeI_table [] = {
	{-768, -737, 0, 1},
	{-736, -705, 1, 2},
	{-704, -673, 2, 0},
	{-672, -641, 3, 1},
	{-640, -609, 0, 3},
	{-608, -577, 1, 2},
	{-576, -545, 2, 2},
	{-544, -513, 3, 3},
	{-512, -481, 0, 2},
	{-480, -449, 1, 1},
	{-448, -417, 2, 2},
	{-416, -385, 3, 3},
	{-384, -353, 0, 1},
	{-352, -321, 1, 2},
	{-320, -289, 2, 3},
	{-288, -257, 3, 3},
	{-256, -225, 0, 2},
	{-224, -193, 1, 2},
	{-192, -161, 2, 2},
	{-160, -129, 3, 1},
	{-128,  -97, 0, 1},
	{-96,   -65, 1, 3},
	{-64,   -33, 2, 1},
	{-32,    -1, 3, 2},
	{  1,    32, 0, 3},
	{ 33,    64, 3, 1},
	{ 65,    96, 2, 1},
//	{ 97,   128, 2, 1},  found bug 2014-09-03 Jorgen Scott
	{ 97,	128, 1, 1},
	{ 129,  160, 0, 2},
	{ 161,  192, 3, 2},
	{ 193,  224, 2, 1},
	{ 225,  256, 1, 0},
	{ 257,  288, 0, 2},
	{ 289,  320, 3, 2},
	{ 321,  352, 2, 3},
	{ 353,  384, 1, 3},
	{ 385,  416, 0, 0},
	{ 417,  448, 3, 2},
	{ 449,  480, 2, 1},
	{ 481,  512, 1, 3},
	{ 513,  544, 0, 3},
	{ 545,  576, 3, 3},
	{ 577,  608, 2, 3},
	{ 609,  640, 1, 0},
	{ 641,  672, 0, 3},
	{ 673,  704, 3, 0},
	{ 705,  736, 2, 1},
	{ 737,  768, 1, 1},
	{ -1000, -1000, 0, 0}
};

struct phasetableElement modeII_table [] = {
	{-192,	-161,	0,	2},
	{-160,	-129,	1,	3},
	{-128,	-97,	2,	2},
	{-96,	-65,	3,	2},
	{-64,	-33,	0,	1},
	{-32,	-1,	1,	2},
	{1,	32,	2,	0},
	{33,	64,	1,	2},
	{65,	96,	0,	2},
	{97,	128,	3,	1},
	{129,	160,	2,	0},
	{161,	192,	1,	3},
	{-1000,	-1000,	0,	0}
};

struct phasetableElement modeIV_table [] = {
	{-384, -353, 0, 0},
	{-352, -321, 1, 1},
	{-320, -289, 2, 1},
	{-288, -257, 3, 2},
	{-256, -225, 0, 2},
	{-224, -193, 1, 2},
	{-192, -161, 2, 0},
	{-160, -129, 3, 3},
	{-128,  -97, 0, 3},
	{-96,   -65, 1, 1},
	{-64,   -33, 2, 3},
	{-32,    -1, 3, 2},
	{  1,    32, 0, 0},
	{ 33,    64, 3, 1},
	{ 65,    96, 2, 0},
	{ 97,   128, 1, 2},
	{ 129,  160, 0, 0},
	{ 161,  192, 3, 1},
	{ 193,  224, 2, 2},
	{ 225,  256, 1, 2},
	{ 257,  288, 0, 2},
	{ 289,  320, 3, 1},
	{ 321,  352, 2, 3},
	{ 353,  384, 1, 0},
	{-1000, -1000, 0, 0}
};

	phaseTable::phaseTable (int16_t modus) {
	Mode	= modus;
	currentTable	= modeI_table;
	switch (Mode) {
	   default:
	   case 1:
	      currentTable	= modeI_table;
	      break;

	   case 2:
	      currentTable	= modeII_table;
	      break;

	   case 4:
	      currentTable	= modeIV_table;
	      break;
	}
}

	phaseTable::~phaseTable (void) {
}

static
const int8_t h0 []   = {0, 2, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 2, 2, 1, 1,
	                0, 2, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 2, 2, 1, 1};
static
const int8_t h1 []   = {0, 3, 2, 3, 0, 1, 3, 0, 2, 1, 2, 3, 2, 3, 3, 0,
	                0, 3, 2, 3, 0, 1, 3, 0, 2, 1, 2, 3, 2, 3, 3, 0};
static
const int8_t h2 []   = {0, 0, 0, 2, 0, 2, 1, 3, 2, 2, 0, 2, 2, 0, 1, 3,
	                0, 0, 0, 2, 0, 2, 1, 3, 2, 2, 0, 2, 2, 0, 1, 3};
static
const int8_t h3 []   = {0, 1, 2, 1, 0, 3, 3, 2, 2, 3, 2, 1, 2, 1, 3, 2,
	                0, 1, 2, 1, 0, 3, 3, 2, 2, 3, 2, 1, 2, 1, 3, 2};

int32_t		phaseTable::h_table (int32_t i, int32_t j) {
	switch (i) {
	   case 0:
	      return h0 [j];
	   case 1:
	      return h1 [j];
	   case 2:
	      return h2 [j];
	   default:
	   case 3:
	      return h3 [j];
	}
}

DSPFLOAT	phaseTable::get_Phi (int32_t k) {
int32_t k_prime, i, j, n = 0;

	for (j = 0; currentTable [j]. kmin != -1000; j ++) {
	   if ((currentTable [j]. kmin <= k) && (k <= currentTable [j]. kmax)) {
	      k_prime = currentTable [j]. kmin;
	      i       = currentTable [j]. i;
	      n	      = currentTable [j]. n;
	      return M_PI / 2 * (h_table (i, k - k_prime) + n);
	   }
	}
	fprintf (stderr, "Help with %d\n", k);
	return 0;
}

