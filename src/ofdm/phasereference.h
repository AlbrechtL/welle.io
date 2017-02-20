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
#ifndef	__PHASEREFERENCE
#define	__PHASEREFERENCE

#include	"fft.h"
#include	<stdio.h>
#include	<stdint.h>
#include	"phasetable.h"
#include	"dab-constants.h"


class phaseReference : public phaseTable {
public:
		phaseReference (DabParams *, int16_t);
		~phaseReference	(void);
	int32_t	findIndex	(DSPCOMPLEX *);
	DSPCOMPLEX	*refTable;
private:
	int32_t		Tu;
	int16_t		threshold;

	common_fft	*fft_processor;
	DSPCOMPLEX	*fft_buffer;
	common_ifft	*res_processor;
	DSPCOMPLEX	*res_buffer;
	int32_t		fft_counter;
	DSPFLOAT	Max;
};
#endif

