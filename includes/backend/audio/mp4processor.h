#
/*
 *    Copyright (C) 2013, 2014
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
#ifndef	MP4PROCESSOR
#define	MP4PROCESSOR
/*
 * 	Handling superframes for DAB+ and delivering
 * 	frames into the ffmpeg or faad decoding library
 */
//
#include	"dab-constants.h"
#include	<stdio.h>
#include	<stdint.h>
#include	"audio-base.h"
#include	"dab-processor.h"
#include	"faad-decoder.h"
#include	"firecode-checker.h"
#include	"reed-solomon.h"
#include	<QObject>
#include	"pad-handler.h"

class	RadioInterface;

class	mp4Processor : public QObject, public dabProcessor {
Q_OBJECT
public:
			mp4Processor	(RadioInterface *,
	                                 int16_t,
	                                 RingBuffer<int16_t> *);
			~mp4Processor	(void);
	void		addtoFrame	(uint8_t *);
private:
	RadioInterface	*myRadioInterface;
	padHandler	my_padhandler;
	bool		processSuperframe (uint8_t [], int16_t);
	int16_t		superFramesize;
	int16_t		blockFillIndex;
	int16_t		blocksInBuffer;
	int16_t		blockCount;
	int16_t		bitRate;
	uint8_t		*frameBytes;
	uint8_t		**RSMatrix;
	int16_t		RSDims;
	int16_t		au_start	[10];
	int32_t		baudRate;

	int32_t		au_count;
	int16_t		au_errors;
	int16_t		errorRate;
	firecode_checker	fc;
	reedSolomon	the_rsDecoder;
	uint8_t		*outVector;
//	and for the aac decoder
	faadDecoder	aacDecoder;
	int16_t		frameCount;
	int16_t		frameErrors;
	int16_t		charSet;
signals:
	void		show_successRate		(int);
	void		showLabel			(QString);
	void		isStereo			(bool);
};

#endif


