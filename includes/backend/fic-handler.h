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
/*
 * 	FIC data
 */
#ifndef	__FIC_HANDLER
#define	__FIC_HANDLER

#include	<stdio.h>
#include	<stdint.h>
#include	"viterbi.h"
#include	"viterbi-2.h"
#include	<QObject>
#include	"fib-processor.h"
#include	<QMutex>

class	RadioInterface;
class	mscHandler;

class ficHandler: public QObject, public viterbi {
Q_OBJECT
public:
		ficHandler		(RadioInterface *);
		~ficHandler		(void);
	void	process_ficBlock	(int16_t *, int16_t);
	void	setBitsperBlock		(int16_t);
	void	clearEnsemble		(void);
	int16_t	get_ficRatio		(void);
	uint8_t	kindofService		(QString &);
	void	dataforDataService	(QString &, packetdata *);
	void	dataforAudioService	(QString &, audiodata *);
private:
	void		process_ficInput	(int16_t *, int16_t);
	int8_t		*PI_15;
	int8_t		*PI_16;
	uint8_t		*bitBuffer_in;
	uint8_t		*bitBuffer_out;
	int16_t		*ofdm_input;
	int16_t		index;
	int16_t		BitsperBlock;
	int16_t		ficno;
	int16_t		ficBlocks;
	int16_t		ficMissed;
	int16_t		ficRatio;
	uint16_t	convState;
	QMutex		fibProtector;
	fib_processor	fibProcessor;
	uint8_t		PRBS [768];
	uint8_t		shiftRegister [9];
signals:
	void		show_ficCRC	(bool);
};

#endif


