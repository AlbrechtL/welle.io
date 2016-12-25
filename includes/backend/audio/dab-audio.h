#
/*
 *
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
 */
#
#ifndef	DAB_AUDIO
#define	DAB_AUDIO

#include	"dab-virtual.h"
#include	<QThread>
#include	<QMutex>
#include	<QWaitCondition>
#include	"ringbuffer.h"
#include	<stdio.h>

class	dabProcessor;
class	uep_deconvolve;
class	eep_deconvolve;
class	RadioInterface;

class	dabAudio:public QThread, public dabVirtual {
public:
	dabAudio	(uint8_t dabModus,
	                 int16_t fragmentSize,
	                 int16_t bitRate,
	                 int16_t uepFlag,
	                 int16_t protLevel,
	                 RadioInterface *mr,
	                 RingBuffer<int16_t> *);
	~dabAudio	(void);
int32_t	process		(int16_t *, int16_t);
void	stopRunning	(void);
protected:
	RadioInterface	*myRadioInterface;
	RingBuffer<int16_t>	*audioBuffer;
private:
void	run		(void);
volatile bool		running;
	uint8_t		dabModus;
	int16_t		fragmentSize;
	int16_t		bitRate;
	int16_t		uepFlag;
	int16_t		protLevel;
	uint8_t		*outV;
	int16_t		**interleaveData;

	QWaitCondition	Locker;
	QMutex		ourMutex;

	uep_deconvolve	*uepProcessor;
	eep_deconvolve	*eepProcessor;
	dabProcessor	*our_dabProcessor;
	RingBuffer<int16_t>	*Buffer;
};

#endif

