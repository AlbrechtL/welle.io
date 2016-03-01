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
#ifndef	__RTP_STREAMER
#define	__RTP_STREAMER

#include	<dab-constants.h>
#include	<QString>
#include	<ringbuffer.h>
#include	"rtpsession.h"
#include	"rtpsessionparams.h"
#include	"rtpudpv4transmitter.h"
#include	"rtpipv4address.h"
#include	"rtptimeutilities.h"
#include	"rtppacket.h"

using namespace jrtplib;
class rtpStreamer {
public:
			rtpStreamer (QString name, int32_t port, RingBuffer<float> *);
			~rtpStreamer (void);
	void		putSamples (int n);
private:
	QString		theName;
	int32_t		thePort;
RingBuffer<float>	*theBuffer;
RingBuffer<float>	*inBuffer;
	RTPSession	session;
	RTPSessionParams sessionparams;
	RTPUDPv4TransmissionParams transparams;
	void		sendBuffer	(uint8_t *, int16_t);
	float		left [481];
	float		right [481];
	uint8_t		buffer [1024];
	int16_t		fillP;
	int16_t		convIndex;
	int		mapTable_int [481];
	float		mapTable_float [481];
};
#endif

