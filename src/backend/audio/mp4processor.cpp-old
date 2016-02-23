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
 * 	superframer for the SDR-J DAB+ receiver
 * 	This processor handles the whole DAB+ specific part
 ************************************************************************
 *	may 15 2015. A real improvement on the code
 *	is the addition from Stefan Poeschel to create a
 *	header for the aac that matches, really a big help!!!!
 ************************************************************************
 */
#include	"mp4processor.h"
#include	<cstring>
#include	"gui.h"
//
#include	"charsets.h"
#include	"faad-decoder.h"
#include	"pad-handler.h"
/**
  *	\brief simple, inline coded, crc checker
  */
bool	dabPlus_crc (uint8_t *msg, int16_t len) {
int i, j;
uint16_t	accumulator	= 0xFFFF;
uint16_t	crc;
uint16_t	genpoly		= 0x1021;

	for (i = 0; i < len; i ++) {
	   int16_t data = msg [i] << 8;
	   for (j = 8; j > 0; j--) {
	      if ((data ^ accumulator) & 0x8000)
	         accumulator = ((accumulator << 1) ^ genpoly) & 0xFFFF;
	      else
	         accumulator = (accumulator << 1) & 0xFFFF;
	      data = (data << 1) & 0xFFFF;
	   }
	}
//
//	ok, now check with the crc that is contained
//	in the au
	crc	= ~((msg [len] << 8) | msg [len + 1]) & 0xFFFF;
	return (crc ^ accumulator) == 0;
}

/**
  *	\class mp4Processor is the main handler for the aac frames
  *	the class proper processes input and extracts the aac frames
  *	that are processed by the "faadDecoder" class
  */
	mp4Processor::mp4Processor (RadioInterface	*mr,
	                            audioSink	*as,
	                            FILE	*errorLog,
	                            int16_t	bitRate):my_padhandler (mr) {

	myRadioInterface	= mr;
	connect (this, SIGNAL (show_successRate (int)),
	         mr, SLOT (show_successRate (int)));
	connect (this, SIGNAL (showLabel (QString)),
	         mr, SLOT (showLabel (QString)));
	ourSink			= as;
	this	-> bitRate	= bitRate;	// input rate

	superFramesize		= 110 * (bitRate / 8);
	RSDims			= bitRate / 8;
	frameBytes		= new uint8_t [RSDims * 120];	// input
	outVector		= new uint8_t [RSDims * 110];
	blockFillIndex	= 0;
	blocksInBuffer	= 0;
//
	aacDecoder		= new faadDecoder (ourSink);
	frameCount	= 0;
	frameErrors	= 0;
//
//	error display
	au_count	= 0;
	au_errors	= 0;
	this	-> errorLog	= errorLog;
}

	mp4Processor::~mp4Processor (void) {
	delete		aacDecoder;
	delete[]	frameBytes;
	delete[]	outVector;
}

/**
  *	\brief addtoFrame
  *
  *	a DAB+ superframe consists of 5 consecutive DAB frames 
  *	we add vector for vector to the superframe. Once we have
  *	5 lengths of "old" frames, we check
  *	Note that the packing in the entry vector is still one bit
  *	per Byte, nbits is the number of Bits (i.e. containing bytes)
  *	the function adds nbits bits, packed in bytes, to the frame
  */
void	mp4Processor::addtoFrame (uint8_t *V, int16_t nbits) {
int16_t	i, j;
uint8_t	temp	= 0;

	for (i = 0; i < nbits / 8; i ++) {	// in bytes
	   temp = 0;
	   for (j = 0; j < 8; j ++)
	      temp = (temp << 1) | (V [i * 8 + j] & 01);
	   frameBytes [blockFillIndex * nbits / 8 + i] = temp;
	}
//
	blocksInBuffer ++;
	blockFillIndex = (blockFillIndex + 1) % 5;
//
/**
  *	we take the last five blocks to look at
  */
	if (blocksInBuffer >= 5) {
///	first, we show the "successrate"
	   if (++frameCount >= 25) {
	      frameCount = 0;
	      show_successRate (4 * (25 - frameErrors));
	      frameErrors = 0;
	   }

/**
  *	starting for real: check the fire code
  *	if the firecode is OK, we handle the frame
  *	and adjust the buffer here for the next round
  */
	   if (fc. check (&frameBytes [blockFillIndex * nbits / 8]) &&
	       (processSuperframe (frameBytes,
	                           blockFillIndex * nbits / 8))) {
//	since we processed a full cycle of 5 blocks, we just start a
//	new sequence, beginning with block blockFillIndex
	      blocksInBuffer	= 0;
	   }
	   else {
/**
  *	we were wrong, virtual shift to left in block sizes
  */
	      blocksInBuffer  = 4;
	      frameErrors ++;
	   }
	}
}

/**
  *	\brief processSuperframe
  *
  *	First, we we that the firecode checker gace green light
  *	However, that does not mean that the values defining the
  *	the sizes and positions of the aac frames are correct
  *	When we see an obvious incorrect value, we let the
  *	function fail and report that in the errorLog file
  *	(which is usually /dev/null).
  */
bool	mp4Processor::processSuperframe (uint8_t frameBytes [], int16_t bo) {
uint8_t		num_aus;
int16_t		i, j, k;
int16_t		nErrors	= 0;
uint8_t		rsIn	[120];
uint8_t		rsOut	[110];
uint8_t		dacRate;
uint8_t		sbrFlag;
uint8_t		aacChannelMode;
uint8_t		psFlag;
uint16_t	mpegSurround;
int32_t		outSamples	= 0;
int32_t		tmp;

//	bits 0 .. 15 is firecode
//	bit 16 is unused
	dacRate		= (frameBytes [bo + 2] >> 6) & 01;	// bit 17
	sbrFlag		= (frameBytes [bo + 2] >> 5) & 01;	// bit 18
	aacChannelMode	= (frameBytes [bo + 2] >> 4) & 01;	// bit 19
	psFlag		= (frameBytes [bo + 2] >> 3) & 01;	// bit 20
	mpegSurround	= (frameBytes [bo + 2] & 07);		// bits 21 .. 23

	switch (2 * dacRate + sbrFlag) {
	   default:		// cannot happen
	      fprintf (errorLog, "serious error in frame 1 (%d) (%d)\n",
	                                          ++au_errors, au_count);
	      return false;

	   case 0:
	      num_aus = 4;
	      au_start [0] = 8;
	      au_start [1] = frameBytes [bo + 3] * 16 +
	                     (frameBytes [bo + 4] >> 4);
	      au_start [2] = (frameBytes [bo + 4] & 0xf) * 256 +
	                      frameBytes [bo + 5];
	      au_start [3] = frameBytes [bo + 6] * 16 +
	                     (frameBytes [bo + 7] >> 4);
	      au_start [4] = 110 *  (bitRate / 8);
	      if ((au_start [3] >= au_start [4]) ||
                  (au_start [2] >= au_start [3]) ||
                  (au_start [1] >= au_start [2])) {
                 fprintf (errorLog, "serious error in frame 2 (%d) (%d) %d %d %d %d %d\n",
	                                         ++au_errors, au_count,
	                               au_start [0],
	                               au_start [1],
	                               au_start [2],
	                               au_start [3],
	                               au_start [4]);
                 return false;
              }
	      break;
//
	   case 1:
	      num_aus = 2;
	      au_start [0] = 5;
	      au_start [1] = frameBytes [bo + 3] * 16 +
	                     (frameBytes [bo + 4] >> 4);
	      au_start [2] = 110 *  (bitRate / 8);
	      if ((au_start [1] < au_start [0]) ||
	          (au_start [1] >= au_start [2])) {
	         fprintf (errorLog, "serious error in frame 3 (%d) (%d) %d %d %d\n",
	                                         ++au_errors, au_count,
	                              au_start [0],
	                              au_start [1],
	                              au_start [2]);
;
	         return false;
	      }
	      break;
//
	   case 2:
	      num_aus = 6;
	      au_start [0] = 11;
	      au_start [1] = frameBytes [bo + 3] * 16 +
	                     (frameBytes [bo + 4] >> 4);
	      au_start [2] = (frameBytes [bo + 4] & 0xf) * 256 +
	                     frameBytes [bo + 5];
	      au_start [3] = frameBytes [bo + 6] * 16 +
	                     (frameBytes [bo + 7] >> 4);
	      au_start [4] = (frameBytes [bo + 7] & 0xf) * 256 +
	                     frameBytes [bo + 8];
	      au_start [5] = frameBytes [bo + 9] * 16 +
	                     (frameBytes [bo + 10] >> 4);
	      au_start [6] = 110 *  (bitRate / 8);

	      if ((au_start [5] >= au_start [6]) ||
                  (au_start [4] >= au_start [5]) ||
                  (au_start [3] >= au_start [4]) ||
                  (au_start [2] >= au_start [3]) ||
                  (au_start [1] >= au_start [2])) {
                 fprintf (errorLog, "serious error in frame 4 (%d) (%d) %d %d %d %d %d %d %d\n",
	                                          ++au_errors, au_count,
	                          au_start [0],
	                          au_start [1],
	                          au_start [2],
	                          au_start [3],
	                          au_start [4],
	                          au_start [5],
	                          au_start [6]);
                 return false;
              }
	      break;
//
	   case 3:
	      num_aus = 3;
	      au_start [0] = 6;
	      au_start [1] = frameBytes [bo + 3] * 16 +
	                     (frameBytes [bo + 4] >> 4);
	      au_start [2] = (frameBytes [bo + 4] & 0xf) * 256 +
	                     frameBytes [bo + 5];
	      au_start [3] = 110 * (bitRate / 8);
	      if ((au_start [2] >= au_start [3]) ||
                  (au_start [1] >= au_start [2])) {
                 fprintf (errorLog, "serious error in frame 5 (%d) (%d) %d %d %d %d\n",
	                                         ++au_errors, au_count,
	                          au_start [0],
	                          au_start [1],
	                          au_start [2],
	                          au_start [3]);
                 return false;
              }
	      break;
	}
/**
  *	apply reed-solomon error repar
  *	OK, what we now have is a vector with RSDims * 120 uint8_t's
  *	the superframe, containing parity bytes for error repair
  *	take into account the interleaving that is applied.
  */
	for (j = 0; j < RSDims; j ++) {
	   for (k = 0; k < 120; k ++) 
	      rsIn [k] = frameBytes [(bo + j + k * RSDims) % (RSDims * 120)];
	   nErrors += rsDecoder. dec (rsIn, rsOut);
	   for (k = 0; k < 110; k ++)
	      outVector [j + k * RSDims] = rsOut [k];
	}
/**
  *	OK, the result is N * 110 * 8 bits (still single bit per byte!!!)
  *	extract the AU's, and prepare a buffer,  with the sufficient
  *	lengthy for conversion to PCM samples
  */
	for (i = 0; i < num_aus; i ++) {
	   int16_t	aac_frame_length;
	   au_count ++;
	   uint8_t theAU [2 * 960 + 10];	// sure, large enough
	   memset (theAU, 0, sizeof (theAU));

///	sanity check 1
	   if (au_start [i + 1] < au_start [i]) {
	      fprintf (errorLog, "%d %d\n", au_start [i + 1], au_start [i]);
	      return false;
	   }

/**
  *	Note that the length can be erroneous, so we check
  *	its sanity
  */
	   aac_frame_length = au_start [i + 1] - au_start [i] - 2;
	   if ((aac_frame_length >= 2 * 960) || (aac_frame_length < 0)) {
	      fprintf (errorLog, "serious error in frame 6 (%d) (%d) frame_length = %d\n",
	                                        ++au_errors,
	                                        au_count, aac_frame_length);
	      return false;
	   }
///	but first the crc check
	   if (dabPlus_crc (&outVector [au_start [i]],
	                    aac_frame_length)) {
/**
  *	create a real aac vector, starting with the newly created
  *	header
  */
	      memcpy (theAU,
	              &outVector [au_start [i]],
	              aac_frame_length * sizeof (uint8_t));
/**
  *	see if we have a PAD
  */
	      if (((theAU [0] >> 5) & 07) == 4)
	         my_padhandler. processPAD (theAU);
/**
  *	just a few bytes extra, such that the decoder can look
  *	beyond the last byte
  */
	      for (j = aac_frame_length;
	           j < aac_frame_length + 10; j ++)
	         theAU [j] = 0;
	      tmp = aacDecoder -> MP42PCM (dacRate,
	                                   sbrFlag,
	                                   mpegSurround,
	                                   aacChannelMode,
	                                   theAU,
	                                   aac_frame_length);
	      if (tmp == 0)
	         frameErrors ++;
	      else
	         outSamples += tmp;
	   }
	   else {
	      au_errors ++;
	      fprintf (errorLog, "CRC failure with dab+ frame (error %d)\n",
	                                          au_errors);
	   }
	}
//	fprintf (stderr, "%d samples good for %d nsec of music\n",
//	                 outSamples, outSamples * 1000 / 48);
//
	return true;
}

