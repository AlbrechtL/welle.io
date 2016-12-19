#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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
#include	"pad-handler.h"
#include	<cstring>
#include	"gui.h"
#include	"charsets.h"
#include	"mot-data.h"
/**
  *	\class padHandler
  *	Handles the pad sectors passed on from mp4Processor
  */
	padHandler::padHandler	(RadioInterface *mr) {
	myRadioInterface	= mr;
	connect (this, SIGNAL (showLabel (QString)),
	         mr, SLOT (showLabel (QString)));
	my_motHandler	= new motHandler (mr);
}

	padHandler::~padHandler	(void) {
	delete my_motHandler;
}

/**
  *	processPas takes the AU's from the mp4processor and
  *	dispatches the kind of PAD info
  */
void	padHandler::processPAD (uint8_t *theAU) {
uint8_t buffer [255];
int16_t	count	= theAU [1];
///	first we copy the data into a local buffer
	memcpy (buffer, &theAU[2], theAU [1]);
//
//	for now we only handle f_padType 0
	uint8_t	f_padType	= (buffer [count - 2] >> 6) & 03;
	if (f_padType != 00) 
	   return;
//
//	OK, we'll try
	uint8_t x_padInd = (buffer [count - 2] >> 4) & 03;
	if (x_padInd == 01) {
	   handle_shortPAD (buffer, count);
	   return;
	}

	if (x_padInd == 02) {
//	   uint8_t Z_bit		= (buffer [count - 1] & 01);
	   uint8_t CI_flag		= (buffer [count - 1] >> 1) & 01;
	   handle_variablePAD (buffer, count, CI_flag);
	}
}

void	padHandler::handle_shortPAD (uint8_t *b, int16_t count) {
uint8_t CI	= b [count - 3];
uint8_t data [4];
int16_t	i;
	for (i = 0; i < 3; i ++)
	   data [i] = b [count - 4 - i];
	data [3] = 0;
	if ((CI & 037)  == 02 || (CI & 037) == 03)
	   dynamicLabel (data, 3, CI);
}
//
//
//	Here we end up when F_PAD type = 00 and X-PAD Ind = 02
static
int16_t	mapLength (int16_t ind) {
	return ind == 0 ? 4 :
	       ind == 1 ? 6 :
	       ind == 2 ? 8 :
	       ind == 3 ? 12 :
	       ind == 4 ? 16 :
	       ind == 5 ? 24 :
	       ind == 6 ? 32 : 48;
}

void	padHandler::handle_variablePAD (uint8_t *b,
	                                int16_t count, uint8_t CI_flag) {
int16_t	CI_index = 0;
uint8_t CI_table [16];
int16_t	i, j;
int16_t	base	= count - 2 - 1;	// for the F-pad

	if (CI_flag == 0)	// I do not understand
	   return;
//
//	The CI flag in the F_PAD data is set, so we have local CI's
//	7.4.2.2: Contents indicators are one byte long

	while (((b [base] & 037) != 0) && CI_index < 4) {
	   CI_table [CI_index ++] = b [base];
	   base -= 1;
	}
	if (CI_index < 4)	// we have a "0" indicator, adjust base
	   base -= 1;
//
//
	for (i = 0; i < CI_index; i ++) {
	   uint8_t appType = CI_table [i] & 037;
	   int16_t length =  mapLength (CI_table [i] >> 5);

//	with appType == 1 we have a new start, 
	   if (appType == 1) {		// length 4 bytes
	      uint8_t	byte_0	= b [base];
	      uint8_t	byte_1	= b [base - 1];
//
//	still not very clear what the crc is referring to
//	      uint16_t	crc	= (b [base - 2] << 8) | b [base - 3];
	      msc_dataGroupLength	= ((byte_0 & 077) << 8) | byte_1;
//	      fprintf (stderr, "msc_dataGroupLength = %d\n",
//	                                    msc_dataGroupLength);
	      msc_dataGroupIndex	= 0;
	      base -= 4;
	      last_appType = 1;
	      continue;
	   }
//
//	first a check to see whether we are ready to handle
//	the xpad
	   if ((appType != 02) && (appType != 03) &&
	       (appType != 12) && (appType != 13)) {
	      last_appType = appType;
	      return;	// sorry, we do not handle this
	   }

	   uint8_t *data = (uint8_t *)alloca (length + 1);
	   for (j = 0; j < length; j ++)  {
	      data [j] = b [base - j];
	   }
	   data [length] = 0;

	   if ((appType == 02) || (appType == 03)) {
	      dynamicLabel (data, length, CI_table [i]);
	   }
	   else
	   if ((appType == 12) && (last_appType == 01)) {
	      add_MSC_element (data, length);
	   }
	   else
	   if ((appType == 13) &&
	      ((last_appType == 12) || (last_appType == 13))) {
	      add_MSC_element (data, length);
	   }
	  
	   last_appType = appType;
	   base -= length;
	   if (base < 0 && i < CI_index - 1) {
	      fprintf (stderr, "Hier gaat het fout, base = %d\n", base);
	      return;
	   }
	}
}
//
//	A dynamic label is created from a sequence of (dynamic) xpad
//	fields, starting with CI = 2, continuing with CI = 3
void	padHandler::dynamicLabel (uint8_t *data, int16_t length, uint8_t CI) {
static int16_t segmentno           = 0;
static int16_t remainDataLength    = 0;
static bool    isLastSegment       = false;
static bool    moreXPad            = false;
int16_t  dataLength                = 0;

	if ((CI & 037) == 02) {	// start of segment
	   uint16_t prefix = (data [0] << 8) | data [1];
	   uint8_t field_1 = (prefix >> 8) & 017;
	   uint8_t Cflag   = (prefix >> 12) & 01;
	   uint8_t first   = (prefix >> 14) & 01;
	   uint8_t last    = (prefix >> 13) & 01;
	   dataLength = length - 2; // The length is with header removed

	   if (first) { 
	      segmentno = 1;
	      charSet = (prefix >> 4) & 017;
	      dynamicLabelText. clear ();
	   }
	   else 
	      segmentno = (prefix >> 4) & 07 + 1;

	   if (Cflag) {		// special dynamic label command
	      // the only specified command is to clear the display
	      dynamicLabelText. clear ();
	   }
	   else {		// Dynamic text length
	      int16_t totalDataLength = field_1 + 1;
	      if (length - 2 < totalDataLength) {
	         dataLength = length - 2; // the length is shortened by header
	         moreXPad   = true;
	      }
	      else {
	         dataLength = totalDataLength;  // no more xpad app's 3
	         moreXPad   = false;
	      }

//	convert dynamic label
	      QString segmentText = toQStringUsingCharset (
	                                 (const char *)&data [2],
	                                 (CharacterSet) charSet,
	                                 dataLength);

	      dynamicLabelText. append (segmentText);

//	if at the end, show the label
	      if (last) {
	         if (!moreXPad) {
	            showLabel (dynamicLabelText);
	                              
	         }
	         else
	            isLastSegment = true;
	      }
	      else 
	         isLastSegment = false;
//	calculate remaining data length
	      remainDataLength = totalDataLength - dataLength;
	   }
	}
	else 
	if (((CI & 037) == 03) && moreXPad) {
	   if (remainDataLength > length) {
	      dataLength = length;
	      remainDataLength -= length;
	   }
	   else {
	      dataLength = remainDataLength;
	      moreXPad   = false;
	   }
	   
	   QString segmentText = toQStringUsingCharset (
	                              (const char *) data,
	                              (CharacterSet) charSet,
	                              dataLength);
	   dynamicLabelText. append(segmentText);
	   if (!moreXPad && isLastSegment) {
	      showLabel (dynamicLabelText);
	   }
	}
}
//
//
//	building an MSC segment by integrating the elements sent per XPAD
//
void	padHandler::add_MSC_element	(uint8_t *data, int16_t length) {
int16_t	i;

#ifndef	MOT_BASICS__
	return;
#endif
	if (msc_dataGroupIndex < 0)
	   return;
	if (length < 0)
	   return;
	
	if (msc_dataGroupIndex + length >= 8192) {
	   fprintf (stderr, "bagger length = %d (%d)\n", length, msc_dataGroupIndex);
	   return;
	}

	for (i = 0; i < length; i ++)
	   msc_dataGroupBuffer [msc_dataGroupIndex ++] = data [i];
	if (msc_dataGroupIndex < msc_dataGroupLength)
	   return;

	build_MSC_segment (msc_dataGroupBuffer, msc_dataGroupLength);
	msc_dataGroupIndex	= 0;
}

void	padHandler::build_MSC_segment (uint8_t *mscdataGroup, int16_t length) {
//	we have a MOT segment, let us look what is in it
//	according to DAB 300 401 (page 37) the header (MSC data group)
//	is
	uint8_t		groupType	=  mscdataGroup [0] & 0xF;
	uint8_t		continuityIndex = (mscdataGroup [1] & 0xF) >> 4;
	uint8_t		repetitionIndex =  mscdataGroup [1] & 0xF;
	int16_t		segmentNumber	= -1;		// default
	int16_t		transportId	= -1;		// default
	bool		lastFlag	= false;	// default
	uint16_t	index;

	if ((mscdataGroup [0] & 0x40) != 0) {
	   bool res	= pad_crc (mscdataGroup, length - 2);
	   if (!res) {
//	      fprintf (stderr, "crc failed\n");
	      return;
	   }
	}

	if ((groupType != 3) && (groupType != 4))
	   return;		// do not know yet
//
//	extensionflag
	bool	extensionFlag	= (mscdataGroup [0] & 0x80) != 0;
//	if the segmentflag is on, then a lastflag and segmentnumber are
//	available, i.e. 2 bytes more
	index			= extensionFlag ? 4 : 2;
	bool	segmentFlag	=  (mscdataGroup [0] & 0x20) != 0;
	if ((segmentFlag) != 0) {
	   lastFlag		= mscdataGroup [index] & 0x80;
	   segmentNumber	= ((mscdataGroup [index] & 0x7F) << 8) | 
	                            mscdataGroup [index + 1];
	   index += 2;
	}
//
//	if the user access flag is on there is a user accessfield
	if ((mscdataGroup [0] & 0x10) != 0) {
	   int16_t lengthIndicator = mscdataGroup [index] & 0x0F;
	   if ((mscdataGroup [index] & 0x10) != 0) { //transportid flag
	      transportId = mscdataGroup [index + 1] << 8 |
	                    mscdataGroup [index + 2];
	      index += 3;
	   }
	   else {
	      fprintf (stderr, "sorry no transportId\n");
	      return;
	   }
	   
	   index += (lengthIndicator - 2);
	}
//
//	the segment is handled by the mot handler
	my_motHandler	-> process_mscGroup (&mscdataGroup [index],
	                                     groupType,
	                                     lastFlag,
	                                     segmentNumber,
	                                     transportId);
}
//

bool	padHandler::pad_crc (uint8_t *msg, int16_t len) {
int i, j;
uint16_t	accumulator	= 0xFFFF;
uint16_t	crc;
uint16_t	genpoly		= 0x1021;

	for (i = 0; i < len; i ++) {
	   int16_t data = msg [i] << 8;
	   for (j = 8; j > 0; j--) {
	      if ((data ^ accumulator) & 0x8000)
	         accumulator = ((accumulator << 1) ^ genpoly) & 0xFFFF;
	      else
	         accumulator = (accumulator << 1) & 0xFFFF;
	      data = (data << 1) & 0xFFFF;
	   }
	}
//
//	ok, now check with the crc that is contained
//	in the au
	crc	= ~((msg [len] << 8) | msg [len + 1]) & 0xFFFF;
	return (crc ^ accumulator) == 0;
}

