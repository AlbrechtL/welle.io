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
 * 	The eep handling
 */
#include	"dab-constants.h"
#include	"eep-protection.h"
#include	"protTables.h"

/**
  *	\brief eep_deconvolve
  *	equal error protection, bitRate and protLevel
  *	define the puncturing table
  */
	eep_protection::eep_protection (int16_t bitRate,
	                                int16_t protLevel):viterbi (24 * bitRate) {
	this	-> bitRate = bitRate;
	outSize		= 24 * bitRate;
	viterbiBlock	= new int16_t [outSize * 4 + 24];
	if (protLevel & 0100) {	// set A profiles
	   switch (protLevel & 07) {
	      case 1:
	         L1	= 6 * bitRate / 8 - 3;
	         L2	= 3;
	         PI1	= get_PCodes (24 - 1);
	         PI2	= get_PCodes (23 - 1);
	         break;

	      case 2:
	         if (bitRate == 8) {
	            L1	= 5;
	            L2	= 1;
	            PI1	= get_PCodes (13 - 1);
	            PI2	= get_PCodes (12 - 1);
	         } else {
	            L1	= 2 * bitRate / 8 - 3;
	            L2	= 4 * bitRate / 8 + 3;
	            PI1	= get_PCodes (14 - 1);
	            PI2	= get_PCodes (13 - 1);
	         }
	         break;

	      case 3:
	         L1	= 6 * bitRate / 8 - 3;
	         L2	= 3;
	         PI1	= get_PCodes (8 - 1);
	         PI2	= get_PCodes (7 - 1);
	         break;

	      case 4:
	         L1	= 4 * bitRate / 8 - 3;
	         L2	= 2 * bitRate / 8 + 3;
	         PI1	= get_PCodes (3 - 1);
	         PI2	= get_PCodes (2 - 1);
	         break;
	   }
	}
	else
	if (protLevel & 0200) {		// B series
	   switch ((protLevel & 07)) {
	      case 4:
	         L1	= 24 * bitRate / 32 - 3;
	         L2	= 3;
	         PI1	= get_PCodes (2 - 1);
	         PI2	= get_PCodes (1 - 1);
	         break;

	      case 3:
	         L1	= 24 * bitRate / 32 - 3;
	         L2	= 3;
	         PI1	= get_PCodes (4 - 1);
	         PI2	= get_PCodes (3 - 1);
	         break;

	      case 2:
	         L1	= 24 * bitRate / 32 - 3;
	         L2	= 3;
	         PI1	= get_PCodes (6 - 1);
	         PI2	= get_PCodes (5 - 1);
	         break;

	      case 1:
	         L1	= 24 * bitRate / 32 - 3;
	         L2	= 3;
	         PI1	= get_PCodes (10 - 1);
	         PI2	= get_PCodes (9 - 1);
	         break;
	   }
	}
}

	eep_protection::~eep_protection (void) {
	delete[]	viterbiBlock;
}

bool	eep_protection::deconvolve (int16_t *v,
	                            int32_t size, uint8_t *outBuffer) {

int16_t	i, j;
int32_t	inputCounter	= 0;
int32_t	viterbiCounter	= 0;
	(void)size;			// currently unused
	memset (viterbiBlock, 0, (outSize * 4 + 24) * sizeof (int16_t)); 
//
//	according to the standard we process the logical frame
//	with a pair of tuples
//	(L1, PI1), (L2, PI2), (L3, PI3), (L4, PI4)
//
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
//	we had a final block of 24 bits  with puncturing according to PI_X
//	This block constitues the 6 * 4 bits of the register itself.
	for (i = 0; i < 24; i ++) {
	   if (PI_X [i] == 1) 
	      viterbiBlock [viterbiCounter] = v [inputCounter ++];
	   else
	      viterbiBlock [viterbiCounter] = 128;
	   viterbiCounter ++;
	}

	viterbi::deconvolve (viterbiBlock, outBuffer);
	return true;
}

