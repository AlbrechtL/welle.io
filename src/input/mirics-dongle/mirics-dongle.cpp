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

#include	"mir_sdr.h"	// the header of the Mirics library
#include	"mirics-dongle.h"	// our header
#include	"mirics-worker.h"	// the worker
#include	"mirics-loader.h"	// funtion loader
//
#define	DEFAULT_GAIN	40

	miricsDongle::miricsDongle (QSettings *s, bool	*success) {
int	err;
float	ver;

	dongleSettings		= s;
	*success		= false;

	myFrame			= new QFrame;
	setupUi (myFrame);
	myFrame		-> show ();
	this	-> inputRate	= Khz (2048);
	this	-> bandWidth	= Khz (1536);

	_I_Buffer	= NULL;
	theLoader	= NULL;

	theLoader	= new miricsLoader (success);
	if (!(*success)) {
	   fprintf (stderr, " No success in loading mirics lib\n");
	   delete theLoader;
	   theLoader = NULL;
	   return;
	}

	err			= theLoader -> my_mir_sdr_ApiVersion (&ver);
//	if (ver != MIR_SDR_API_VERSION) {
//	   fprintf (stderr, "Foute API: %f, %d\n", ver, err);
//	   statusLabel	-> setText ("mirics error");
//	   return NULL;
//	}

	_I_Buffer	= new RingBuffer<int16_t>(2 * 1024 * 1024);
	vfoFrequency	= Khz (94700);
	currentGain	= DEFAULT_GAIN;
	vfoOffset	= 0;
	theWorker	= NULL;

	dongleSettings		-> beginGroup ("dongleSettings");
	externalGain 		-> setValue (
	            dongleSettings -> value ("externalGain", 10). toInt ());
	f_correction		-> setValue (
	            dongleSettings -> value ("f_correction", 0). toInt ());
	KhzOffset		-> setValue (
	            dongleSettings -> value ("KhzOffset", 0). toInt ());
	dongleSettings	-> endGroup ();
	setExternalGain (externalGain	-> value ());
	set_KhzOffset	(KhzOffset	-> value ());
	connect (externalGain, SIGNAL (valueChanged (int)),
	         this, SLOT (setExternalGain (int)));
	connect (KhzOffset, SIGNAL (valueChanged (int)),
	         this, SLOT (set_KhzOffset (int)));
	*success	= true;
}

	miricsDongle::~miricsDongle	(void) {
	dongleSettings	-> beginGroup ("dongleSettings");
	dongleSettings	-> setValue ("externalGain", externalGain -> value ());
	dongleSettings	-> setValue ("f_correction", f_correction -> value ());
	dongleSettings	-> setValue ("KhzOffset", KhzOffset -> value ());
	dongleSettings	-> endGroup ();
	stopReader ();
	if (_I_Buffer != NULL)
	   delete _I_Buffer;
	if (theLoader != NULL)
	   delete theLoader;
	if (theWorker != NULL)
	   delete theWorker;
	delete myFrame;
}

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

bool	miricsDongle::legalFrequency (int32_t f) {
	return (bankFor (f) != -1);
}

int32_t	miricsDongle::defaultFrequency	(void) {
	return Khz (94700);
}

void	miricsDongle::setVFOFrequency	(int32_t newFrequency) {
int32_t	realFreq = newFrequency + vfoOffset;

	if (bankFor (realFreq) == -1)
	   return;

	if (theWorker == NULL) {
	   vfoFrequency = newFrequency + vfoOffset;
	   return;
	}

	if (bankFor (realFreq) != bankFor (vfoFrequency)) {
	   stopReader ();
	   vfoFrequency	= realFreq;
	   restartReader ();
	   return;
	}
	else
	   theWorker -> setVFOFrequency (realFreq);
	vfoFrequency = realFreq;
}

int32_t	miricsDongle::getVFOFrequency	(void) {
	return vfoFrequency - vfoOffset;
}

void	miricsDongle::setExternalGain	(int newGain) {
	if (newGain < 0 || newGain > 102)
	   return;

	if (theWorker != NULL)
	   theWorker -> setExternalGain (newGain);
	currentGain = newGain;
}

void	miricsDongle::set_KhzOffset	(int o) {
	vfoOffset	= o;
}

int32_t	miricsDongle::setExternalRate	(int32_t newRate) {
	if (newRate < bandWidth)
	   return inputRate;

	if (theWorker != NULL)
	   theWorker -> setExternalRate (newRate);
	inputRate	= newRate;
	return inputRate;
}

bool	miricsDongle::restartReader	(void) {
bool	success;

	if (theWorker != NULL)
	   return true;

	theWorker = new miricsWorker (inputRate,
	                              bandWidth,
	                              vfoFrequency,
	                              theLoader,
	                              _I_Buffer,
	                              &success);
	_I_Buffer	-> FlushRingBuffer ();
	if (success)
	   theWorker -> setExternalGain (currentGain);
	return success;
}

void	miricsDongle::stopReader	(void) {
	if (theWorker == NULL)
	   return;
	theWorker	-> stop ();
	while (theWorker -> isRunning ())
	   usleep (100);
	delete theWorker;
	theWorker = NULL;
}
//
//	The brave old getSamples. For the mirics dongle, we get
//	size still in I/Q pairs.
//	We assume 14 bits data with this rate and these frequencies
int32_t	miricsDongle::getSamples (DSPCOMPLEX *V, int32_t size) { 
int32_t	amount, i;
int16_t	buf [2 * size];
//
	amount = _I_Buffer	-> getDataFromBuffer (buf, 2 * size);
	for (i = 0; i < amount / 2; i ++)  
	   V [i] = DSPCOMPLEX (buf [2 * i] / 4096.0,
	                                buf [2 * i + 1] / 4096.0);
	return amount / 2;
}

int32_t	miricsDongle::Samples	(void) {
	return _I_Buffer	-> GetRingBufferReadAvailable () / 2;
}

uint8_t	miricsDongle::myIdentity	(void) {
	return MIRICS_STICK;
}

void	miricsDongle::resetBuffer	(void) {
	_I_Buffer	-> FlushRingBuffer ();
}

int16_t	miricsDongle::maxGain	(void) {
	return 101;
}

