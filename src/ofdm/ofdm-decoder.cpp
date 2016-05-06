#
/*
 *    Copyright (C) 2013 2015
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
 *	Once the bits are "in", interpretation and manipulation
 *	should reconstruct the data blocks.
 *	Ofdm_decoder is called once every Ts samples, and
 *	its invocation results in 2 * Tu bits
 */
#include	"ofdm-decoder.h"
#include	"gui.h"
#include	"phasetable.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"freq-interleaver.h"
#define	SYNCLENGTH	10
/**
  *	\brief ofdmDecoder
  *	The class ofdmDecoder is - when implemented in a separate thread -
  *	taking the data from the ofdmProcessor class in, and
  *	will extract the Tu samples, do an FFT and extract the
  *	carriers and map them on (soft) bits
  */
	ofdmDecoder::ofdmDecoder	(DabParams	*p,
	                                 RadioInterface *mr,
	                                 DSPCOMPLEX	*refTable,
	                                 ficHandler	*my_ficHandler,
	                                 mscHandler	*my_mscHandler) {
int16_t	i;
	this	-> params		= p;
	this	-> myRadioInterface	= mr;
	this	-> refTable		= refTable;
	this	-> my_ficHandler	= my_ficHandler;
	this	-> my_mscHandler	= my_mscHandler;
	this	-> T_s			= params	-> T_s;
	this	-> T_u			= params	-> T_u;
	this	-> carriers		= params	-> K;
	ibits				= new int16_t [2 * this -> carriers];

	this	-> T_g			= T_s - T_u;
	fft_handler			= new common_fft (T_u);
	fft_buffer			= fft_handler -> getVector ();
	phaseReference			= new DSPCOMPLEX [T_u];
	myMapper			= new interLeaver (params);
//
	connect (this, SIGNAL (show_snr (int)),
	         mr, SLOT (show_snr (int)));
	snrCount		= 0;
	snr			= 0;	

/**
  *	When implemented in a thread, the thread controls the
  *	reading in of the data and processing the data through
  *	functions for handling block 0, FIC blocks and MSC blocks.
  *
  *	We just create a large buffer where index i refers to block i.
  *
  */
	command			= new DSPCOMPLEX * [params -> L];
	for (i = 0; i < params -> L; i ++)
	   command [i] = new DSPCOMPLEX [T_u];
#ifdef	__BETTER_LOCK
	bufferResources		= new QSemaphore (params -> L);
#endif
	amount		= 0;
	start ();
}

	ofdmDecoder::~ofdmDecoder	(void) {
int16_t	i;
	running	= false;
	commandHandler. wakeAll ();
	usleep (1000);
	while (!isFinished () && isRunning ());
	   usleep (100);
	delete		fft_handler;
	delete[]	phaseReference;
	delete	myMapper;
	for (i = 0; i < params -> L; i ++)
	   delete[] command [i];
	delete[] command;
#ifdef	__BETTER_LOCK
	delete	bufferResources;
#endif
}

void	ofdmDecoder::stop		(void) {
	running = false;
	commandHandler. wakeAll ();
	while (isRunning ())
	   usleep (100);
}

//
//
/**
  *	The code in the thread executes a simple loop,
  *	waiting for the next block and executing the interpretation
  *	operation for that block.
  *	In our original code the block count was 1 higher than
  *	our count here.
  */
void	ofdmDecoder::run	(void) {
int16_t	currentBlock	= 0;

	running		= true;
	while (running) {
	   helper. lock ();
	   commandHandler. wait (&helper, 100);
	   helper. unlock ();
	   while ((amount > 0) && running) {
	      if (currentBlock == 0)
	         processBlock_0 ();
	      else
	      if (currentBlock < 4)
	         decodeFICblock (currentBlock);
	      else
	         decodeMscblock (currentBlock);
#ifdef	__BETTER_LOCK
	      bufferResources -> release (1);
#endif
	      helper. lock ();
	      currentBlock = (currentBlock + 1) % (params -> L);
	      amount -= 1;
	      helper. unlock ();
	   }
	}
	fprintf (stderr, "ofdm decoder is closing down now\n");
}
/**
  *	We need some functions to enter the ofdmProcessor data
  *	in the buffer.
  */
void	ofdmDecoder::processBlock_0 (DSPCOMPLEX *vi) {
#ifdef	__BETTER_LOCK
	bufferResources -> acquire (1);
#endif
	memcpy (command [0], vi, sizeof (DSPCOMPLEX) * T_u);
	helper. lock ();
	amount ++;
	commandHandler. wakeOne ();
	helper. unlock ();
}

void	ofdmDecoder::decodeFICblock (DSPCOMPLEX *vi, int32_t blkno) {
#ifdef	__BETTER_LOCK
	bufferResources -> acquire (1);
#endif
	memcpy (command [blkno], &vi [T_g], sizeof (DSPCOMPLEX) * T_u);
	helper. lock ();
	amount ++;
	commandHandler. wakeOne ();
	helper. unlock ();
}

void	ofdmDecoder::decodeMscblock (DSPCOMPLEX *vi, int32_t blkno) {
#ifdef	__BETTER_LOCK
	bufferResources -> acquire (1);
#endif
	memcpy (command [blkno], &vi [T_g], sizeof (DSPCOMPLEX) * T_u);
	helper. lock ();
	amount ++;
	commandHandler. wakeOne ();
	helper. unlock ();
}
/**
  *	Note that the distinction, made in the ofdmProcessor class
  *	does not add much here, iff we decide to choose the multi core
  *	option definitely, then code may be simplified there.
  */

/**
  *	handle block 0 as collected from the buffer
  */
void	ofdmDecoder::processBlock_0 (void) {
int16_t	i, index = 0;
DSPCOMPLEX	*w = (DSPCOMPLEX *)alloca (64 * sizeof (DSPCOMPLEX));
float	Min	= 1000;

	memcpy (fft_buffer, command [0], T_u * sizeof (DSPCOMPLEX));
	fft_handler	-> do_FFT ();
/**
  *	The SNR is determined by looking at a segment of bins
  *	within the signal region and bits outside.
  *	It is just an indication
  */
	snr		= 0.7 * snr + 0.3 * get_snr (fft_buffer);
	if (++snrCount > 10) {
	   show_snr (snr);
	   snrCount = 0;
	}
/**
  *	we are now in the frequency domain, and we keep the carriers
  *	as coming from the FFT as phase reference.
  */
	memcpy (phaseReference, fft_buffer, T_u * sizeof (DSPCOMPLEX));
}
/**
  *	for the other blocks of data, the first step is to go from
  *	time to frequency domain, to get the carriers.
  *	we distinguish between FIC blocks and other blocks,
  *	only to spare a test. Tthe mapping code is the same
  *
  *	\brief decodeFICblock
  *	do the transforms and hand over the result to the fichandler
  */
void	ofdmDecoder::decodeFICblock (int32_t blkno) {
int16_t	i;

	memcpy (fft_buffer, command [blkno], T_u * sizeof (DSPCOMPLEX));
fftlabel:
/**
  *	first step: do the FFT
  */
	fft_handler -> do_FFT ();
/**
  *	a little optimization: we do not interchange the
  *	positive/negative frequencies to their right positions.
  *	The de-interleaving understands this
  */
toBitsLabel:
/**
  *	Note that from here on, we are only interested in the
  *	"carriers" useful carriers of the FFT output
  */
	for (i = 0; i < carriers; i ++) {
	   int16_t	index	= myMapper -> mapIn (i);
	   if (index < 0) 
	      index += T_u;
/**
  *	decoding is computing the phase difference between
  *	carriers with the same index in subsequent blocks.
  *	The carrier of a block is the reference for the carrier
  *	on the same position in the next block
  */
	   DSPCOMPLEX	r1 = fft_buffer [index] * conj (phaseReference [index]);
	   phaseReference [index] = fft_buffer [index];
	   DSPFLOAT ab1	= jan_abs (r1);
///	split the real and the imaginary part and scale it
//	Note that the de-puncturing adds '0' as do not know.
//	If we decide to map the values onto the range 0 .. 255,
//	as requested by the viterbi decoder, we also should change
//	the added '0' values in the depuncturing
	   ibits [i]		= - real (r1) / ab1 * 127.0;
	   ibits [carriers + i] = - imag (r1) / ab1 * 127.0;
	}
handlerLabel:
	my_ficHandler -> process_ficBlock (ibits, blkno);
}
/**
  *	Msc block decoding is equal to FIC block decoding,
  */
void	ofdmDecoder::decodeMscblock (int32_t blkno) {
int16_t	i;

	memcpy (fft_buffer, command [blkno], T_u * sizeof (DSPCOMPLEX));
fftLabel:
	fft_handler -> do_FFT ();
//
//	Note that "mapIn" maps to -carriers / 2 .. carriers / 2
//	we did not set the fft output to low .. high
toBitsLabel:
	for (i = 0; i < carriers; i ++) {
	   int16_t	index	= myMapper -> mapIn (i);
	   if (index < 0) 
	      index += T_u;
	      
	   DSPCOMPLEX	r1 = fft_buffer [index] * conj (phaseReference [index]);
	   phaseReference [index] = fft_buffer [index];
	   DSPFLOAT ab1	= jan_abs (r1);
//	Recall:  the viterbi decoder wants 127 max pos, - 127 max neg
//	we make the bits into softbits in the range -127 .. 127
	   ibits [i]		= - real (r1) / ab1 * 127.0;
	   ibits [carriers + i] = - imag (r1) / ab1 * 127.0;
	}
handlerLabel:
	my_mscHandler -> process_mscBlock (ibits, blkno);
}

//
//
/**
  *	for the snr we have a full T_u wide vector, with in the middle
  *	K carriers.
  *	Just get the strength from the selected carriers compared
  *	to the strength of the carriers outside that region
  */
int16_t	ofdmDecoder::get_snr (DSPCOMPLEX *v) {
int16_t	i;
DSPFLOAT	noise 	= 0;
DSPFLOAT	signal	= 0;
int16_t	low	= T_u / 2 -  carriers / 2;
int16_t	high	= low + carriers;

	for (i = 10; i < low - 20; i ++)
	   noise += abs (v [(T_u / 2 + i) % T_u]);

	for (i = high + 20; i < T_u - 10; i ++)
	   noise += abs (v [(T_u / 2 + i) % T_u]);

	noise	/= (low - 30 + T_u - high - 30);
	for (i = T_u / 2 - carriers / 4;  i < T_u / 2 + carriers / 4; i ++)
	   signal += abs (v [(T_u / 2 + i) % T_u]);

	return get_db (signal / (carriers / 2)) - get_db (noise);
}

