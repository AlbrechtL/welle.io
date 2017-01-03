#
/*
 *    Copyright (C) 2015
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
 */
#
#ifndef	__DATA_PROCESSOR
#define	__DATA_PROCESSOR

#include	"dab-processor.h"
#include	"dab-virtual.h"
#include	<stdio.h>
#include	<string.h>
#include	<QObject>

class	RadioInterface;
class	uep_deconvolve;
class	eep_deconvolve;
class	virtual_dataHandler;

class	dataProcessor:public QObject, public dabProcessor {
Q_OBJECT
public:
	dataProcessor	(RadioInterface *mr,
	                 int16_t	bitRate,
	                 uint8_t	DSCTy,
	                 uint8_t	DGflag,
	                 int16_t	FEC_scheme,
	                 bool		show_crcErrors);
	~dataProcessor	(void);
void	addtoFrame	(uint8_t *);
private:
	RadioInterface	*myRadioInterface;
	int16_t		bitRate;
	uint8_t		DSCTy;
	uint8_t		DGflag;
	int16_t		FEC_scheme;
	bool		show_crcErrors;
	int16_t		crcErrors;
	int16_t		handledPackets;
	QByteArray	series;
	uint8_t		packetState;
	int32_t		streamAddress;		// int since we init with -1
//
//	result handlers
	void		handleTDCAsyncstream 	(uint8_t *, int16_t);
	void		handlePackets		(uint8_t *, int16_t);
	void		handlePacket		(uint8_t *);
	virtual_dataHandler *my_dataHandler;
//
signals:
	void		show_mscErrors		(int);
};

#endif

