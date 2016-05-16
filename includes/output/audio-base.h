#
/*
 *    Copyright (C)  2009, 2010, 2011
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in ESDR are derived from
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

#ifndef __AUDIO_BASE__
#define	__AUDIO_BASE__
#include	"dab-constants.h"
#include	<stdio.h>
#include	"fir-filters.h"
#include	<sndfile.h>
#include	<QMutex>
#include	<QObject>
#include	"ringbuffer.h"


class	audioBase: public QObject{
Q_OBJECT
public:
			audioBase		(RingBuffer<int16_t> *);
virtual			~audioBase		(void);
virtual	void		stop			(void);
virtual	void		restart			(void);
	void		audioOut		(int);
	void		startDumping		(SNDFILE *);
	void		stopDumping		(void);
private:
	RingBuffer<int16_t>	*buffer;
	void		audioOut_16000		(int16_t *, int32_t);
	void		audioOut_24000		(int16_t *, int32_t);
	void		audioOut_32000		(int16_t *, int32_t);
	void		audioOut_48000		(int16_t *, int32_t);
	LowPassFIR	f_16000;
	LowPassFIR	f_24000;
	LowPassFIR	f_32000;
	SNDFILE		*dumpFile;
	QMutex		myLocker;
protected:
virtual	void		audioOutput		(float *, int32_t);
};
#endif

