#
/*
 *    Copyright (C) 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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

#include	"fic-handler.h"
#include	"gui.h"
#include	"msc-handler.h"
#include	"fib-processor.h"
#include	"protTables.h"
//
//	The 3072 bits of the serial motherword shall be split into
//	24 blocks of 128 bits each.
//	The first 21 blocks shall be subjected to
//	puncturing (per 32 bits) according to PI_16
//	The next three blocks shall be subjected to 
//	puncturing (per 32 bits) according to PI_15
//	The last 24 bits shall be subjected to puncturing
//	according to the table X
static
uint8_t PI_X [24] = {
	1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0,
	1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0
};

/**
  *	\class ficHandler
  * 	We get in - through get_ficBlock - the FIC data
  * 	in units of 768 bits.
  * 	We follow the standard and apply conv coding and
  * 	puncturing.
  *	The data is sent through to the fic processor
  */
		ficHandler::ficHandler (RadioInterface *mr):viterbi (768) {
int16_t	i, j;
	bitBuffer_out	= new uint8_t [768];
	ofdm_input 	= new int16_t [2304];
	index		= 0;
	BitsperBlock	= 2 * 1536;
	ficno		= 0;
	ficBlocks	= 0;
	ficMissed	= 0;
	ficRatio	= 0;
	PI_15		= get_PCodes (15 - 1);
	PI_16		= get_PCodes (16 - 1);
	fibProcessor	= new fib_processor	(mr);
	memset (shiftRegister, 1, 9);

	for (i = 0; i < 768; i ++) {
	   PRBS [i] = shiftRegister [8] ^ shiftRegister [4];
	   for (j = 8; j > 0; j --)
	      shiftRegister [j] = shiftRegister [j - 1];

	   shiftRegister [0] = PRBS [i];
	}

	connect (this, SIGNAL (show_ficCRC (bool)),
	         mr, SLOT (show_ficCRC (bool)));
}

		ficHandler::~ficHandler (void) {
	        delete	bitBuffer_out;
	        delete	ofdm_input;
	        delete	fibProcessor;
}

	
/**
  *	\brief setBitsperBlock
  *	The number of bits to be processed per incoming block
  *	is 2 * p -> K, which still depends on the Mode.
  *	for Mode I it is 2 * 1536, for Mode II, it is 2 * 384,
  *	for Mode III it is 192, Mode IV gives 2 * 768.
  *	for Mode II we will get the 2304 bits after having read
  *	the 3 FIC blocks,
  *	for Mode IV we will get 3 * 2 * 768 = 4608, i.e. two resulting blocks
  *	Note that Mode III is NOT supported
  */

void	ficHandler::setBitsperBlock	(int16_t b) {
	if ((b == 2 * 384) ||
	    (b == 2 * 768) ||
	    (b == 2 * 1536))
	BitsperBlock	= b;
	index		= 0;
	ficno		= 0;
}

/**
  *	\brief process_ficBlock
  *	The number of bits to be processed per incoming block
  *	is 2 * p -> K, which still depends on the Mode.
  *	for Mode I it is 2 * 1536, for Mode II, it is 2 * 384,
  *	for Mode III it is 192, Mode IV gives 2 * 768.
  *	for Mode II we will get the 2304 bits after having read
  *	the 3 FIC blocks, each with 768 bits.
  *	for Mode IV we will get 3 * 2 * 768 = 4608, i.e. two resulting blocks
  *	Note that Mode III is NOT supported
  *	
  *	The function is called with a blkno. This should be 1, 2 or 3
  *	for each time 2304 bits are in, we call process_ficInput
  */
void	ficHandler::process_ficBlock (int16_t *data,
	                              int16_t blkno) {
int32_t	i;

	if (blkno == 1) {
	   index = 0;
	   ficno = 0;
	}
//
	if ((1 <= blkno) && (blkno <= 3)) {
	   for (i = 0; i < BitsperBlock; i ++) {
	      ofdm_input [index ++] = data [i];
	      if (index >= 2304) {
	         process_ficInput (ofdm_input, ficno);
	         index = 0;
	         ficno ++;
	      }
	   }
	}
	else
	   fprintf (stderr, "You should not call ficBlock here\n");
//	we are pretty sure now that after block 4, we end up
//	with index = 0
}

/**
  *	\brief process_ficInput
  *	we have a vector of 2304 (0 .. 2303) soft bits that has
  *	to be de-punctured and de-conv-ed into a block of 768 bits
  *	In this approach we first create the full 3072 block (i.e.
  *	we first depuncture, and then we apply the deconvolution
  *	In the next coding step, we will combine this function with the
  *	one above
  */
void	ficHandler::process_ficInput (int16_t *ficblock,
	                              int16_t ficno) {
int16_t	input_counter	= 0;
int16_t	i, k;
int32_t	local		= 0;
int16_t	viterbiBlock [3072 + 24];

/**
  *	a block of 2304 bits is considered to be a codeword
  *	In the first step we have 21 blocks with puncturing according to PI_16
  *	each 128 bit block contains 4 subblocks of 32 bits
  *	on which the given puncturing is applied
  */
	for (i = 0; i < 21; i ++) {
	   for (k = 0; k < 32 * 4; k ++) {
	      if (PI_16 [k % 32] == 1)  
	         viterbiBlock [local] = ficblock [input_counter ++];
	      else
	         viterbiBlock [local] = 0;	// a real "do not know"
	      local ++;
	   }
	}
/**
  *	In the second step
  *	we have 3 blocks with puncturing according to PI_15
  *	each 128 bit block contains 4 subblocks of 32 bits
  *	on which the given puncturing is applied
  */
	for (i = 0; i < 3; i ++) {
	   for (k = 0; k < 32 * 4; k ++) {
	      if (PI_15 [k % 32] == 1)  
	         viterbiBlock [local ++] = ficblock [input_counter ++];
	      else
	         viterbiBlock [local ++] = 0;	// a real "do not know"
	      }
	}

/**
  *	we have a final block of 24 bits  with puncturing according to PI_X
  *	This block constitues the 6 * 4 bits of the register itself.
  */
	for (k = 0; k < 24; k ++) {
	   if (PI_X [k] == 1) {
	      viterbiBlock [local ++] = ficblock [input_counter ++];
	   }
	   else
	      viterbiBlock [local ++] = 0;
	}
/**
  *	Now we have the full word ready for deconvolution
  *	deconvolution is according to DAB standard section 11.2
  */
	deconvolve (viterbiBlock, bitBuffer_out);
/**
  *	if everything worked as planned, we now have a
  *	768 bit vector containing three FIB's
  *
  *	first step: energy dispersal according to the DAB standard
  *	We use a predefined vector PRBS
  */
	for (i = 0; i < 768; i ++)
	   bitBuffer_out [i] ^= PRBS [i];
/**
  *	each of the fib blocks is protected by a crc
  *	(we know that there are three fib blocks each time we are here
  *	we keep track of the successrate
  *	and show that per 100 fic blocks
  */
	for (i = ficno * 3; i < ficno * 3 + 3; i ++) {
	   uint8_t *p = &bitBuffer_out [(i % 3) * 256];
	   if (!check_CRC_bits (p, 256)) {
	      show_ficCRC (false);
	      continue;
	   }
	   show_ficCRC (true);
	   fibProcessor	-> process_FIB (p, ficno);
	}
//	fibProcessor	-> printActions (ficno);
}

void	ficHandler::clearEnsemble (void) {
	fibProcessor	-> clearEnsemble ();
}

uint8_t	ficHandler::kindofService	(QString &s) {
	return fibProcessor	-> kindofService (s);
}

void	ficHandler::dataforAudioService	(QString &s, audiodata *d) {
	fibProcessor	-> dataforAudioService (s, d);
}

void	ficHandler::dataforDataService	(QString &s, packetdata *d) {
	fibProcessor	-> dataforDataService (s, d);
}

int16_t	ficHandler::get_ficRatio (void) {
	return ficRatio;
}
