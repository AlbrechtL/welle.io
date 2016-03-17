#
/*
 *    Copyright (C) 2011, 2012, 2013
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
 *
 */

#include	"audiosink.h"
#include	<stdio.h>
#include	<sys/stat.h>
#include	<QDebug>

#ifdef	WE_STREAM
#include	"gui.h"
#endif
/*
 *	The class is the sink for the data generated
 *	It resembles the older paWriter class, however, we have
 *	different data types (complex/float32 vs int16). We really
 *	should make a single generic class of it.
 */
#ifdef	WE_STREAM
	audioSink::audioSink	(int16_t latency,
	                         RadioInterface	*mr,
	                         RingBuffer<float>	*streamerBuffer) {
	connect (this, SIGNAL (samplesforStreamer (int)),
	         mr, SLOT (samplesforStreamer (int)));
	this	-> streamerBuffer	= streamerBuffer;
#else
	audioSink::audioSink	(int16_t latency) {
#endif
int32_t	i;

	this	-> CardRate	= 48000;
	this	-> latency	= latency;
	_O_Buffer		= new RingBuffer<float>(2 * 32768);
	portAudio		= false;
	writerRunning		= false;
	if (Pa_Initialize () != paNoError) {
	   fprintf (stderr, "Initializing Pa for output failed\n");
	   return;
	}

	portAudio	= true;
	qDebug ("Hostapis: %d\n", Pa_GetHostApiCount ());

	for (i = 0; i < Pa_GetHostApiCount (); i ++)
	   qDebug ("Api %d is %s\n", i, Pa_GetHostApiInfo (i) -> name);

	numofDevices	= Pa_GetDeviceCount ();
	ostream		= NULL;
	dumpFile	= NULL;
	f_16000		= new LowPassFIR (5, 16000, 48000);
	f_24000		= new LowPassFIR (5, 24000, 48000);
	f_32000		= new LowPassFIR (5, 32000, 96000);
}

	audioSink::~audioSink	(void) {
	if ((ostream != NULL) && !Pa_IsStreamStopped (ostream)) {
	   paCallbackReturn = paAbort;
	   (void) Pa_AbortStream (ostream);
	   while (!Pa_IsStreamStopped (ostream))
	      Pa_Sleep (1);
	   writerRunning = false;
	}

	if (ostream != NULL)
	   Pa_CloseStream (ostream);

	if (portAudio)
	   Pa_Terminate ();

	delete	_O_Buffer;
}

int32_t	audioSink::cardRate	(void) {
	return CardRate;
}
//
bool	audioSink::selectDevice (int16_t odev) {
PaError err;
	fprintf (stderr, "select device with %d\n", odev);
	if (!isValidDevice (odev))
	   return false;

	if ((ostream != NULL) && !Pa_IsStreamStopped (ostream)) {
	   paCallbackReturn = paAbort;
	   (void) Pa_AbortStream (ostream);
	   while (!Pa_IsStreamStopped (ostream))
	      Pa_Sleep (1);
	   writerRunning = false;
	}

	if (ostream != NULL)
	   Pa_CloseStream (ostream);

	outputParameters. device		= odev;
	outputParameters. channelCount		= 2;
	outputParameters. sampleFormat		= paFloat32;
	outputParameters. suggestedLatency	= 
	                          Pa_GetDeviceInfo (odev) ->
	                                      defaultHighOutputLatency * 4;
//	bufSize	= (int)((float)outputParameters. suggestedLatency);
	bufSize	= latency * 10 * 256;

//	if (bufSize < 0 || bufSize > 17300)
//	   bufSize = 16384;

	outputParameters. hostApiSpecificStreamInfo = NULL;
//
	fprintf (stderr, "Suggested size for outputbuffer = %d\n", bufSize);
	err = Pa_OpenStream (
	             &ostream,
	             NULL,
	             &outputParameters,
	             CardRate,
	             bufSize,
	             0,
	             this	-> paCallback_o,
	             this
	      );

	if (err != paNoError) {
	   qDebug ("Open ostream error\n");
	   return false;
	}
	fprintf (stderr, "stream opened\n");
	paCallbackReturn = paContinue;
	err = Pa_StartStream (ostream);
	if (err != paNoError) {
	   qDebug ("Open startstream error\n");
	   return false;
	}
	fprintf (stderr, "stream started\n");
	writerRunning	= true;
	return true;
}

void	audioSink::restart	(void) {
PaError err;

	if (!Pa_IsStreamStopped (ostream))
	   return;

	_O_Buffer	-> FlushRingBuffer ();
	paCallbackReturn = paContinue;
	err = Pa_StartStream (ostream);
	if (err == paNoError)
	   writerRunning	= true;
}

void	audioSink::stop	(void) {
	if (Pa_IsStreamStopped (ostream))
	   return;

	paCallbackReturn	= paAbort;
	(void)Pa_StopStream	(ostream);
	while (!Pa_IsStreamStopped (ostream))
	   Pa_Sleep (1);
	writerRunning		= false;
}
//
//	helper
bool	audioSink::OutputrateIsSupported (int16_t device, int32_t Rate) {
PaStreamParameters *outputParameters =
	           (PaStreamParameters *)alloca (sizeof (PaStreamParameters)); 

	outputParameters -> device		= device;
	outputParameters -> channelCount	= 2;	/* I and Q	*/
	outputParameters -> sampleFormat	= paFloat32;
	outputParameters -> suggestedLatency	= 0;
	outputParameters -> hostApiSpecificStreamInfo = NULL;

	return Pa_IsFormatSupported (NULL, outputParameters, Rate) ==
	                                          paFormatIsSupported;
}
/*
 * 	... and the callback
 */
int	audioSink::paCallback_o (
		const void*			inputBuffer,
                void*				outputBuffer,
		unsigned long			framesPerBuffer,
		const PaStreamCallbackTimeInfo	*timeInfo,
	        PaStreamCallbackFlags		statusFlags,
	        void				*userData) {
RingBuffer<float>	*outB;
float	*outp		= (float *)outputBuffer;
audioSink *ud		= reinterpret_cast <audioSink *>(userData);
uint32_t	actualSize;
uint32_t	i;
	(void)statusFlags;
	(void)inputBuffer;
	(void)timeInfo;
	if (ud -> paCallbackReturn == paContinue) {
	   outB = (reinterpret_cast <audioSink *> (userData)) -> _O_Buffer;
	   actualSize = outB -> getDataFromBuffer (outp, 2 * framesPerBuffer);
	   for (i = actualSize; i < 2 * framesPerBuffer; i ++)
	      outp [i] = 0;
	}

	return ud -> paCallbackReturn;
}
//
//	This one is a hack for handling different baudrates coming from
//	the aac decoder
void	audioSink::audioOut	(int16_t *V, int32_t amount, int32_t rate) {
	switch (rate) {
	   case 16000:	
	      audioOut_16000 (V, amount);
	      return;
	   case 24000:
	      audioOut_24000 (V, amount);
	      return;
	   case 32000:
	      audioOut_32000 (V, amount);
	      return;
	   default:
	   case 48000:
	      audioOut_48000 (V, amount);
	      return;
	}
}
//
//	scale up from 16 -> 48
void	audioSink::audioOut_16000	(int16_t *V, int32_t amount) {
float	*buffer = (float *)alloca (6 * amount * sizeof (float));
int32_t	i;
int32_t	available = _O_Buffer -> GetRingBufferWriteAvailable ();

	amount	=  3 * amount;
	if (2 * amount > available)
	   amount = (available / 2) & ~01;
	for (i = 0; i < amount / 3; i ++) {
	   DSPCOMPLEX help = DSPCOMPLEX (0, 0);
	   help = DSPCOMPLEX (float (V [2 * i]) / 32767.0,
	                      float (V [2 * i + 1]) / 32767.0);
	   help	= f_16000 -> Pass (help);
	   buffer [6 * i] = real (help);
	   buffer [6 * i + 1] = imag (help);
	   help = f_16000 -> Pass (DSPCOMPLEX (0, 0));
	   buffer [6 * i + 2] = real (help);
	   buffer [6 * i + 3] = imag (help);
	   help = f_16000 -> Pass (DSPCOMPLEX (0, 0));
	   buffer [6 * i + 4] = real (help);
	   buffer [6 * i + 5] = imag (help);
	}

	if (dumpFile != NULL)
	   sf_writef_float (dumpFile, buffer, amount);

#ifdef	WE_STREAM
	streamerBuffer	-> putDataIntoBuffer (buffer, 2 * amount);
	samplesforStreamer (2 * amount);
#else
	_O_Buffer	-> putDataIntoBuffer (buffer, 2 * amount);
#endif
}

void	audioSink::audioOut_24000	(int16_t *V, int32_t amount) {
float	*buffer = (float *)alloca (4 * amount * sizeof (float));
int32_t	i;
int32_t	available = _O_Buffer -> GetRingBufferWriteAvailable ();

	amount	= 2 * amount;
	if (2 * amount > available)
	   amount = (available / 2) & ~01;
	for (i = 0; i < amount / 2; i ++) {
	   DSPCOMPLEX help = DSPCOMPLEX (float (V [2 * i]) / 32767.0,
	                                 float (V [2 * i + 1]) / 32767.0);
	   help = f_24000 -> Pass (help);
	   buffer [4 * i] = real (help);
	   buffer [4 * i + 1] = imag (help);
	   help = f_24000 -> Pass (DSPCOMPLEX (0, 0));
	   buffer [4 * i + 2] = real (help);
	   buffer [4 * i + 3] = imag (help);
	}

	if (dumpFile != NULL)
	   sf_writef_float (dumpFile, buffer, 2 * amount);

#ifdef	WE_STREAM
	streamerBuffer	-> putDataIntoBuffer (buffer, 4 * amount);
	samplesforStreamer (4 * amount);
#else
	_O_Buffer	-> putDataIntoBuffer (buffer, 4 * amount);
#endif
}
//
//	Conversion from 32 -> 48 is by first converting to 96000
//	and then back to 48000
void	audioSink::audioOut_32000	(int16_t *V, int32_t amount) {
DSPCOMPLEX *buffer_1 = (DSPCOMPLEX *)alloca (3 * amount * sizeof (DSPCOMPLEX));
float	*buffer = (float *)alloca (2 * 3 * amount / 2 * sizeof (float));
int32_t	i;

	for (i = 0; i < amount; i ++) {
	   DSPCOMPLEX help = DSPCOMPLEX (float (V [2 * i]) / 32767.0,
	                                 float (V [2 * i + 1]) / 32767.0);
	   buffer_1 [3 * i + 0] = f_32000 -> Pass (help);
	   buffer_1 [3 * i + 1] = f_32000 -> Pass (DSPCOMPLEX (0, 0));
	   buffer_1 [3 * i + 2] = f_32000 -> Pass (DSPCOMPLEX (0, 0));
	}
//
//	Note that although we use "2 * i" for index left and right
//	they mean different things
	for (i = 0; i < 3 * amount / 2; i ++) {
	   buffer [2 * i]	= real (buffer_1 [2 * i]);
	   buffer [2 * i + 1]	= imag (buffer_1 [2 * i]);
	}
	   
	if (dumpFile != NULL)
	   sf_writef_float (dumpFile, buffer, 3 * amount / 2);

#ifdef	WE_STREAM
	streamerBuffer	-> putDataIntoBuffer (buffer, 3 * amount);
	samplesforStreamer (3 * amount);
#else
	_O_Buffer	-> putDataIntoBuffer (buffer, 3 * amount);
#endif
}

void	audioSink::audioOut_48000	(int16_t *V, int32_t amount) {
float	*buffer = (float *)alloca (2 * amount * sizeof (float));
int32_t	i, n;
int32_t	available = _O_Buffer -> GetRingBufferWriteAvailable ();

	n	=  amount;
	if (2 * n > available)
	   n = (available / 2) & ~01;

	for (i = 0; i < amount; i ++) {
	   buffer [2 * i] = float (V [2 * i]) / 32767.0;
	   buffer [2 * i + 1] = float (V [2 * i + 1]) / 32767.0;
	}

	if (dumpFile != NULL)
	   sf_writef_float (dumpFile, buffer, amount);
//
//	O_Buffer contains floats, so a double "amount" (which was the
//	count of IQ samples
#ifdef	WE_STREAM
	streamerBuffer	-> putDataIntoBuffer (buffer, 2 * amount);
	samplesforStreamer (2 * amount);
#else
	_O_Buffer	-> putDataIntoBuffer (buffer, 2 * amount);
#endif
}
//
//	putSample output comes from the FM receiver

int32_t	audioSink::putSample	(DSPCOMPLEX v) {
	return putSamples (&v, 1);
}

int32_t	audioSink::putSamples		(DSPCOMPLEX *V, int32_t n) {
float	*buffer = (float *)alloca (2 * n * sizeof (float));
int32_t	i;
int32_t	available = _O_Buffer -> GetRingBufferWriteAvailable ();

	if (2 * n > available)
	   n = (available / 2) & ~01;
	for (i = 0; i < n; i ++) {
	   buffer [2 * i] = real (V [i]);
	   buffer [2 * i + 1] = imag (V [i]);
	}

	if (dumpFile != NULL)
	   sf_writef_float (dumpFile, buffer, n);
	_O_Buffer	-> putDataIntoBuffer (buffer, 2 * n);
	return n;
}

int16_t	audioSink::numberofDevices	(void) {
	return numofDevices;
}

QString audioSink::outputChannelwithRate (int16_t ch, int32_t rate) {
const PaDeviceInfo *deviceInfo;
QString name = QString ("");

	if ((ch < 0) || (ch >= numofDevices))
	   return name;

	deviceInfo = Pa_GetDeviceInfo (ch);
	if (deviceInfo == NULL)
	   return name;
	if (deviceInfo -> maxOutputChannels <= 0)
	   return name;

	if (OutputrateIsSupported (ch, rate))
	   name = QString (deviceInfo -> name);
	return name;
}

int16_t	audioSink::invalidDevice	(void) {
	return numofDevices + 128;
}

bool	audioSink::isValidDevice (int16_t dev) {
	return 0 <= dev && dev < numofDevices;
}

bool	audioSink::selectDefaultDevice (void) {
	return selectDevice (Pa_GetDefaultOutputDevice ());
}

void	audioSink::startDumping	(SNDFILE *f) {
	dumpFile	= f;
}

void	audioSink::stopDumping	(void) {
	dumpFile	= NULL;
}

int32_t	audioSink::getSelectedRate	(void) {
	return CardRate;
}

