#
/*
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
 *
 *    This file is part of the SDR-J (JSDR).
 *    Many of the ideas as implemented in JSDR are derived from
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

#ifndef __MIRICS_WORKER
#define	__MIRICS_WORKER

#include	<stdlib.h>
#include	<stdio.h>
#include	<math.h>
#include	<string.h>
#include	<unistd.h>
#include	<stdint.h>
#include	<QThread>
#include	"ringbuffer.h"
#include	"mirics-loader.h"

class	miricsWorker: public QThread {
public:
			miricsWorker	(int32_t,
	                                 int32_t,
	                                 int32_t,
	                                 miricsLoader *,
	                                 RingBuffer<int16_t> *,
	                                 bool *);
			~miricsWorker	(void);
	void		setVFOFrequency	(int32_t);
	void		setExternalGain	(int16_t);
	void		stop		(void);
	void		setExternalRate	(int32_t);
private:
	void			run	(void);
	miricsLoader		*functions;	// 
	RingBuffer<int16_t>	*_I_Buffer;
	float			defaultFreq;
	double			deviceRate;
	double			deltaRate;
	int32_t			bandWidth;
	double			lastFrequency;
	int16_t			lastGain;
	bool			runnable;
	int32_t			sps;
	int32_t			anyChange;
};
#endif

