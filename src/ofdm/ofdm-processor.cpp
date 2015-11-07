#
/*
 *    Copyright (C) 2013, 2014
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
#include	"ofdm-processor.h"
#include	"ofdm-decoder.h"
#include	"gui.h"
//
#define	syncBufferMask	(32768 - 1)
//
	ofdmProcessor::ofdmProcessor	(virtualInput	*theRig,
	                                 DabParams	*p,
	                                 RadioInterface *mr,
	                                 mscHandler 	*msc,
	                                 ficHandler 	*fic,
	                                 int16_t	threshold){
int32_t	i;
	this	-> theRig		= theRig;
	this	-> params		= p;
	this	-> T_null		= p	-> T_null;
	this	-> T_s			= p 	-> T_s;
	this	-> T_u			= p	-> T_u;
	this	-> myRadioInterface	= mr;
	this	-> my_mscHandler	= msc;
	this	-> my_ficHandler	= fic;
	dumping				= false;
//
	ofdmBuffer			= new DSPCOMPLEX [76 * T_s];
	ofdmBufferIndex			= 0;
	ofdmSymbolCount			= 0;
	tokenCount			= 0;
	phaseSynchronizer	= new phaseReference (params, threshold);
	my_ofdmDecoder		= new ofdmDecoder (params,
	                                           myRadioInterface,
	                                           my_ficHandler,
	                                           my_mscHandler);
	fineCorrector		= 0;	
	coarseCorrector		= 0;
	f2Correction		= false;
	oscillatorTable		= new DSPCOMPLEX [INPUT_RATE];
	localPhase		= 0;

	for (i = 0; i < INPUT_RATE; i ++)
	   oscillatorTable [i] = DSPCOMPLEX (cos (2.0 * M_PI * i / INPUT_RATE),
	                                     sin (2.0 * M_PI * i / INPUT_RATE));

	connect (this, SIGNAL (show_fineCorrector (int)),
	         myRadioInterface, SLOT (set_fineCorrectorDisplay (int)));
	connect (this, SIGNAL (show_coarseCorrector (int)),
	         myRadioInterface, SLOT (set_coarseCorrectorDisplay (int)));
	connect (this, SIGNAL (setSynced (char)),
	         myRadioInterface, SLOT (setSynced (char)));

	gain		= 30;
	bufferContent	= 0;
//
	start ();
}

	ofdmProcessor::~ofdmProcessor	(void) {
	running		= false;	// this will cause an
	                                // exception to be raised
	                        	// through the getNextSampleReady
	msleep (100);
	terminate ();
	wait ();
	delete	my_ofdmDecoder;
	delete	ofdmBuffer;
	delete	phaseSynchronizer;
	delete	oscillatorTable;
	disconnect (this, SIGNAL (show_fineCorrector (int)),
	            myRadioInterface, SLOT (set_fineCorrectorDisplay (int)));
	disconnect (this, SIGNAL (show_coarseCorrector (int)),
	            myRadioInterface, SLOT (set_coarseCorrectorDisplay (int)));
}

DSPCOMPLEX ofdmProcessor::getSample (int32_t phase) {
DSPCOMPLEX temp;
	if (!running)
	   throw 21;
	if (bufferContent == 0) {
	   bufferContent = theRig -> Samples ();
	   while ((bufferContent == 0) && running) {
	      usleep (10);
	      bufferContent = theRig -> Samples (); 
	   }
	}
	if (!running)	
	   throw 20;
//
//	so here, bufferContent > 0
	theRig -> getSamples (&temp, 1);
	bufferContent --;
	if (dumping) {
	   float f [2];
	   f [0] = real (temp);
	   f [1] = imag (temp);
	   sf_writef_float (dumpFile, f, 1);
	}
//
//	OK, we have a sample!!
//	first: adjust frequency. We need Hz accuracy
	localPhase	-= phase;
	localPhase	= (localPhase + INPUT_RATE) % INPUT_RATE;
	temp		*= oscillatorTable [localPhase];
	sLevel		= 0.00001 * jan_abs (temp) + (1 - 0.00001) * sLevel;
#define	N	5
	sampleCnt	++;
	if (++ sampleCnt > INPUT_RATE / N) {
	   show_fineCorrector	(fineCorrector);
	   show_coarseCorrector	(coarseCorrector / KHz (1));
	   sampleCnt = 0;
	}
	return temp;
}
//

void	ofdmProcessor::getSamples (DSPCOMPLEX *v, int16_t n, int32_t phase) {
int32_t		i;

	if (!running)
	   throw 21;
	if (n > bufferContent) {
	   bufferContent = theRig -> Samples ();
	   while ((bufferContent < n) && running) {
	      usleep (10);
	      bufferContent = theRig -> Samples (); 
	   }
	}
	if (!running)	
	   throw 20;
//
//	so here, bufferContent >= n
	n	= theRig -> getSamples (v, n);
	bufferContent -= n;
	if (dumping) {
	   float f [2];
	   for (i = 0; i < n; i ++) {
	      f [0] = real (v [i]);
	      f [1] = imag (v [i]);
	      sf_writef_float (dumpFile, f, 1);
	   }
	}
//
//	OK, we have samples!!
//	first: adjust frequency. We need Hz accuracy
	for (i = 0; i < n; i ++) {
	   localPhase	-= phase;
	   localPhase	= (localPhase + INPUT_RATE) % INPUT_RATE;
	   v [i]	*= oscillatorTable [localPhase];
	   sLevel	= 0.00001 * jan_abs (v [i]) + (1 - 0.00001) * sLevel;
	}

	sampleCnt	+= n;
	if (sampleCnt > INPUT_RATE / N) {
	   show_fineCorrector	(fineCorrector);
	   show_coarseCorrector	(coarseCorrector / KHz (1));
	   sampleCnt = 0;
	}
}
//	The main thread, reading sample and trying to identify
//	the ofdm frames
void	ofdmProcessor::run	(void) {
int32_t		startIndex;
int32_t		i, j;
DSPCOMPLEX	FreqCorr;
int32_t		counter;
float		currentStrength;
int32_t		syncBufferIndex;
int32_t		syncBufferSize;
float		envBuffer	[syncBufferMask + 1];
float		signalLevel;

	running		= true;
	fineCorrector	= 0;
	sLevel		= 0;
	try {
//	first, we initialize to get the buffers filled
initing:
	   syncBufferIndex	= 0;
	   signalLevel		= 0;
	   currentStrength	= 0;
//
//	first, we build up a reasonable initial value for the signalLevel
	   sLevel	= 0;
	   for (i = 0; i < 10 * T_s; i ++) {
	      sLevel		+=jan_abs (getSample (0));
	   }

notSynced:
//	read in T_s samples for a next attempt;
	   for (i = 0; i < T_s; i ++) {
	      DSPCOMPLEX sample			= getSample (0);
	      envBuffer [syncBufferIndex]	= jan_abs (sample);
	      syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
	   }

	   currentStrength	= 0;
	   for (i = syncBufferIndex - 50; i < syncBufferIndex; i ++)
	      currentStrength += envBuffer [i];


//	we now have initial values for currentStrength and signalLevel
//	Then we look for a "dip" in the datastream
SyncOnNull:
	   counter	= 0;
	   setSynced (false);
	   while (currentStrength / 50  > 0.35 * sLevel) {
	      DSPCOMPLEX sample	=
	                      getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex] = jan_abs (sample);
//	update the levels
	      currentStrength += envBuffer [syncBufferIndex] -
	                      envBuffer [(syncBufferIndex + syncBufferSize - 50) & syncBufferMask];
	      syncBufferIndex =
	                      (syncBufferIndex + 1) & syncBufferMask;
	      counter ++;
	      if (counter > 2 * T_s)	// hopeless
	         goto notSynced;
	   }

//	Now it is waiting for the end of the dip
SyncOnEndNull:
	   while (currentStrength / 50 < 0.85 * sLevel) {
	      DSPCOMPLEX sample = getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex] = abs (sample);
//	update the levels
	      currentStrength += envBuffer [syncBufferIndex] -
	                        envBuffer [(syncBufferIndex + syncBufferSize - 50) & syncBufferMask];
	      syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
	      counter	++;
	      if (counter > 4 * T_s)	// hopeless
	         goto notSynced;
	   }


SyncOnPhase:
//
//	here we also enter after having processed a full frame
//	now read in Tu samples
	   for (i = 0; i <  params -> T_u; i ++) {
	      ofdmBuffer [i] = getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex] = jan_abs (ofdmBuffer [i]);
//	update the levels
	      currentStrength += envBuffer [syncBufferIndex] -
	                              envBuffer [(syncBufferIndex  + syncBufferSize - 50) & syncBufferMask];
	      syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
	      counter ++;
	   }
//
//	and then, call upon the phase synchronizer to verify/compute
//	the real "first" sample of the stream
	   startIndex = phaseSynchronizer ->
	                        findIndex (ofdmBuffer, params -> T_u);
	   if (startIndex < 0) { // no sync, try again
//	      fprintf (stderr, "startIndex = %d\n", startIndex);
	      goto notSynced;
	   }

//	Once here, we are synchronized, we need to copy the data we
//	used for synchronization for block 0
	   memmove (ofdmBuffer, &ofdmBuffer [startIndex],
	                  (params -> T_u - startIndex) * sizeof (DSPCOMPLEX));
	   ofdmBufferIndex	= params -> T_u - startIndex;

OFDM_PRS:
//	read the missing samples in the ofdm buffer
	   setSynced (true);
	   getSamples (&ofdmBuffer [ofdmBufferIndex],
	               T_u - ofdmBufferIndex,
	               coarseCorrector + fineCorrector);
//
//	block0 will set the phase reference for further decoding
	   my_ofdmDecoder  -> processBlock_0 (ofdmBuffer);
//
//	after block 0, we will just read in the other (params -> L - 1) blocks
OFDM_SYMBOLS:
//	The first ones are the FIC blocks

	   FreqCorr		= DSPCOMPLEX (0, 0);
	   for (ofdmSymbolCount = 2;
	        ofdmSymbolCount <= 4; ofdmSymbolCount ++) {
	      getSamples (ofdmBuffer, T_s, coarseCorrector + fineCorrector);
	      for (i = (int)T_u; i < (int)T_s; i ++) 
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
	
	      my_ofdmDecoder -> decodeFICblock (ofdmBuffer, ofdmSymbolCount);
	   }

//	and similar for the (params -> L - 4) MSC blocks
	   for (ofdmSymbolCount = 5;
	        ofdmSymbolCount <= (int16_t)(params -> L);
	        ofdmSymbolCount ++) {
	      getSamples (ofdmBuffer, T_s, coarseCorrector + fineCorrector);
	      for (i = (int32_t)T_u; i < (int32_t)T_s; i ++) 
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
//	OK, at the end of a block
	      my_ofdmDecoder -> decodeMscblock (ofdmBuffer, ofdmSymbolCount);
	   }

//	We integrate the offset measured
	   fineCorrector += 0.1 * arg (FreqCorr) / M_PI *
	                        (params -> carrierDiff / 2);
//
//	OK, at the end of the frame
//	assume everything went well and skip T_null - 50 samples
	   syncBufferIndex	= 0;
	   currentStrength	= 0;
	   for (i = 0; i < T_null - 50; i ++) {
	      DSPCOMPLEX sample	= getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex ++] = jan_abs (sample);
	   }
//
//	and  we read 50 samples for a new  currentStrength
	   for (i = T_null - 50; i < T_null; i ++) {
	      DSPCOMPLEX sample	= getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex] = jan_abs (sample);
	      currentStrength 	+= envBuffer [syncBufferIndex ++];
	   }
//
//	so, we processed Tnull samples, so the real start should be T_g
//	samples away
	   counter	= 0;
//
static int waar = 0;
           if ((waar++ >= 3) && f2Correction) {
              float correction  = my_ofdmDecoder -> coarseCorrector ();
              coarseCorrector   += correction * params -> carrierDiff;
              waar = 0;
           }

	   if (fineCorrector > params -> carrierDiff / 2) {
	      fineCorrector -= params -> carrierDiff;
	      coarseCorrector += params -> carrierDiff;
	   }
	   else
	   if (fineCorrector < - params -> carrierDiff / 2) {
	      fineCorrector += params -> carrierDiff;
	      coarseCorrector -= params -> carrierDiff;
	   }
	
//	and off we go, up to the next frame
	   goto SyncOnPhase;
	}
	catch (int e) {
	   fprintf (stderr, "ofdmProcessor will halt\n");
	}
}

void	ofdmProcessor:: reset	(void) {
	fineCorrector = coarseCorrector = 0;
	f2Correction	= true;
}

void	ofdmProcessor::stop	(void) {
	running	= false;
}

void	ofdmProcessor::startDumping	(SNDFILE *f) {
	if (dumping)
	   return;
//	do not change the order here.
	dumpFile 	= f;
	dumping		= true;
}

void	ofdmProcessor::stopDumping	(void) {
	dumping = false;
}
//

void	ofdmProcessor::coarseCorrectorOn (void) {
	f2Correction 	= true;
	coarseCorrector	= 0;
}

void	ofdmProcessor::coarseCorrectorOff (void) {
	f2Correction	= false;
}

