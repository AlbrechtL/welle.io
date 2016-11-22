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

#include	<QThread>
#include	<QSettings>
#include	<QHBoxLayout>
#include	<QLabel>

#include	"mirsdrapi-rsp.h"
#include	"sdrplay.h"	// our header
#include	"sdrplay-worker.h"	// the worker
#include	"sdrplay-loader.h"	// funtion loader
//
#define	DEFAULT_GAIN	25

	sdrplay::sdrplay  (QSettings *s, bool *success, bool show) {
int	err;
float	ver;

	sdrplaySettings		= s;
	this	-> myFrame	= new QFrame (NULL);
	setupUi (this -> myFrame);
	if (show)
	   this	-> myFrame	-> show ();
	this	-> inputRate	= Khz (2048);
	this	-> bandWidth	= Khz (1536);

	*success		= false;
	_I_Buffer	= NULL;
	theLoader	= NULL;
	theWorker	= NULL;

	theLoader	= new sdrplayLoader (success);
	if (!(*success)) {
	   fprintf (stderr, " No success in loading sdrplay lib\n");
	   delete theLoader;
	   theLoader = NULL;
	   return;
	}

	err			= theLoader -> my_mir_sdr_ApiVersion (&ver);
	if (ver != MIR_SDR_API_VERSION) {
	   fprintf (stderr, "Foute API: %f, %d\n", ver, err);
	   statusLabel	-> setText ("mirics error");
	}

	api_version	-> display (ver);
	_I_Buffer	= new RingBuffer<int16_t>(2 * 1024 * 1024);
	vfoFrequency	= Khz (94700);
	currentGain	= DEFAULT_GAIN;
	vfoOffset	= 0;

	sdrplaySettings		-> beginGroup ("sdrplaySettings");
	gainSlider 		-> setValue (
	            sdrplaySettings -> value ("externalGain", 10). toInt ());
	sdrplaySettings	-> endGroup ();

	setExternalGain	(gainSlider	-> value ());
	connect (gainSlider, SIGNAL (valueChanged (int)),
	         this, SLOT (setGain_slider (int)));
	connect (agcControl, SIGNAL (stateChanged (int)),
	         this, SLOT (set_autoGain (int)));
	*success	= true;
}

	sdrplay::~sdrplay	(void) {
	sdrplaySettings	-> beginGroup ("sdrplaySettings");
	sdrplaySettings	-> setValue ("externalGain", gainSlider -> value ());
	sdrplaySettings	-> endGroup ();
	stopReader ();
	if (_I_Buffer != NULL)
	   delete _I_Buffer;
	if (theLoader != NULL)
	   delete theLoader;
	if (theWorker != NULL)
	   delete theWorker;
	delete	myFrame;
}
//
//	The filter bank for the dongle is the first
//	one
static inline
int16_t	bankFor (int32_t freq) {
	if (freq < 60 * MHz (1))
	   return -1;
	if (freq < 120 * MHz (1))
	   return 1;
	if (freq < 245 * MHz (1))
	   return 2;
	if (freq < 420 * MHz (1))
	   return -1;
	if (freq < 1000 * MHz (1))
	   return 3;
	return -1;
}
//
//	But for the sdrplay we use the second one
static inline
int16_t	bankFor_sdr (int32_t freq) {
	if (freq < 12 * MHz (1))
	   return 1;
	if (freq < 30 * MHz (1))
	   return 2;
	if (freq < 60 * MHz (1))
	   return 3;
	if (freq < 120 * MHz (1))
	   return 4;
	if (freq < 250 * MHz (1))
	   return 5;
	if (freq < 420 * MHz (1))
	   return 6;
	if (freq < 1000 * MHz (1))
	   return 7;
	if (freq < 2000 * MHz (1))
	   return 8;
	return -1;
}

bool	sdrplay::legalFrequency (int32_t f) {
	return (bankFor_sdr (f) != -1);
}

int32_t	sdrplay::defaultFrequency	(void) {
	return Khz (94700);
}

void	sdrplay::setVFOFrequency	(int32_t newFrequency) {
int32_t	realFreq = newFrequency + vfoOffset;

	if (bankFor_sdr (realFreq) == -1)
	   return;

	if (theWorker == NULL) {
	   vfoFrequency = newFrequency + vfoOffset;
	   return;
	}

	if (bankFor_sdr (realFreq) != bankFor_sdr (vfoFrequency)) {
	   stopReader ();
	   vfoFrequency	= realFreq;
	   restartReader ();
	   return;
	}
	else
	   theWorker -> setVFOFrequency (realFreq);
	vfoFrequency = realFreq;
}

int32_t	sdrplay::getVFOFrequency	(void) {
	return vfoFrequency - vfoOffset;
}

void	sdrplay::setGain_slider	(int newGain) {
	if (newGain < 0 || newGain > 102)
	   return;

	if (theWorker != NULL)
	   theWorker -> setExternalGain (newGain);
	currentGain = newGain;
	return;
}

void	sdrplay::setGain	(int newgain) {
	setGain_slider (newgain * maxGain () / 100);
}

void	sdrplay::set_autoGain	(bool b) {
	this	-> agcMode	= agcControl -> isChecked ();
	if (theWorker == NULL)
	   return;
	theWorker	-> set_agcControl (this -> agcMode);
}

void	sdrplay::setAgc		(bool b) {
	this	-> agcMode	= b;
	if (theWorker == NULL)
	   return;
	theWorker	-> set_agcControl (this -> agcMode);
}

	

int16_t	sdrplay::maxGain	(void) {
	return 101;
}

bool	sdrplay::restartReader	(void) {
bool	success;

	if (theWorker != NULL)
	   return true;

	theWorker = new sdrplayWorker (inputRate,
	                               bandWidth,
	                               vfoFrequency,
	                               currentGain,
	                               agcMode,
	                               theLoader,
	                               _I_Buffer,
	                               &success);
	_I_Buffer	-> FlushRingBuffer ();
	return success;
}

void	sdrplay::stopReader	(void) {
	if (theWorker == NULL)
	   return;
	theWorker	-> stop ();
	while (theWorker -> isRunning ())
	   usleep (100);
	delete theWorker;
	theWorker = NULL;
}
//
//	The brave old getSamples. For the mirics stick, we get
//	size still in I/Q pairs
//	Note that the sdrPlay returns 10 bit values
int32_t	sdrplay::getSamples (DSPCOMPLEX *V, int32_t size) { 
int32_t	amount, i;
int16_t	*buf  = (int16_t *)alloca (2 * size * sizeof (int16_t));
//
	amount = _I_Buffer	-> getDataFromBuffer (buf, 2 * size);
	for (i = 0; i < amount / 2; i ++)  
	   V [i] = DSPCOMPLEX (buf [2 * i] / 2048.0,
	                       buf [2 * i + 1] / 2048.0);
	return amount / 2;
}

int32_t	sdrplay::Samples	(void) {
	return _I_Buffer	-> GetRingBufferReadAvailable () / 2;
}

uint8_t	sdrplay::myIdentity	(void) {
	return SDRPLAY;
}

void	sdrplay::resetBuffer	(void) {
	_I_Buffer	-> FlushRingBuffer ();
}

int16_t	sdrplay::bitDepth	(void) {
	return 14;
}

