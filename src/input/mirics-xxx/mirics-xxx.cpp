#
/*
 *    Copyright (C) 2010, 2011, 2012, 2013
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

#include	"mirisdr.h"	// the included library
#include	"mirics-xxx.h"	// our header
//
//	This is the user-side call back function
static
void	MIRICSCallBack (uint8_t *buf, uint32_t len, void *ctx) {
mirics_xxx	*theStick = (mirics_xxx *)ctx;

	if (theStick == NULL)
	   return;
//	Note that our "context"  is the stick
	theStick-> _I_Buffer -> putDataIntoBuffer ((int16_t *)buf, len / 2);
}
//
//	for handling the events in libusb, we need a controlthread
//	whose sole purpose is to process the mirisdr_read_async function
//	from the lib.
class	mirics_driver : public QThread {
private:
	mirics_xxx	*theStick;
public:

	mirics_driver (mirics_xxx *d) {
	theStick	= d;
	start ();
}

	~mirics_driver (void) {
}

private:
virtual void	run (void) {
	mirisdr_read_async (theStick -> device,
	                   (mirisdr_read_async_cb_t)&MIRICSCallBack,
	                   (void *)theStick,
	                   32,
	                   theStick	-> bufferLength);
//	fprintf (stderr, "worker will terminate\n");
	}
};
//
//	Our wrapper is a simple classs

	mirics_xxx::mirics_xxx (QSettings *s, bool *success) {

	this	-> miriSettings	= s;
	*success		= false;	//default
	open			= false;
	myFrame			= new QFrame;
	setupUi	(myFrame);
	myFrame			-> show ();
	inputRate		= Khz (2048);
	bandWidth		= Khz (1536);	// set here by default

	*success = setupDevice (inputRate);
//	that is it, 
}

	mirics_xxx::~mirics_xxx		(void) {
//	first save settings
	if (open && miriSettings != NULL) {
	   miriSettings -> beginGroup ("xxxSettings");
	   miriSettings	-> setValue ("miri_externalGain",
	                                             externalGain -> value ());
	   miriSettings	-> setValue ("miri_khzOffset", khzOffset -> value ());
	   miriSettings -> endGroup ();
	}
	fprintf (stderr, "fase 1\n");
	if (open) {
	   stopReader ();
	   mirisdr_close (device);
	}
	fprintf (stderr, "fase 2\n");

	if (_I_Buffer != NULL)
	   delete _I_Buffer;
	fprintf (stderr, "fase 3\n");
	if (gains != NULL)
	   delete[] gains;
	fprintf (stderr, "fase 4\n");
	delete myFrame;
}

bool	mirics_xxx::setupDevice (int32_t rateIn) {
int16_t	deviceCount;
int	r, i, k;
int16_t	deviceIndex;
//
	this		-> bufferLength	= 12 * 768;
	open		= false;
	_I_Buffer	= NULL;
	gains		= NULL;
	deviceCount	= mirisdr_get_device_count ();
	if (deviceCount == 0) {
	   fprintf (stderr, "No devices found\n");
	   statusLabel	-> setText ("No devices");
	   return false;
	}
	deviceIndex 			= 0;	// default
//
//	vfoFrequency is the VFO frequency
	vfoFrequency		= KHz (94700);	// just a dummy
	vfoOffset		= 0;

	deviceCount 		= (int16_t)(mirisdr_get_device_count ());
	if (deviceCount == 0) {
	   fprintf (stderr, "No valid devices\n");
	   goto err;
	}

	deviceIndex = 0;	// default
//	OK, now open the hardware
	r			= mirisdr_open (&device, deviceIndex);
	if (r < 0) {
	   fprintf (stderr, "Opening mirics failed\n");
	   statusLabel	-> setText ("opening failed");
	   goto err;
	}

	r	= mirisdr_set_sample_rate (device, inputRate);
	if (r < 0) {
	   fprintf (stderr, "Setting samplerate failed\n");
	   goto err;
	}

	r		= mirisdr_set_sample_format (device, "252_S16");
	if (r < 0) {
	   fprintf (stderr, "Setting sample format failed\n");
	   goto err;
	}
	fprintf (stderr, "xxx-1\n");
	open	= true;
	gainsCount 	= mirisdr_get_tuner_gains (device, NULL);
	gainsCount	= 101;
	fprintf(stderr, "Supported gain values (%d): ", gainsCount);
	gains		= new int [gainsCount];
	gainsCount = mirisdr_get_tuner_gains (device, gains);
	for (i = 0; i < gainsCount; i++)
	   fprintf(stderr, "%.1f ", gains [i] / 10.0);
//
	fprintf (stderr, "setting gain mode\n");
	mirisdr_set_tuner_gain_mode (device, 1);
	fprintf (stderr, "set tuner gain  to %d (%d)\n", gainsCount / 2,
	                                      gains [gainsCount / 2]);
	mirisdr_set_tuner_gain (device, gains [gainsCount / 2]);
	fprintf (stderr, "gain is set  to %.1f\n", gains [gainsCount / 2] / 10.0);
	_I_Buffer	= new RingBuffer<int16_t>(1024 * 1024);
	workerHandle	= NULL;

	connect (externalGain, SIGNAL (valueChanged (int)),
	         this, SLOT (setExternalGain (int)));
	connect (khzOffset, SIGNAL (valueChanged (int)),
	         this, SLOT (setKhzOffset (int)));
//
//	reset some values from previous incarnations
	if (miriSettings != NULL) {
	   miriSettings -> beginGroup ("xxxSettings");
	   k	= miriSettings	-> value ("miri_externalGain", 26). toInt ();
	   externalGain	-> setValue (k);
	   k	= miriSettings	-> value ("miri_khzOffset", 0). toInt ();
	   khzOffset	-> setValue (k);
	   miriSettings -> endGroup ();
	}

	return true;
err:
	if (open)
	   mirisdr_close (device);
	open		= false;
	return false;
}
//
int32_t	mirics_xxx::getVFOFrequency	(void) {
	if (!open)
	   return defaultFrequency ();
	return (int32_t)mirisdr_get_center_freq (device) - vfoOffset;
}
//
//	The external world sets a virtual VFO frequency
//	The real VFO frequency can be influences by an externally
//	set vfoOffset
void	mirics_xxx::setVFOFrequency	(int32_t f) {
	if (!open)
	   return;

	vfoFrequency	= f;
	(void)mirisdr_set_center_freq (device, f + vfoOffset);
}

int32_t	mirics_xxx::defaultFrequency	(void) {
	return KHz (94700);
}
//
bool	mirics_xxx::restartReader	(void) {
int32_t	r;

	if (!open)
	   return false;

	if (workerHandle != NULL)	// running already
	   return true;

	r = mirisdr_reset_buffer (device);
	if (r < 0)
	   return false;

	mirisdr_set_center_freq (device, vfoFrequency + vfoOffset);
	workerHandle	= new mirics_driver (this);
	return true;
}

void	mirics_xxx::stopReader	(void) {
	if (workerHandle == NULL)
	   return;

	mirisdr_cancel_async (device);
	if (workerHandle != NULL) {
	   while (!workerHandle -> isFinished ()) 
	      usleep (10);

	   delete	workerHandle;
	}
	workerHandle	= NULL;
}
//

void	mirics_xxx::setExternalGain	(int gain) {
static int	oldGain	= 0;

	gain	= gainsCount - gain;
	if (gain == oldGain)
	   return;
	if ((gain < 0) || (gain > gainsCount))
	   return;	

	oldGain	= gain;
	mirisdr_set_tuner_gain (device, gains [gain]);
	(void)mirisdr_get_tuner_gain (device);
}
//
//	Good old getSamples
//
int32_t	mirics_xxx::getSamples (DSPCOMPLEX *V, int32_t size) {
int16_t	*buf	= (int16_t *)alloca (2 * size * sizeof (int16_t));
int32_t	i;

	if (!open)
	   return	0;
//	first step: get data from the buffer
	(void) _I_Buffer	-> getDataFromBuffer (buf, 2 * size);
//
//	and scale
	for (i = 0; i < size; i ++)  
	   V [i] = DSPCOMPLEX (buf [2 * i] / 32768.0,
	                       buf [2 * i + 1] / 32768.0);

	return size;
}

int32_t	mirics_xxx::Samples	(void) {
	return _I_Buffer -> GetRingBufferReadAvailable () / 2;
}
//
//	vfoOffset is in Hz, we have two spinboxes influencing the
//	settings
void	mirics_xxx::setKhzOffset	(int k) {
	vfoOffset	= (vfoOffset / 1000) + Khz (k);
	(void)mirisdr_set_center_freq (device, vfoFrequency + vfoOffset);
}

uint8_t	mirics_xxx::myIdentity	(void) {
	return MIRICS_STICK;
}

void	mirics_xxx::resetBuffer	(void) {
	_I_Buffer	-> FlushRingBuffer ();
}

int16_t	mirics_xxx::maxGain		(void) {
	return gainsCount;
}

