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
 * 	superframer for the SDR-J DAB+ receiver
 * 	This processor handles the whole DAB+ specific part
 */
#include	"mp4processor.h"
#include	<cstring>
#include	"gui.h"
//
#include	"faad-decoder.h"

//	simple, inline coded, crc checker
//
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
//
//	and some simple "bit-setter" functions
static inline
void	setBit (uint8_t x [], uint8_t bit, int32_t pos) {
int16_t	iByte;
int16_t	iBit;

	iByte	= pos / 8;
	iBit	= pos % 8;
	x [iByte] = (x [iByte] & (~(1 << (7 - iBit)))) |
	            (bit << (7 - iBit));
}

static inline
void	setBits (uint8_t x[], uint32_t bits,
	         int32_t startPosition, int32_t numBits) {
int32_t i;
uint8_t	bit;

	for (i = 0; i < numBits; i ++) {
	   bit = bits & (1 << (numBits - i - 1)) ? 1 : 0;
	   setBit (x, bit, startPosition + i);
	}
}

//
//	Now for real
	mp4Processor::mp4Processor (RadioInterface *mr,
	                            audioSink *as,
	                            FILE *mp4file,
	                            int16_t bitRate,
	                            bool corrector) {
int16_t	i;

	myRadioInterface	= mr;
	connect (this, SIGNAL (show_successRate (int)),
	         mr, SLOT (show_successRate (int)));
	ourSink			= as;
	mp4File			= mp4file;
	this	-> bitRate	= bitRate;	// input rate

	superFramesize		= 110 * (bitRate / 8);
	RSDims			= bitRate / 8;
	frameBytes		= new uint8_t [RSDims * 120];	// input
	outVector		= new uint8_t [RSDims * 110];
	blockFillIndex	= 0;
	blocksInBuffer	= 0;
//
	aacDecoder		= new faadDecoder (ourSink, corrector);
//	header info for the aac frame
//	The frame structure is largely copied from the GNU radio files
//	(Thanks)
	d_fh.syncword		= 0xfff;
	d_fh.id			= 0;
	d_fh.layer		= 0;
	d_fh.protection_absent	= 1;
	d_fh.profile_objecttype	= 0;	// aac main - 1
	d_fh.private_bit	= 0;	// ignored when decoding
	d_fh.original_copy	= 0;
	d_fh.home		= 0;
	d_vh.copyright_id_bit	= 0;
	d_vh.copyright_id_start	= 0;
	d_vh.adts_buffer_fullness = 1999; // ? according to OpenDab
	d_vh.no_raw_data_blocks	= 0;

	d_header[0] 	= d_fh.syncword >> 4;
	d_header[1] 	= (d_fh.syncword & 0xf) << 4;
	d_header[1] 	|= d_fh.id << 3;
	d_header[1] 	|= d_fh.layer << 1;
	d_header[1] 	|= d_fh.protection_absent;
	d_header[2] 	= d_fh.profile_objecttype << 6;
//	sampling frequency index filled in dynamically
	d_header[2] 	|= d_fh.private_bit << 1;
//	channel configuration filled in dynamically
	d_header[3] 	= d_fh.original_copy << 5;
	d_header[3] 	|= d_fh.home << 4; 
	d_header[3] 	|= d_vh.copyright_id_bit << 3;
	d_header[3] 	|= d_vh.copyright_id_start << 2;
//	framelength filled in dynamically
	d_header[4] 	= 0;
	d_header[5] 	= d_vh.adts_buffer_fullness >> 6;
	d_header[6] 	= (d_vh.adts_buffer_fullness & 0x3f) << 2;
	d_header[6] 	|= d_vh.no_raw_data_blocks;
//
//	and finally
	frameCount	= 0;
	frameErrors	= 0;
//
//	error display
	au_count	= 0;
	au_errors	= 0;
}

	mp4Processor::~mp4Processor (void) {
	delete		aacDecoder;
	delete[]	frameBytes;
	delete[]	outVector;
}
//
//	we add vector for vector to the superframe. Once we have
//	5 lengths of "old" frames, we check
void	mp4Processor::addtoFrame (uint8_t *V, int16_t nbits) {
int16_t	i, j;
uint8_t	temp	= 0;
//
//	Note that the packing in the entry vector is still one bit
//	per Byte, nbits is the number of Bits (i.e. containing bytes)
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
//	we take the last five blocks to look at
	if (blocksInBuffer >= 5) {
	   if (++frameCount >= 25) {
	      frameCount = 0;
	      show_successRate (4 * (25 - frameErrors));
	      frameErrors = 0;
	   }

//	OK, we give it a try, check the fire code
	   if (fc. check (&frameBytes [blockFillIndex * nbits / 8]) &&
	       (processSuperframe (frameBytes,
	                           blockFillIndex * nbits / 8))) {
//	since we processed a full cycle of 5 blocks, we just start a
//	new sequence, beginning with block blockFillIndex
	      blocksInBuffer	= 0;
	   }
	   else {	// virtual shift to left in block sizes
	      blocksInBuffer  = 4;
	      frameErrors ++;
	   }
	}
}
//
bool	mp4Processor::processSuperframe (uint8_t frameBytes [], int16_t bo) {
uint8_t		num_aus;
int16_t		i, j, k;
uint8_t		byte;
uint8_t		temp;
int16_t		base;
int16_t		nErrors	= 0;
uint8_t		rfa;
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
	      d_fh. sampling_freq_idx	= 5;
	      break;

	   case 1:
	      num_aus = 2;
	      au_start [0] = 5;
	      au_start [1] = frameBytes [bo + 3] * 16 +
	                     (frameBytes [bo + 4] >> 4);
	      au_start [2] = 110 *  (bitRate / 8);
	      d_fh. sampling_freq_idx	= 8;
	      break;

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
	      d_fh. sampling_freq_idx	= 3;
	      break;

	   case 3:
	      num_aus = 3;
	      au_start [0] = 6;
	      au_start [1] = frameBytes [bo + 3] * 16 +
	                     (frameBytes [bo + 4] >> 4);
	      au_start [2] = (frameBytes [bo + 4] & 0xf) * 256 +
	                     frameBytes [bo + 5];
	      au_start [3] = 110 * (bitRate / 8);
	      d_fh. sampling_freq_idx	= 6;
	      break;
	}
//
	if (mpegSurround == 0) {
	   if (sbrFlag && !aacChannelMode && psFlag)
	      d_fh. channel_conf	= 2;
	   else
	      d_fh. channel_conf	= 1 << aacChannelMode;
	} else
	if (mpegSurround == 1) 
	   d_fh. channel_conf		= 6;
	else {
	   fprintf (stderr, "Unrecognized mpeg surround config (ignored)\n");
	   if (sbrFlag && !aacChannelMode && psFlag)
	      d_fh. channel_conf	= 2;
	   else
	      d_fh. channel_conf	= 1 << aacChannelMode;
	}

	setBits (&d_header [2], d_fh. sampling_freq_idx, 2, 4);
	setBits (&d_header [2], d_fh. channel_conf, 7, 3);
//
//	apply reed-solomon error repar
//	OK, what we now have is a vector with RSDims * 120 uint8_t's
//	the superframe, containing parity bytes for error repair
//	take into account the interleaving that is applied.
	for (j = 0; j < RSDims; j ++) {
	   for (k = 0; k < 120; k ++) 
	      rsIn [k] = frameBytes [(bo + j + k * RSDims) % (RSDims * 120)];
	   nErrors += rsDecoder. dec (rsIn, rsOut);
	   for (k = 0; k < 110; k ++)
	      outVector [j + k * RSDims] = rsOut [k];
	}
//
//	OK, the result is N * 110 * 8 bits (still single bit per byte!!!)
//	extract the AU's, and prepare a buffer, sufficiently
//	long for conversion to PCM samples
	for (i = 0; i < num_aus; i ++) {
	   au_count ++;
	   uint8_t hulpBuffer [960];	// sure, large enough
//
//	The crc takes the two last bytes from the au vector
//	add 7 bytes for the header (7 bytes since protection is absent)
	   d_vh. aac_frame_length = au_start [i + 1] - au_start [i] - 2 + 7;
//	but first the crc check
	   if (dabPlus_crc (&outVector [au_start [i]],
	                    d_vh. aac_frame_length - 7)) {
	      setBits (&d_header [3], d_vh. aac_frame_length, 6, 13);
//
//	create a real aac vector, starting with the newly created
//	header
	      memcpy (hulpBuffer, d_header, 7 * sizeof (uint8_t));
	      memcpy (&hulpBuffer [7],
	              &outVector [au_start [i]],
	              d_vh. aac_frame_length * sizeof (uint8_t));
//
//	add some empty bytes, allowing the decoder to look past the last
//	data byte
	      for (j = d_vh. aac_frame_length;
	           j < d_vh. aac_frame_length + 10; j ++)
	         hulpBuffer [j] = 0;
	      if (mp4File != NULL)
	         (void)fwrite (hulpBuffer,  sizeof (uint8_t),
	                       d_vh. aac_frame_length, mp4File);
	      else {
	         tmp = 
	           aacDecoder -> MP42PCM (hulpBuffer, d_vh. aac_frame_length);
	         if (tmp == 0)
	            frameErrors ++;
	         else
	            outSamples += tmp;
	      }
	   }
	   else {
	      au_errors ++;
//	      fprintf (stderr, "CRC failure with dab+ frame\n");
	   }
	}
//	fprintf (stderr, "%d samples good for %d nsec of music\n",
//	                 outSamples, outSamples * 1000 / 48);
//
	return true;
}

void	mp4Processor::setFile (FILE *f) {
	mp4File = f;
}

