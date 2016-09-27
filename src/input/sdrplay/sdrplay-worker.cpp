#
/*
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
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

#include	"dab-constants.h"	// some general definitions
#include	"sdrplay-worker.h"		// our header
#include	"ringbuffer.h"		// the buffer

//	The sdrplay worker is the actual wrapper around the mir-sdr
//	interface. It is a pretty simple thread performing the
//	basic functions.
//	
//	It is a pretty local class

#define	NO_CHANGE	0000
#define	FREQ_CHANGE	0001
#define	GAIN_CHANGE	0002
#define	RATE_CHANGE	0004
#define	AGC_CHANGE	0010

	sdrplayWorker::sdrplayWorker (int32_t	deviceRate,
	                              int32_t	bandWidth,
	                              int32_t	defaultFreq,
	                              int32_t	currentGain,
	                              int32_t	agcMode,
	                              sdrplayLoader	*f,
	                              RingBuffer<int16_t> *buf,
	                              bool	*OK) {
int	err;

	this	-> deviceRate	= float(deviceRate) / MHz (1);
	this	-> bandWidth	= bandWidth / MHz (1);
	this	-> defaultFreq	= float(defaultFreq) / MHz (1);
	this	-> currentGain	= currentGain;
	this	-> agcMode	= agcMode;
	this	-> functions	= f;
	_I_Buffer		= buf;
	*OK			= false;	// just the default

	mir_sdr_Bw_MHzT aa = 
	        bandWidth <= 200 * KHz (1) ? mir_sdr_BW_0_200 :
	        bandWidth <= 300 * KHz (1) ? mir_sdr_BW_0_300 :
	        bandWidth <= 600 * KHz (1) ? mir_sdr_BW_0_600 :
	        bandWidth <= 1536 * KHz (1) ? mir_sdr_BW_1_536 :
	        bandWidth <= 5000 * KHz (1) ? mir_sdr_BW_5_000 :
	        bandWidth <= 6000 * KHz (1) ? mir_sdr_BW_6_000 :
	        bandWidth <= 7000 * KHz (1) ? mir_sdr_BW_7_000 :
	        bandWidth <= 8000 * KHz (1) ? mir_sdr_BW_8_000 :
	              mir_sdr_BW_8_000;
	        
//	Note: the "API check" has been done by the owner of this thread
	err			= functions -> my_mir_sdr_Init (currentGain,
	                                            this -> deviceRate,
	                                            this -> defaultFreq,
	                                            aa,
	                                            mir_sdr_IF_Zero,
	                                            &sps);
	if (err != 0) {
	   fprintf (stderr, "Probleem init\n");
	   return;
	}

	err			= functions -> my_mir_sdr_AgcControl (agcMode,
	                                                 30, 0, 0, 0, 1, 0);
	err			= functions -> my_mir_sdr_SetDcMode (4, 1);
	err			= functions -> my_mir_sdr_SetDcTrackTime (63);
//
//	some defaults:
	lastFrequency		= defaultFreq;	// the parameter!!!!
	runnable		= true;
	anyChange		= 0;
	start ();
	*OK			= true;
}

//	As usual, killing objects containing a thread need to
//	be done carefully.
void	sdrplayWorker::stop	(void) {
	runnable	= false;
}

	sdrplayWorker::~sdrplayWorker	(void) {
	runnable	= false;
	while (isRunning ())
	   msleep (1);
	functions -> my_mir_sdr_Uninit ();
}
//
//	The actual thread does not do much more than reading the
//	values made available through the Mirics API implementation
//	and passing the values on
//
void	sdrplayWorker:: run (void) {
int16_t		*localBuf 	=
	                      (int16_t *)alloca (2 * sps * sizeof (int16_t));
int16_t		*xi		= (int16_t *)alloca (sps * sizeof (int16_t));
int16_t		*xq		= (int16_t *)alloca (sps * sizeof (int16_t));
uint32_t	fs;
int16_t		i;
int32_t		grc, rfc, fsc;
int	err;

	functions -> my_mir_sdr_SetSyncUpdatePeriod ((int)(deviceRate * MHz (1) / 2));
	functions -> my_mir_sdr_SetSyncUpdateSampleNum (sps);
	while (runnable) {
	   err =  functions ->
	       my_mir_sdr_ReadPacket (&xi [0], & xq [0], &fs, &grc, &rfc, &fsc);

//	currently, we are not interested in the results other than the actual
//	data
	   for (i = 0; i < sps; i ++) {
	      localBuf [2 * i] 		= xi [i];
	      localBuf [2 * i + 1] 	= xq [i];
	   }
	   _I_Buffer	-> putDataIntoBuffer (localBuf, 2 * sps);

	   if (fsc != 0 || rfc != 0 ||grc != 0)
	      fprintf (stderr, "fsc = %d, rfc = %d, grc = %d\n",
	                                        fsc, rfc, grc);
//	OK, data is now stored, now checking for updates
	   while (anyChange != NO_CHANGE) {
	      if (anyChange & FREQ_CHANGE) {
	         functions -> my_mir_sdr_SetRf (lastFrequency, 1, 0);
	         anyChange &= ~FREQ_CHANGE;
	      }
	      if (anyChange & GAIN_CHANGE) {
	         functions -> my_mir_sdr_SetGr (currentGain, 1, 0);
	         anyChange &= ~GAIN_CHANGE;
	      }
	      if (anyChange & AGC_CHANGE) {
	         functions -> my_mir_sdr_AgcControl (agcMode, 30, 0, 0, 0, 1, 0);
	         if (agcMode == 0)
	            functions -> my_mir_sdr_SetGr (currentGain, 1, 0);
	         anyChange &= ~AGC_CHANGE;
	      }
	      if (anyChange & RATE_CHANGE) {
	        int r =  functions -> my_mir_sdr_SetFs (deltaRate, 1, 1, 0);
	        if (r == 0)
	           deviceRate = deltaRate / MHz (1);
	         anyChange &= ~RATE_CHANGE;
	      }
	   }
	}
//	fprintf (stderr, "sdrplay worker now stopped\n");
}

void	sdrplayWorker::setVFOFrequency	(int32_t f) {
	lastFrequency	= f;
	anyChange 	|= FREQ_CHANGE;
}

void	sdrplayWorker::setExternalGain	(int16_t gain) {
	if (gain < 0 || gain > 102)	// should not happen
	   return;
	currentGain	= gain;
	anyChange 	|= GAIN_CHANGE;
}
//
//	For now, setExternalRate is ONLY called from within
//	the DAB software to slightly adjust the rate
void	sdrplayWorker::setExternalRate	(int32_t newRate) {
	deltaRate	= double (newRate);
	anyChange	|= RATE_CHANGE;
}

void	sdrplayWorker::set_agcControl (int32_t agcMode) {
	this	-> agcMode	= agcMode;
	anyChange		|= AGC_CHANGE;
}

