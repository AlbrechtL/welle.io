#
/*
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
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

#include	"tcp-streamer.h"

		tcpStreamer::tcpStreamer	(RingBuffer<int16_t> *b,
	                                         int32_t port):
	                                             audioBase (b) {
	buffer			= new RingBuffer<float> (2 * 32768);
	this	-> port		= port;
	connected		= false;
//	Now for the communication
	connect (&streamer, SIGNAL (newConnection (void)),
	                this, SLOT (acceptConnection (void)));
	streamer. listen (QHostAddress::Any, port);
	connect (this, SIGNAL (handleSamples (void)),
	         this, SLOT (processSamples (void)));
}

		tcpStreamer::~tcpStreamer	(void) {
}

void	tcpStreamer::acceptConnection (void) {
	if (connected) {
	   fprintf (stderr, "attempt to bind, but already occuppied\n");
	   return;
	}

	streamerAddress = streamer. nextPendingConnection ();
	QHostAddress s = streamerAddress -> peerAddress ();
	fprintf (stderr, "Accepted a client %s\n", s.toString (). toLatin1 (). data ());
	connected	= true;
}

//
//	sendSamples is called and acts when we are connected

#define	largeValue	32768.0
#define	bufferSize	(2 * 4096)
//
//	
void	tcpStreamer::audioOutput (float *b, int32_t amount) {
	if (!connected)
	   return;
	buffer -> putDataIntoBuffer (b, 2 * amount);
	if (buffer -> GetRingBufferReadAvailable () > bufferSize)
	   emit handleSamples ();
}


void	tcpStreamer::processSamples (void) {
QByteArray	datagram;
float		localBuffer [bufferSize];
int16_t	i;
int32_t		amount;
	if (!connected) {
	   buffer -> FlushRingBuffer ();
	   return;
	}
//	an encoded sample takes 2 input values and delivers 4 bytes
//	It is assumed that the "sound values" do not exceed 16 bits
	datagram. resize (2 * bufferSize);
	while (buffer -> GetRingBufferReadAvailable () > bufferSize) {
	   amount = buffer -> getDataFromBuffer (localBuffer, bufferSize);
	   for (i = 0; i < amount / 2; i ++) {
	      int16_t re = localBuffer [2 * i] * largeValue;
	      int16_t im = localBuffer [2 * i + 1] * largeValue;
	      datagram [4 * i + 0] = ((re & 0xFF00) >>  8) & 0xFF;
	      datagram [4 * i + 1] =  (re & 0x00FF) & 0xFF;

	      datagram [4 * i + 2] = ((im & 0xFF00) >>  8) & 0xFF;
	      datagram [4 * i + 3] =  (im & 0x00FF) & 0xFF;
	   }

	   if (streamerAddress -> state () ==
	                   QAbstractSocket::UnconnectedState) {
	      fprintf (stderr, "unconnected state\n");
	      streamer. close ();
	      streamer. listen (QHostAddress::Any, port);
	      connected	= false;
	      return;
	   }

	   streamerAddress -> write (datagram. data (), datagram. size ());
	}
}


