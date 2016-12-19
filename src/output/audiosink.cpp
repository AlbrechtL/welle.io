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
#include	<QDebug>
#include	<QMessageBox>
/*
 */
	audioSink::audioSink	(int16_t latency,
#ifdef	GUI_3
	                         QStringList *s,
#else
	                         QComboBox *s,
#endif
	                         RingBuffer<int16_t> *b): audioBase (b) {
int32_t	i;
	this	-> latency	= latency;
#ifdef	GUI_3
	this	-> InterfaceList	= s;
#else
	this	-> streamSelector	= s;
#endif

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
	outTable	= new int16_t [numofDevices + 1];
	for (i = 0; i < numofDevices; i ++)
	   outTable [i] = -1;
	ostream		= NULL;
#ifdef	GUI_3
	setupChannels (InterfaceList);
#else
	setupChannels (streamSelector);
	connect (streamSelector, SIGNAL (activated (int)),
	         this,  SLOT (set_streamSelector (int)));
	streamSelector	-> show ();
#endif
	selectDefaultDevice ();
	
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
	delete[] outTable;
#ifndef	GUI_3
	if (streamSelector != NULL)
	   streamSelector	-> hide ();
#endif
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

void	audioSink::audioOutput	(float *b, int32_t amount) {
	_O_Buffer	-> putDataIntoBuffer (b, 2 * amount);
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

int32_t	audioSink::cardRate	(void) {
	return 48000;
}

#ifndef	GUI_3
bool	audioSink::setupChannels (QComboBox *streamOutSelector) {
uint16_t	ocnt	= 1;
uint16_t	i;

	for (i = 0; i <  numofDevices; i ++) {
	   const QString so = 
	             outputChannelwithRate (i, CardRate);
	   qDebug ("Investigating Device %d\n", i);

	   if (so != QString ("")) {
	      streamOutSelector -> insertItem (ocnt, so, QVariant (i));
	      outTable [ocnt] = i;
	      qDebug (" (output):item %d wordt stream %d (%s)\n", ocnt , i,
	                      so. toLatin1 ().data ());
	      ocnt ++;
	   }
	}

	qDebug () << "added items to combobox";
	return ocnt > 1;
}
#else
bool	audioSink::setupChannels (QStringList *streamOutSelector) {
uint16_t	ocnt	= 1;
uint16_t	i;

	for (i = 0; i <  numofDevices; i ++) {
	   const QString so = 
	             outputChannelwithRate (i, CardRate);
	   qDebug ("Investigating Device %d\n", i);

	   if (so != QString ("")) {
	      streamOutSelector -> append (so);
	      outTable [ocnt] = i;
	      qDebug (" (output):item %d wordt stream %d (%s)\n", ocnt , i,
	                      so. toLatin1 ().data ());
	      ocnt ++;
	   }
	}

	qDebug () << "added items to combobox";
	return ocnt > 1;
}
#endif

bool	audioSink::set_streamSelector (int idx) {
int16_t	outputDevice;

	if (idx == 0)
	   return false;

	outputDevice = outTable [idx];
	if (!isValidDevice (outputDevice)) {
	   return false;
	}

	stop	();
	if (!selectDevice (outputDevice)) {
	   fprintf (stderr, "error selecting device\n");
	   selectDefaultDevice ();
	   return false;
	}

	qWarning () << "selected output device " << idx << outputDevice;
	return true;
}
//
int16_t	audioSink::numberofDevices	(void) {
	return numofDevices;
}

