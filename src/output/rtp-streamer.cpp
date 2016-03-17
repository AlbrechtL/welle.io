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
#include	<dab-constants.h>
#include	<stdio.h>
#include	<sys/stat.h>
#include	"rtp-streamer.h"
#include	<iostream>

	rtpStreamer::rtpStreamer (QString name, int32_t port,
	                          RingBuffer<int16_t> *b):audioBase (b){
int16_t	i;
	theName		= name;
	thePort		= port;
	inBuffer	= new RingBuffer<float> (2 * 32768);
	theBuffer	= new RingBuffer<float> (8 * 32768);

	sessionparams. SetOwnTimestampUnit (1.0 / (44100 * 4));
			
	RTPUDPv4TransmissionParams transparams;
	transparams. SetPortbase(8000);
			
	int status = session. Create (sessionparams, &transparams);
	if (status < 0) {
	   std::cerr << RTPGetErrorString(status) << std::endl;
	   exit(-1);
	}

	uint8_t localip []	= {127, 0, 0, 255};
	RTPIPv4Address addr (localip, port);
	status = session. AddDestination(addr);
	if (status < 0) {
	   std::cerr << RTPGetErrorString(status) << std::endl;
	   exit (-1);
	}
	
	session.SetDefaultPayloadType	(10);
	session.SetDefaultMark		(false);
	session.SetDefaultTimestampIncrement (1024);

	convIndex	= 0;
	for (i = 0; i < 441; i ++) {
	   mapTable_int [i] =  int (floor (i * (480.0 / 441.0)));
           mapTable_float [i] = i * (480.0 / 441.0) - mapTable_int [i];
        }
	fillP		= 0;
	fprintf (stderr, "streamer is er nu\n");
}

	rtpStreamer::~rtpStreamer	(void) {
	delete theBuffer;
}
//
//	simple function to map 48000 -> 44100, needed for streamer
//
//	Simple interpolation using chunks of 2 * 480 values,
//	mapping them onto 2 * 441 output values
//

void	rtpStreamer::audioOutput (float *b, int n) {
int16_t	i, j;
int	status;
	(void)n;

	inBuffer	-> putDataIntoBuffer (b, 2 * n);
	while (inBuffer -> GetRingBufferReadAvailable () > 960) {
	   float v [960];
	   inBuffer	-> getDataFromBuffer (v, 960);
	   for (i = 0; i < 480; i ++) {
	      left  [convIndex]	= v [2 * i];
	      right [convIndex]	= v [2 * i + 1];
	      if (++convIndex > 480) {
	         for (j = 0; j < 441; j ++) {
	            int16_t	inpBase		= mapTable_int [j];
	            float	inpRatio	= mapTable_float [j];
	            int leftS	= (left [inpBase + 1] * inpRatio +
	                           left [inpBase] * (1.0 - inpRatio)) * 32768;
	            int rightS	= (right [inpBase + 1] * inpRatio +
	                           right [inpBase] * (1.0 - inpRatio)) * 32768;

	            buffer [fillP ++]	= (leftS >> 8) & 0xFF;
	            buffer [fillP ++]	= leftS & 0xFF;
	            buffer [fillP ++]	= (rightS >> 8) & 0xFF;
	            buffer [fillP ++]	= rightS & 0xFF;
	            if (fillP >= 1024) {
	               fillP	= 0;
	               sendBuffer (buffer, 1024);
	            }
	         }
	         left	[0]	= left  [480];
	         right	[0]	= right [480];
	         convIndex = 1;
	      }
	   }
	}
}

void	rtpStreamer::sendBuffer (uint8_t *b, int16_t n) {
int	status;

	status = session. SendPacket (buffer, 1024);
	if (status < 0) {
	   std::cerr << RTPGetErrorString(status) << std::endl;
	   exit(-1);
	}
	session. BeginDataAccess ();
	if (session. GotoFirstSource ()) {
	   do {
	      RTPPacket *packet;
	      while ((packet = session.GetNextPacket()) != 0) {
	         std::cout << "Got packet with " 
	                   << "extended sequence number " 
	                   << packet->GetExtendedSequenceNumber() 
	                   << " from SSRC " << packet->GetSSRC() 
	                   << std::endl;
	         session.DeletePacket(packet);
	      }
	   } while (session. GotoNextSource());
	}
	session.EndDataAccess ();
}

