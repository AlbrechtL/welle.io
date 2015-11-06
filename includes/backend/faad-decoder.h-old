#
/*
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
 *	This file will be included in mp4processor
*/
#
#include	"neaacdec.h"
#include	"audiosink.h"
#include	"newconverter.h"

class	faadDecoder {
private:
	bool			processorOK;
	bool			aacInitialized;
	uint32_t		aacCap;
	NeAACDecHandle		aacHandle;
	NeAACDecConfigurationPtr	aacConf;
	NeAACDecFrameInfo	hInfo;
	audioSink		*ourSink;
	int32_t			baudRate;
	newConverter		*myConverter;
	DSPCOMPLEX		*tempBuffer;
	bool			corrector;
//
public:
	faadDecoder	(audioSink *as, bool corrector) {
	ourSink		= as;
	this	-> corrector = corrector;
	aacCap		= NeAACDecGetCapabilities	();
	aacHandle	= NeAACDecOpen			();
	aacConf		= NeAACDecGetCurrentConfiguration (aacHandle);
	aacInitialized	= false;
	baudRate	= 48000;
	myConverter	= new newConverter ();
//	fprintf (stderr, "for 4096 samples in krijgen we %d samples uit\n",
//	                   myConverter -> getOutputSize ());
	tempBuffer	= new DSPCOMPLEX [480];
}

	~faadDecoder	(void) {
	NeAACDecClose	(aacHandle);
	delete		myConverter;
	delete []	tempBuffer;
}

int16_t	MP42PCM (uint8_t buffer [], int16_t bufferLength) {
int16_t	len;
int16_t	i;
int16_t	samples;
uint8_t	channels;
long unsigned int	sample_rate;
int16_t	*outBuffer;
NeAACDecFrameInfo	hInfo;

	if (!aacInitialized) {
	   len = NeAACDecInit (aacHandle, 
	                       buffer, bufferLength, &sample_rate, &channels);
	   if (len < 0) {
	      fprintf (stderr, "Cannot handle this frame\n");
	      return 0;
	   }

	   outBuffer = (int16_t *)NeAACDecDecode (aacHandle,
	                               &hInfo,
	                               &buffer [len],
	                               (uint64_t)(bufferLength - len));
	   aacInitialized = true;
	}
	else 
	   outBuffer = (int16_t *)NeAACDecDecode (aacHandle,
	                                          &hInfo,
	                                          buffer,
	                                          (uint64_t)bufferLength);

	sample_rate	= hInfo. samplerate;
	samples		= hInfo. samples;
	if ((sample_rate == 24000) ||
	    (sample_rate == 48000) ||
	    (sample_rate != baudRate))
	      baudRate = sample_rate;
	
//	fprintf (stderr, "bytes consumed %d\n", (int)(hInfo. bytesconsumed));
//	fprintf (stderr, "samplerate = %d, samples = %d, channels = %d, error = %d, sbr = %d\n", sample_rate, samples,
//	         hInfo. channels,
//	         hInfo. error,
//	         hInfo. sbr);
//	fprintf (stderr, "header = %d\n", hInfo. header_type);
	channels	= hInfo. channels;
	if (hInfo. error != 0) {
	   fprintf (stderr, "Warning: %s\n",
	               faacDecGetErrorMessage (hInfo. error));
	   return 0;
	}
	   
	if (channels == 2 && corrector) {
	   int16_t	amount;
	   for (i = 0; i < samples / 2; i ++) {
	      if (myConverter ->  add (outBuffer [2 * i],
	                               outBuffer [2 * i + 1],
	                               tempBuffer, &amount)) {
	         ourSink	-> putSamples (tempBuffer, amount);
	      }
	   }
	}
	else
	if (channels == 2) 
	   ourSink -> audioOut (outBuffer, samples / 2);
	else
	if (channels == 1) {
	   int16_t *buffer = (int16_t *)alloca (2 * samples);
	   int16_t i;
	   for (i = 0; i < samples; i ++) {
	      buffer [2 * i]	= ((int16_t *)outBuffer) [i];
	      buffer [2 * i + 1] = buffer [2 * i];
	   }
	   ourSink -> audioOut (buffer, samples);
	}
	else
	   fprintf (stderr, "Cannot handle these channels\n");

	return samples / 2;
}

};
