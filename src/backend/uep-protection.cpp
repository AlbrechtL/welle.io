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
 *
 * 	The deconvolution for both uep and eep
 */
#include	"dab-constants.h"
#include	"uep-protection.h"
#include	"protTables.h"

struct protectionProfile {
	int16_t	bitRate;
	int16_t	protLevel;
	int16_t L1;
	int16_t	L2;
	int16_t	L3;
	int16_t	L4;
	int16_t	PI1;
	int16_t	PI2;
	int16_t	PI3;
	int16_t	PI4;
} profileTable [] = {
	{32,	5,	3, 4, 17, 0,	5, 3, 2, -1},
	{32,	4,	3, 3, 18, 0,	11, 6, 5, -1},
	{32,	3,	3, 4, 14, 3,	15, 9, 6, 8},
	{32,	2,	3, 4, 14, 3,	22, 13, 8, 13},
	{32,	1,	3, 5, 13, 3,	24, 17, 12, 17},

	{48,	5,	4, 3, 26, 3,	5, 4, 2, 3},
	{48,	4,	3, 4, 26, 3,	9, 6, 4, 6},
	{48,	3,	3, 4, 26, 3,	15, 10, 6, 9},
	{48,	2,	3, 4, 26, 3,	24, 14, 8, 15},
	{48,	1,	3, 5, 25, 3,	24, 18, 13, 18},

	{64,	5,	6, 9, 31, 2,	5, 3, 2, 3},
	{64,	4,	6, 9, 33, 0,	11, 6, 6, -1},
	{64,	3,	6, 12, 27, 3,	16, 8, 6, 9},
	{64,	2,	6, 10, 29, 3,	23, 13, 8, 13},
	{64,	1,	6, 11, 28, 3,	24, 18, 12, 18},

	{80,	5,	6, 10, 41, 3,	6, 3, 2, 3},
	{80,	4,	6, 10, 41, 3,	11, 6, 5, 6},
	{80,	3,	6, 11, 40, 3,	16, 8, 6, 7},
	{80,	2,	6, 10, 41, 3,	23, 13, 8, 13},
	{80,	1,	6, 10, 41, 3,	24, 7, 12, 18},

	{96,	5,	7, 9, 53, 3,	5, 4, 2, 4},
	{96,	4,	7, 10, 52, 3,	9, 6, 4, 6},
	{96,	3,	6, 12, 51, 3,	16, 9, 6, 10},
	{96,	2,	6, 10, 53, 3,	22, 12, 9, 12},
	{96,	1,	6, 13, 50, 3,	24, 18, 13, 19},
//
//	Thanks to Kalle Riis, who found that the "112" was missing
	{112,	5,	14, 17, 50, 3,	5, 4, 2, 5},
	{112,	4,	11, 21, 49, 3,	9, 6, 4, 8},
	{112,	3,	11, 23, 47, 3,	16, 8, 6, 9},
	{112,	2,	11, 21, 49, 3,	23, 12, 9, 14},

	{128,	5,	12, 19, 62, 3,	5, 3, 2, 4},
	{128,	4,	11, 21, 61, 3,	11, 6, 5, 7},
	{128,	3,	11, 22, 60, 3,	16, 9, 6, 10},
	{128,	2,	11, 21, 61, 3,	22, 12, 9, 14},
	{128,	1,	11, 20, 62, 3,	24, 17, 13, 19},

	{160,	5,	11, 19, 87, 3,	5, 4, 2, 4},
	{160,	4,	11, 23, 83, 3,	11, 6, 5, 9},
	{160,	3,	11, 24, 82, 3,	16, 8, 6, 11},
	{160,	2,	11, 21, 85, 3,	22, 11, 9, 13},
	{160,	1,	11, 22, 84, 3,	24, 18, 12, 19},

	{192,	5,	11, 20, 110, 3,	6, 4, 2, 5},
	{192,	4,	11, 22, 108, 3,	10, 6, 4, 9},
	{192,	3,	11, 24, 106, 3, 16, 10, 6, 11},
	{192,	2,	11, 20, 110, 3, 22, 13, 9, 13},
	{192,	1,	11, 21, 109, 3,	24, 20, 13, 24},

	{224,	5,	12, 22, 131, 3,	8,  6, 2, 6},
	{224,	4,	12, 26, 127, 3,	12, 8, 4, 11},
	{224,	3,	11, 20, 134, 3, 16, 10, 7, 9},
	{224,	2,	11, 22, 132, 3,	24, 16, 10, 15},
	{224,	1,	11, 24, 130, 3,	24, 20, 12, 20},

	{256,	5,	11, 24, 154, 3,	6, 5, 2, 5},
	{256,	4,	11, 24, 154, 3,	12, 9, 5, 10},
	{256,	3,	11, 27, 151, 3,	16, 10, 7, 10},
	{256,	2,	11, 22, 156, 3,	24, 14, 10, 13},
	{256,	1,	11, 26, 152, 3,	24, 19, 14, 18},

	{320,	5,	11, 26, 200, 3,	8, 5, 2, 6},
	{320,	4,	11, 25, 201, 3,	13, 9, 5, 10},
	{320,	2,	11, 26, 200, 3,	24, 17, 9, 17},
	
	{384,	5,	11, 27, 247, 3,	8, 6, 2, 7},
	{384,	3,	11, 24, 250, 3,	16, 9, 7, 10},
	{384,	1,	12, 28, 245, 3,	24, 20, 14, 23},
	{0,	-1,	-1, -1, -1, -1,	-1, -1, -1, -1}
};

static
int16_t	findIndex (int16_t bitRate, int16_t protLevel) {
int16_t	i;

	for (i = 0; profileTable [i].bitRate != 0; i ++)
	   if ((profileTable [i]. bitRate == bitRate) &&
	       (profileTable [i]. protLevel == protLevel))
	      return i;

	return -1;
}

/**
  *	the table is based on chapter 11 of the DAB standard.
  *
  *	\brief uep_deconvolve
  *
  *	The bitRate and the protectionLevel determine the 
  *	depuncturing scheme.
  */
	uep_protection::uep_protection (int16_t bitRate,
	                                int16_t protLevel):viterbi (24 * bitRate) {
int16_t	index;

	this	-> bitRate		= bitRate;
	index	= findIndex (bitRate, protLevel);
	if (index == -1) {
	   fprintf (stderr, "%d (%d) has a problem\n", bitRate, protLevel);
	   index = 1;
	}
	outSize		= 24 * bitRate;
	viterbiBlock	= new int16_t [outSize * 4 + 24];
	L1	= profileTable [index]. L1;
	L2	= profileTable [index]. L2;
	L3	= profileTable [index]. L3;
	L4	= profileTable [index]. L4;

	PI1	= get_PCodes (profileTable [index]. PI1 -1);
	PI2	= get_PCodes (profileTable [index]. PI2 -1);
	PI3	= get_PCodes (profileTable [index]. PI3 -1);
	if ((profileTable [index]. PI4 - 1) != -1)
	   PI4	= get_PCodes (profileTable [index]. PI4 -1);
	else
	   PI4	= NULL;
}

	uep_protection::~uep_protection (void) {
	delete[]	viterbiBlock;
}

bool	uep_protection::deconvolve (int16_t *v,
	                            int32_t size, uint8_t *outBuffer) {
int16_t	i, j;
int16_t	inputCounter	= 0;
int32_t	viterbiCounter	= 0;
	(void)size;			// currently unused

//	according to the standard we process the logical frame
//	with a pair of tuples
//	(L1, PI1), (L2, PI2), (L3, PI3), (L4, PI4)

///	clear the bits in the viterbiBlock,
///	only the non-punctured ones are set
	memset (viterbiBlock, 0, (outSize * 4 + 24) * sizeof (int16_t)); 
	for (i = 0; i < L1; i ++) {
	   for (j = 0; j < 128; j ++) {
	      if (PI1 [j % 32] == 1) 
	         viterbiBlock [viterbiCounter] = v [inputCounter ++];
	      else
	         viterbiBlock [viterbiCounter] = 128;
	      viterbiCounter ++;
	   }
	}

	for (i = 0; i < L2; i ++) {
	   for (j = 0; j < 128; j ++) {
	      if (PI2 [j % 32] == 1) 
	         viterbiBlock [viterbiCounter] = v [inputCounter ++];
	      else
	         viterbiBlock [viterbiCounter] = 128;
	      viterbiCounter ++;
	   }
	}

	for (i = 0; i < L3; i ++) {
	   for (j = 0; j < 128; j ++) {
	      if (PI3 [j % 32] == 1) 
	         viterbiBlock [viterbiCounter] = v [inputCounter ++];
	      else
	         viterbiBlock [viterbiCounter] = 128;
	      viterbiCounter ++;	
	   }
	}

	for (i = 0; i < L4; i ++) {
	   for (j = 0; j < 128; j ++) {
	      if (PI4 [j % 32] == 1) 
	         viterbiBlock [viterbiCounter] = v [inputCounter ++];
	      else
	         viterbiBlock [viterbiCounter] = 128;
	      viterbiCounter ++;	
	   }
	}

/**
  *	we have a final block of 24 bits  with puncturing according to PI_X
  *	This block constitues the 6 * 4 bits of the register itself.
  */
	for (i = 0; i < 24; i ++) {
	   if (PI_X [i] == 1)  
	      viterbiBlock [viterbiCounter] = v [inputCounter ++];
	   else
	      viterbiBlock [viterbiCounter] = 128;
	   viterbiCounter ++;
	}
//
///	The actual deconvolution is done by the viterbi decoder

	viterbi::deconvolve (viterbiBlock, outBuffer);
	return true;
}
