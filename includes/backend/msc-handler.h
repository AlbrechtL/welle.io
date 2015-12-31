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
 * 	MSC data
 */
#
#ifndef	MSC_DECODER
#define	MSC_DECODER

#include	<stdio.h>
#include	<stdint.h>
#include	"audiosink.h"
#include	"deconvolve.h"
#include	<stdio.h>
#include	"dab-constants.h"

class	RadioInterface;
class	dabVirtual;

class mscHandler {
public:
		mscHandler		(RadioInterface *,
	                                 audioSink *,
	                                 FILE	*);
		~mscHandler		(void);
	void	process_mscBlock	(int16_t *, int16_t);
	void	setMode			(DabParams *);
	void	set_audioChannel	(int16_t, int16_t, int16_t,
	                                 int16_t, int16_t, int16_t, int16_t,
	                                 int16_t, int16_t);
	void    set_dataChannel         (int16_t, int16_t, int16_t,
                                         int16_t, int16_t, int16_t,
	                                 int16_t, uint8_t,
                                         uint8_t, int16_t);
	uint8_t	getMode			(void);
	int16_t	getChannel		(void);
	int16_t	getLanguage		(void);
	int16_t	getType			(void);
	void	stopProcessing		(void);
	void	stop			(void);
	void	setFiles		(FILE *, FILE *);
private:
	bool		audioService;
	RadioInterface	*myRadioInterface;
	FILE		*errorLog;
	dabVirtual	*dabHandler;
	int16_t		*cifVector;
	int16_t		cifCount;
	int16_t		blkCount;
	int16_t		currentChannel;
	bool		newChannel;
	int16_t		new_packetAddress;
	int16_t		new_DSCTy;
	int16_t		new_startAddr;
	int16_t		new_Length;
	int16_t		new_uepFlag;
	int16_t		new_protLevel;
	uint8_t		new_DGflag;
	int16_t		new_bitRate;
	int16_t		new_language;
	int16_t		new_type;
	int16_t		new_FEC_scheme;
	int16_t		packetAddress;
	uint8_t		DSCTy;
	int16_t		startAddr;
	int16_t		Length;
	int16_t		uepFlag;
	int16_t		protLevel;
	uint8_t		DGflag;
	int16_t		bitRate;
	int16_t		FEC_scheme;
	int8_t		dabModus;
	int8_t		new_dabModus;
	int16_t		BitsperBlock;
	int16_t		numberofblocksperCIF;
	int16_t		blockCount;
	audioSink	*our_audioSink;
};

#endif


