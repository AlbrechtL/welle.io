#
/*
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
 */
#
#include	"dab-constants.h"
#include	"msc-handler.h"
#include	"gui.h"
#include	"dab-virtual.h"
#include	"dab-serial.h"
#include	"dab-concurrent.h"
//
//	Driver program for processing the MSC.
//	Three operations here (apart from selecting
//	the local frame in the MSC vector)
//	1. deinterleaving
//	2. deconvolution (including depuncturing)
//	3. energy dispersal
//	4. in case of DAB: creating MP2 packets
//	5. in case of DAB+: assembling superframes and creating MP4 packets
//
//	The selected service(s) is (are) to be found in the
//	ficParameters.

#define	CUSize	(4 * 16)
//	Note CIF counts from 0 .. 3
//
		mscHandler::mscHandler	(RadioInterface *mr,
	                                 audioSink	*sink,
	                                 FILE		*errorLog,
	                                 bool	concurrent) {
		myRadioInterface	= mr;
	        concurrencyOn 		= concurrent;
	        cifVector		= new int16_t [55296];
	        cifCount		= 0;	// msc blocks in CIF
	        blkCount		= 0;
	        dabHandler		= new dabVirtual;
	        newChannel		= false;
	        currentChannel		= -1;
	        dabModus		= 0;
	        our_audioSink		= sink;
	        this -> errorLog	= errorLog;
//	assume default Mode I
		BitsperBlock		= 2 * 1536;
	   	numberofblocksperCIF	= 18;
}

		mscHandler::~mscHandler	(void) {
	delete[]  cifVector;
	delete	dabHandler;
}

void	mscHandler::setChannel (int16_t subchId,
	                        int16_t uepFlag,
	                        int16_t	startAddr,
	                        int16_t	Length,
	                        int16_t	protLevel,
	                        int16_t	bitRate,
	                        int16_t	ASCTy,
	                        int16_t	language,
	                        int16_t	type) {
	newChannel	= true;
	currentChannel	= subchId;
	new_uepFlag	= uepFlag;
	new_startAddr	= startAddr;
	new_Length	= Length;
	new_protLevel	= protLevel;
	new_bitRate	= bitRate;
	new_language	= language;
	new_type	= type;
	new_dabModus	= ASCTy == 077 ? DAB_PLUS : DAB;
//	fprintf (stderr, "Preparations for channel select\n");
}
//
void	mscHandler::setMode	(DabParams *p) {
	BitsperBlock	= 2 * p -> K;
	if (p -> dabMode == 4)	// 2 CIFS per 76 blocks
	   numberofblocksperCIF	= 36;
	else
	if (p -> dabMode == 1)	// 4 CIFS per 76 blocks
	   numberofblocksperCIF	= 18;
	else
	if (p -> dabMode == 2)	// 1 CIF per 76 blocks
	   numberofblocksperCIF	= 72;
	else			// shouldnot/cannot happen
	   numberofblocksperCIF	= 18;
}

//
//	add blocks. First is (should be) block 5, last is (should be) 76
void	mscHandler::process_mscBlock	(int16_t *fbits,
	                                 int16_t blkno) { 
int16_t	currentblk;
int16_t	*myBegin;

	if (currentChannel == -1)
	   return;

	currentblk	= (blkno - 5) % numberofblocksperCIF;
//
//	we only change channel at the start of a new frame!!!
	if (newChannel) {
//	if ((blkno - 5 == 0) && newChannel) {
	   newChannel	= false;
	   dabHandler -> stopRunning ();
	   delete dabHandler;

	   if (concurrencyOn)
	      dabHandler = new dabConcurrent (new_dabModus,
	                                      new_Length * CUSize,
	                                      new_bitRate,
	                                      new_uepFlag,
	                                      new_protLevel,
	                                      myRadioInterface,
	                                      errorLog,
	                                      our_audioSink);
	   else
	      dabHandler = new dabSerial (new_dabModus,
	                                  new_Length * CUSize,
	                                  new_bitRate,
	                                  new_uepFlag,
	                                  new_protLevel,
	                                  myRadioInterface,
	                                  errorLog,
	                                  our_audioSink);
	                                  
	   startAddr	= new_startAddr;
	   Length	= new_Length;
	   protLevel	= new_protLevel;
	   bitRate	= new_bitRate;
	}
//
//	and the normal operation is:
//	We might optimize this by realizing that the exact segment
//	we take from the CIF is known.
	memcpy (&cifVector [currentblk * BitsperBlock],
	                    fbits, BitsperBlock * sizeof (int16_t));
	if (currentblk < numberofblocksperCIF - 1) 
	   return;
//
//	OK, now we have a full CIF
	blkCount	= 0;
	cifCount	= (cifCount + 1) & 03;
	myBegin		= &cifVector [startAddr * CUSize];
//
//	Here we move the vector to be processed to a
//	separate task or separate function, depending on
//	the settings in the ini file, we might take advantage of multi cores
	(void) dabHandler -> process (myBegin, Length * CUSize);
}
//
//
uint8_t	mscHandler::getMode	(void) {
	return new_dabModus;
}

int16_t	mscHandler::getChannel	(void) {
	return currentChannel;
}

int16_t	mscHandler::getLanguage (void) {
	return new_language;
}

int16_t	mscHandler::getType (void) {
	return new_type;
}

void	mscHandler::stopProcessing (void) {
	currentChannel = -1;
}

