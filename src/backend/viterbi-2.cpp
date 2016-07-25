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
#include "viterbi-2.h"
/*
 *	The viterbi algorithm has been recoded for fitness in the
 *	SDR-J DAB framwork.
 *	This version is "codeword oriented", i.e. aiming at
 *	translating codewords of a given size.
 */
#define		K	7
//#define		Poly1	91
#define		Poly1	0133
//#define		Poly2	121
#define		Poly2	0171
//#define		Poly3	101
#define		Poly3	0145
//#define		Poly4	91
#define		Poly4	0133
#define	NumofStates	(1 << (K - 1))

static	int16_t	predecessor_for_0 [NumofStates];
static	int16_t predecessor_for_1 [NumofStates];

	viterbi_2::viterbi_2 (int16_t blockLength) {
int16_t	i, j;

	this	-> blockLength	= blockLength;
//
	transCosts		= new int32_t *[blockLength + 6 + 1];
	history			= new int16_t *[blockLength + 6 + 1];
	sequence		= new int16_t  [blockLength + 6 + 1];
//
	for (i = 0; i <= blockLength + 6; i++) {
	   transCosts [i]	= new int32_t [NumofStates];
	   history [i]		= new int16_t [NumofStates];
	   sequence [i] 	= 0;
	   for (j = 0; j < NumofStates; j ++) {
	      transCosts [i][j] = 0;
	      history [i][j] = 0;
	   }
	}
//
//	These tables give a mapping from (state * bit * Poly -> outputbit)
	poly1_table	= new uint8_t [2 * NumofStates];
	for (i = 0; i < 2; i ++)
	   for (j = 0; j < NumofStates; j ++) 
	      poly1_table [i * NumofStates + j] = bitFor (j, Poly1, i);

	for (i = 0; i < 2 * NumofStates - 1; i ++)
	   fprintf (stderr, "%d %d\n", i, poly1_table (i);
	poly2_table	= new uint8_t [2 * NumofStates];
	for (i = 0; i < 2; i ++)
	   for (j = 0; j < NumofStates; j ++)
	      poly2_table [i * NumofStates + j] = bitFor (j, Poly2, i);

	poly3_table	= new uint8_t [2 * NumofStates];
	for (i = 0; i < 2; i ++)
	   for (j = 0; j < NumofStates; j ++)
	      poly3_table [i * NumofStates + j] = bitFor (j, Poly3, i);

	poly4_table	= new uint8_t [2 * NumofStates];
	for (i = 0; i < 2; i ++)
	   for (j = 0; j < NumofStates; j ++)
	      poly4_table [i * NumofStates + j] = bitFor (j, Poly4, i);

	table5		= new uint8_t [2 * NumofStates];
	for (i = 0; i < 2 * NumofStates; i ++)
	   table5 [i] = (poly1_table [i] << 3) |
	                (poly2_table [i] << 2) |
	                (poly3_table [i] << 1) |
	                poly4_table [i];

	for (i = 0; i < NumofStates; i ++) {
	   predecessor_for_0 [i] = ((i << 1) + 00) & (NumofStates - 1);
	   predecessor_for_1 [i] = ((i << 1) + 01) & (NumofStates - 1);
	}
}

	viterbi_2::~viterbi_2 () {
int32_t i;
	for (i = 0; i < blockLength + 6 + 1; i++) {
	   delete[] transCosts [i];
	   delete[] history [i];
	}

	delete[]	sequence;
	delete[]	transCosts;
	delete[]	history;
	delete		poly1_table;
	delete		poly2_table;
	delete		poly3_table;
	delete		poly4_table;
	delete		table5;
}
//
//	sym is the sequence of soft bits
//	its length = 4 * blockLength + 4 * 6
void	viterbi_2::deconvolve (int16_t *sym, uint8_t *out) {
uint16_t	cState, bestState;
uint16_t 	i;
uint16_t	prev_0, prev_1;
int32_t 	costs_0, costs_1;
int32_t		minimalCosts;
//
//	first step is to "pump" the soft bits into the state machine
//	and compute the cost matrix.
//	we assume the overall costs for state 0 are zero
//	and remain zero
	for (i = 1; i <= blockLength + 6; i ++) {
	   for (cState = 0; cState < NumofStates / 2; cState ++) {
//	      uint8_t entrybit =  0;
	      prev_0	= predecessor_for_0 [cState];
	      prev_1	= predecessor_for_1 [cState];

//	we compute the minimal costs, based on the costs of the
//	prev states, and the additional costs of arriving from
//	the previous state to the current state with the symbol "sym"
//
//	entrybit = 0, so the index for the cost function is prev_xx
	      costs_0 = transCosts [i - 1][prev_0] +
	                costsFor (prev_0, &sym [4 * (i - 1)]);
	      costs_1 = transCosts [i - 1][prev_1] +
	                costsFor (prev_1, &sym [4 * (i - 1)]);
	      if (costs_0 < costs_1) {
	         transCosts [i][cState] = costs_0;
	         history [i][cState] = prev_0;
	      } else {
	         transCosts [i][cState] = costs_1;
	         history [i][cState] = prev_1;
	      }
	   }
	   for (cState = NumofStates / 2; cState < NumofStates; cState ++) {
//	      uint8_t entrybit = 1;
	      prev_0	= predecessor_for_0 [cState];
	      prev_1	= predecessor_for_1 [cState];

//	we compute the minimal costs, based on the costs of the
//	prev states, and the additional costs of arriving from
//	the previous state to the current state with the symbol "sym"
//
//	entrybit is here "1", so the index is id cost function
//	is prev_xx + NumofStates
	      costs_0 = transCosts [i - 1][prev_0] +
	                costsFor (prev_0 + NumofStates, &sym [4 * (i - 1)]);
	      costs_1 = transCosts [i - 1][prev_1] +
	                costsFor (prev_1 + NumofStates, &sym [4 * (i - 1)]);
	      if (costs_0 < costs_1) {
	         transCosts [i][cState] = costs_0;
	         history [i][cState] = prev_0;
	      } else {
	         transCosts [i][cState] = costs_1;
	         history [i][cState] = prev_1;
	      }
	   }
	}
//
//	Once all costs are computed, we can look for the minimal cost
//	Our "end state" is somewhere in column blockLength + 6
	minimalCosts	= INT_MAX;
	bestState	= 0;
	for (i = 0; i < NumofStates; i++) {
	   if (transCosts [blockLength][i] < minimalCosts) {
	      minimalCosts = transCosts [blockLength][i];
	      bestState = i;
	   }
	}
	sequence [blockLength] = bestState;
/*
 *	Trace backgoes back to state 0, and builds up the
 *	sequence of decoded symbols
 */
	for (i = blockLength + 6; i > 0; i --) 
	   sequence [i - 1] = history [i][sequence[i]];

	for (i = 1; i <= blockLength; i++) 
	   out [i - 1] = (sequence [i] >= NumofStates / 2) ? 01 : 00;
}
//
//	Note that the soft bits are such that
//	they are int16_t -255 -> (bit)1, +255 -> (bit)0
//	sym [0] is the "first"  of 4 soft bits
//	for optimization purposes, we ask the user to provide the
//	right index for the various tables (i.e. "state"  or, if the
//	entrybit = 1 "state + NumofStates"
int16_t	viterbi_2::costsFor (uint16_t lIndex, int16_t *sym) {
int32_t		res		= 0;
uint8_t		targetBit	= 0;
//
//	The "switch" turned out to be far less efficient than
//	the "if ... else"
//	switch (table5 [lIndex] & 017) {
//	   default:	// cannot happen
//	   case 0:	// 0 0 0 0
//	      return 1204 - sym [0] - sym [1] - sym [2] - sym [3];
//
//	   case 1:	// 0 0 0 1
//	      return 1204 - sym [0] - sym [1] - sym [2] + sym [3];
//
//	   case 2:	// 0 0 1 0
//	      return 1204 - sym [0] - sym [1] + sym [2] - sym [3];
//
//	   case 3:	// 0 0 1 1
//	      return 1204 - sym [0] - sym [1] + sym [2] + sym [3];
//
//	   case 4:	// 0 1 0 0
//	      return 1204 - sym [0] + sym [1] - sym [2] - sym [3];
//
//	   case 5:	// 0 1 0 1
//	      return 1204 - sym [0] + sym [1] - sym [2] + sym [3];
//
//	   case 6:	// 0 1 1 0
//	      return 1204 - sym [0] + sym [1] + sym [2] - sym [3];
//
//	   case 7:	// 0 1 1 1
//	      return 1204 - sym [0] + sym [1] + sym [2] + sym [3];
//
//	   case 010:	// 1 0 0 0
//	      return 1204 + sym [0] - sym [1] - sym [2] - sym [3];
//
//	   case 011:	// 1 0 0 1
//	      return 1204 + sym [0] - sym [1] - sym [2] + sym [3];
//
//	   case 012:	// 1 0 1 0
//	      return 1204 + sym [0] - sym [1] + sym [2] - sym [3];
//
//	   case 013:	// 1 0 1 1
//	      return 1204 + sym [0] - sym [1] + sym [2] + sym [3];
//
//	   case 014:	// 1 1 0 0
//	      return 1204 + sym [0] + sym [1] - sym [2] - sym [3];
//
//	   case 015:	// 1 1 0 1
//	      return 1204 + sym [0] + sym [1] - sym [2] + sym [3];
//
//	   case 016:	// 1 1 1 0
//	      return 1204 + sym [0] + sym [1] + sym [2] - sym [3];
//
//	   case 017:	// 1 1 1 1
//	      return 1204 + sym [0] + sym [1] + sym [2] + sym [3];
//	}

//
//	sym [0] -> 256 is a 0
//	sym [0] -> -256 is a 1

	targetBit = poly1_table [lIndex];
	if (targetBit == 0)
	   res += 256 - sym [0];
	else
	   res += 256 + sym [0];

	targetBit = poly2_table [lIndex];
	if (targetBit == 0)
	   res += 256 - sym [1];
	else
	   res += 256 + sym [1];

	targetBit = poly3_table [lIndex];
	if (targetBit == 0)
	   res += 256 - sym [2];
	else
	   res += 256 + sym [2];

	targetBit = poly4_table [lIndex];
	if (targetBit == 0)
	   res += 256 - sym [3];
	else
	   res += 256 + sym [3];

	return res;
}
/*
 *	as an aid, we give a function "bitFor" that, given
 *	the register state, the polynome and the bit to be inserted
 *	returns the bit coming from the engine
 */
uint8_t	viterbi_2::bitFor (uint16_t state, uint16_t poly, uint8_t bit) {
uint16_t Register;
uint8_t	resBit = 0;
int16_t	i;
//
//	the register after shifting "bit" in would be:
	Register = bit == 0 ? state : (state + NumofStates);
	Register &= poly;
/*
 *	now for the individual bits
 */
	for (i = 0; i <= K; i++) {
	   resBit ^= (Register & 01);
	   Register >>= 1;
	}

	return resBit;
}

