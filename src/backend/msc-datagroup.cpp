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
#include	"dab-constants.h"
#include	"msc-datagroup.h"
#include	"deconvolve.h"
#include	"gui.h"

#define	SERVER	"127.0.0.1"
#define	PORT	100241
#define	BUFLEN	512
//
//	Interleaving is - for reasons of simplicity - done
//	inline rather than through a special class-object
//	We could make a single handler for interleaving
//	and deconvolution

//	The main fnction of this class is to assemble the 
//	MSCdatagroups and dispatch to the appropriate handler
static
int8_t	interleaveDelays [] = {
	     15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0};
//
//
//	fragmentsize == Length * CUSize
	mscDatagroup::mscDatagroup	(RadioInterface *,
	                         	 uint8_t DSCTy,
	                         	 int16_t packetAddress,
	                         	 int16_t fragmentSize,
	                         	 int16_t bitRate,
	                         	 int16_t uepFlag,
	                         	 int16_t protLevel,
	                                 uint8_t DGflag,
	                         	 int16_t FEC_scheme) {
int32_t i, j;
	this	-> DSCTy	= DSCTy;
	this	-> packetAddress	= packetAddress;
	this	-> fragmentSize	= fragmentSize;
	this	-> bitRate	= bitRate;
	this	-> uepFlag	= uepFlag;
	this	-> protLevel	= protLevel;
	this	-> DGflag	= DGflag;
	this	-> FEC_scheme	= FEC_scheme;

	outV			= new uint8_t [2 * bitRate * 24];
	interleaveData		= new int16_t *[fragmentSize]; // the size
	for (i = 0; i < fragmentSize; i ++) {
	   interleaveData [i] = new int16_t [16];
	   for (j = 0; j < 16; j ++)
	      interleaveData [i][j] = 0;
	}
	countforInterleaver	= 0;

	uepProcessor		= NULL;
	eepProcessor		= NULL;
	if (uepFlag == 0)
	   uepProcessor	= new uep_deconvolve (bitRate,
	                                      protLevel);
	else
	   eepProcessor	= new eep_deconvolve (bitRate,
	                                      protLevel);
//
	Buffer		= new RingBuffer<int16_t>(64 * 32768);
	packetState	= 0;
	streamAddress	= -1;
//
//	todo: we should make a class for each of the
//	recognized DSCTy values
	if (DSCTy == 59) {	// embedded IP
	   memset ((char *)(&si_other), 0, sizeof (si_other));
	   si_other. sin_family	= AF_INET;
	   si_other. sin_port	= htons (PORT);
	   socketAddr		= socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	   if (socketAddr == -1)
	      fprintf (stderr, "cannot open socket\n");
	   else
	      if (inet_aton (SERVER, &si_other. sin_addr) == 0) {
	         fprintf (stderr, "inet_aton () failed\n");
	   }
	}
	start ();
}

	mscDatagroup::~mscDatagroup	(void) {
int16_t	i;
	running = false;
	while (this -> isRunning ())
	   usleep (1);
	delete Buffer;
	if (uepFlag == 0)
	   delete uepProcessor;
	else
	   delete eepProcessor;
	delete[]	outV;
	for (i = 0; i < fragmentSize; i ++)
	   delete[] interleaveData [i];
	delete[]	interleaveData;
}

int32_t	mscDatagroup::process	(int16_t *v, int16_t cnt) {
int32_t	fr;
	   while ((fr = Buffer -> GetRingBufferWriteAvailable ()) < cnt) {
	      if (!running)
	         return 0;
	      usleep (10);
	   }

	   Buffer	-> putDataIntoBuffer (v, cnt);
	   Locker. wakeAll ();
	   return fr;
}

void	mscDatagroup::run	(void) {
int32_t	countforInterleaver	= 0;
uint8_t	shiftRegister [9];
int16_t	Data [fragmentSize];
int16_t	i, j;

	running	= true;
	while (running) {
	   while (Buffer -> GetRingBufferReadAvailable () < fragmentSize) {
	      ourMutex. lock ();
	      Locker. wait (&ourMutex, 1);	// 1 msec waiting time
	      ourMutex. unlock ();
	      if (!running)
	         break;
	   }

	   if (!running) 
	      break;

	   Buffer	-> getDataFromBuffer (Data, fragmentSize);
//
	   for (i = 0; i < fragmentSize; i ++) {
	      interleaveData [i][interleaveDelays [i & 017]] = Data [i];
	      Data [i] = interleaveData [i] [0];
//	and shift
	      memmove (&interleaveData [i][0],
	               &interleaveData [i][1],
	               interleaveDelays [i & 017] * sizeof (int16_t));
	   }
//
//	only continue when de-interleaver is filled
	   if (countforInterleaver <= 15) {
	      countforInterleaver ++;
	      continue;
	   }
//
	   if (uepFlag == 0)
	      uepProcessor -> deconvolve (Data, fragmentSize, outV);
	   else
	      eepProcessor -> deconvolve (Data, fragmentSize, outV);
//
//	and the inline energy dispersal
	   memset (shiftRegister, 1, 9);
	   for (i = 0; i < bitRate * 24; i ++) {
	      uint8_t b = shiftRegister [8] ^ shiftRegister [4];
	      for (j = 8; j > 0; j--)
	         shiftRegister [j] = shiftRegister [j - 1];
	      shiftRegister [0] = b;
	      outV [i] ^= b;
	   }
//	What we get here is a long sequence of bits, not packed
//	but forming a DAB packet
//	we hand it over to make an MSC data group
//	There is - obviously - some exception, that is
//	when the DG flag is on and there are no datagroups for DSCTy5
	   if ((this -> DSCTy == 5) &&
	       (this -> DGflag))	// no datagroups
	      handleTDCAsyncstream (outV, 24 * bitRate);
	   else
	      handlePackets (outV, 24 * bitRate);
	}
}
//
//	It might take a msec for the task to stop
void	mscDatagroup::stopRunning (void) {
	running = false;
	while (this -> isRunning ())
	   usleep (100);
}
//
//	While for a full mix there will be a single packet in a
//	data compartment, however, it might be the case that there
//	are more!!!
void	mscDatagroup::handlePackets (uint8_t *data, int16_t length) {
	while (length / 8 > 0) {
	   handlePacket (data, length);
	   length -= (getBits_2 (data, 0) + 1) * 24 * 8;
	   data	= &(data [(getBits_2 (data, 0) + 1) * 24 * 8]);
	}
}
//
//	Handle a single DAB packet:
//	Note, although nit yet encountered, the standard says that
//	there may be multiple streams, to be identified by
//	the address. For the time being we only handle a single
//	stream!!!!
void	mscDatagroup::handlePacket (uint8_t *data, int16_t length) {
int16_t	packetLength	= (getBits_2 (data, 0) + 1) * 24;
int16_t	continuityIndex	= getBits_2 (data, 2);
int16_t	firstLast	= getBits_2 (data, 4);
int16_t	address		= getBits   (data, 6, 10);
uint16_t command	= getBits_1 (data, 16);
int16_t	usefulLength	= getBits_7 (data, 17);
int16_t	i;

	if (!check_mscCRC (data, packetLength * 8))
	   return;
	if (address == 0)
	   return;		// padding packet
	if (streamAddress == -1)
	   streamAddress = address;
	if (streamAddress != address)	// sorry
	   return;

//	assemble the full MSC datagroup
	if (packetState == 0) {	// waiting for a start
	   if (firstLast == 02) {	// first packet
	      packetState = 1;
	      series. resize (usefulLength * 8);
	      for (i = 0; i < series. size (); i ++)
	         series [i] = data [24 + i];
	   }
	   else
	   if (firstLast == 03) {	// single packet, mostly padding
	      series. resize (usefulLength * 8);
	      for (i = 0; i < series. size (); i ++)
	         series [i] = data [24 + i];
	      handleMSCdatagroup (series);
	   }
	   else 
	      series. resize (0);	// packetState remains 0
	}
	else
	if (packetState == 01) {	// within a series
	   if (firstLast == 0) {	// intermediate packet
	      int16_t currentLength = series. size ();
	      series. resize (currentLength + 8 * usefulLength);
	      for (i = 0; i < 8 * usefulLength; i ++)
	         series [currentLength + i] = data [24 + i];
	   }
	   else
	   if (firstLast == 01) {	// last packet
	      int16_t currentLength = series. size ();
	      series. resize (currentLength + 8 * usefulLength);
	      for (i = 0; i < 8 * usefulLength; i ++)
	         series [currentLength + i] = data [24 + i];
	      handleMSCdatagroup (series);
	      packetState = 0;
	   }
	   else
	   if (firstLast == 02) {	// first packet, previous one erroneous
	      packetState = 1;
	      series. resize (usefulLength * 8);
	      for (i = 0; i < series. size (); i ++)
	         series [i] = data [24 + i];
	   }
	   else {
	      packetState = 0;
	      series. resize (0);
	   }
	}
}
//
//	It took a while but finally, we have a MSCdatagroup
void	mscDatagroup::handleMSCdatagroup (QByteArray msc) {
uint8_t *data		= (uint8_t *)(msc. data ());
bool	extensionFlag	= getBits_1 (data, 0) != 0;
bool	crcFlag		= getBits_1 (data, 1) != 0;
bool	segmentFlag	= getBits_1 (data, 2) != 0;
bool	userAccessFlag	= getBits_1 (data, 3) != 0;
uint8_t	groupType	= getBits_4 (data, 4);
uint8_t	CI		= getBits_4 (data, 8);
int16_t	next		= 16;		// bits
bool	lastSegment	= false;
uint16_t segmentNumber	= 0;
bool transportIdFlag	= false;
uint16_t transportId	= 0;
uint8_t	lengthInd;
int16_t	i; int32_t checkSum	= 0;

	if (msc. size () == 0)
	   return;
	if (crcFlag && !check_mscCRC (data, msc.size ())) 
	   return;

	if (extensionFlag)
	   next += 16;

	if (segmentFlag) {
	   lastSegment	= getBits_1 (data, next) != 0;
	   segmentNumber = getBits (data, next + 1, 15);
	   next += 16;
	}

	if (userAccessFlag) {
	   transportIdFlag	= getBits_1 (data, next + 3);
	   lengthInd		= getBits_4 (data, next + 4);
	   next	+= 8;
	   if (transportIdFlag) {
	      transportId = getBits (data, next, 16);
	   }
	   next	+= lengthInd * 8;
	}

	checkSum = 0;
	uint16_t	ipLength	= 0;
	switch (DSCTy) {
	   case 5:		// TPEG channel
//	      fprintf (stderr, "mode 5, groupType = %d\n", groupType);
	      break;

	   case 59:		// embedded IP
	      ipLength = getBits (data, next + 16, 16);
	      if (ipLength < msc. size () / 8) {	// just to be sure
	         QByteArray ipVector;
	         ipVector. resize (ipLength);
	         for (i = 0; i < ipLength; i ++)
	            ipVector [i] = getBits_8 (data, next + 8 * i);
	         if ((ipVector [0] >> 4) != 4)
	            return;	// should be version 4
	         process_ipVector (ipVector);
	      }
	      break;

	   case 60:		// MOT
	      processMOT (&data [next], 
	                  msc. size () - next - (crcFlag != 0 ? 16 : 0),
	                  groupType,
	                  lastSegment,
	                  segmentNumber,
	                  transportIdFlag,
	                  transportId);
	      break;
	   default:
	      fprintf (stderr, "MSCdatagroup met groupType %d\n", groupType);
	}
}

void	mscDatagroup::process_ipVector (QByteArray v) {
uint8_t	*data		= (uint8_t *)(v. data ());
int16_t	headerSize	= data [0] & 0x0F;	// in 32 bits words
int16_t ipSize		= (data [2] << 8) | data [3];
uint8_t	protocol	= data [9];

uint32_t checkSum	= 0;
int16_t	i;

	for (i = 0; i < 2 * headerSize; i ++)
	   if (i != 5)
	      checkSum +=  (data [2 * i] << 8) | data [2 * i + 1];
	checkSum = (checkSum >> 16) + (checkSum & 0xFFFF);
	                 
	switch (protocol) {
	   case 17:			// UDP protocol
	      process_udpVector (&data [4 * headerSize], ipSize - 4 * headerSize);
	      return;
	   default:
	      return;
	}
}
//
//	We keep it simple now, just hand over the data from the
//	udp packet to port 8888
void	mscDatagroup::process_udpVector (uint8_t *data, int16_t length) {
char *message = (char *)(&(data [8]));

	if (socketAddr != -1)
	   if (sendto (socketAddr,
	               message,
	               length - 8,
	               0,
	               (struct sockaddr *)&si_other,
	               sizeof (si_other)) == -1)
	   fprintf (stderr, "sorry, send did not work\n");
}
//
//	MOT should be handled in a separate object (todo)
void	mscDatagroup::processMOT (uint8_t	*data,
	                          int16_t	length,
	                          uint8_t	groupType,
	                          bool		lastSegment,
	                          int16_t	segmentNumber,
	                          bool		transportIdFlag,
	                          uint16_t	transportId) {
	(void)length;
	if (!transportIdFlag)
	   return;		// sorry

uint16_t segmentSize	= getBits (data, 3, 13);
uint8_t repetitionCount	= getBits_3 (data, 0);

	if ((segmentNumber == 0) && (groupType == 3)) { // header
	   uint32_t bodySize	= getLBits (data, 16, 28);
	   uint32_t headerSize	= getBits (data, 16 + 28, 13);
	   uint8_t  contentType	= getBits_6 (data, 16 + 41);
	   uint16_t subType	= getBits (data, 16 + 47,  9);
	   fprintf (stderr, "new MOT header %d (sizes %d %d), segment %d, content %d (%d)\n", 
	         transportId, bodySize, headerSize,
	         segmentNumber, contentType, subType);
	}
	else
	if ((segmentNumber == 0) && (groupType == 6)) {	// MOT directory
	   int32_t directorySize	= getLBits (data, 18, 30);
	   int32_t numofObjects		= getBits  (data, 48, 16);
	   int32_t carousselPeriod	= getLBits (data, 64, 24);
	   int16_t segmentSize		= getBits  (data, 88, 13);
	   int16_t extensionLength	= getBits (data, 101, 16);
	   fprintf (stderr, "directory %d dirS = %d, numOb = %d, extension = %d\n",
	                      transportId, directorySize, numofObjects, extensionLength);
	}
	else
	if ((groupType == 4) || (groupType == 6)) {
	   fprintf (stderr, "  groupType = %d, Ti = %d, segmentNumber %d, segmentSize %d last %s\n",
	            groupType,
	            transportId,
	            segmentNumber,
	            segmentSize, lastSegment ? "true" : "false");
	}
	else
	   fprintf (stderr, "grouptype = %d, Ti = %d, sn = %d, ss = %d\n",
	                     groupType, transportId, segmentNumber, segmentSize);
}
//
//	Really no idea what to do here
void	mscDatagroup::handleTDCAsyncstream (uint8_t *data, int16_t length) {
int16_t	packetLength	= (getBits_2 (data, 0) + 1) * 24;
int16_t	continuityIndex	= getBits_2 (data, 2);
int16_t	firstLast	= getBits_2 (data, 4);
int16_t	address		= getBits   (data, 6, 10);
uint16_t command	= getBits_1 (data, 16);
int16_t	usefulLength	= getBits_7 (data, 17);

	(void)	length;
	(void)	packetLength;
	(void)	continuityIndex;
	(void)	firstLast;
	(void)	address;
	(void)	command;
	(void)	usefulLength;
	if (!check_mscCRC (data, packetLength * 8))
	   return;
}
//
//
static
const uint8_t crcPolynome [] =
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};	// MSB .. LSB

bool	mscDatagroup::check_mscCRC (uint8_t *in, int16_t size) {
int16_t	i, f;
uint8_t	b [16];
int16_t	Sum	= 0;

	memset (b, 1, 16);

	for (i = size - 16; i < size; i ++)
	   in [i] ^= 1;

	for (i = 0; i < size; i++) {
	   if ((b [0] ^ in [i]) == 1) {
	      for (f = 0; f < 15; f++) 
	         b [f] = crcPolynome [f] ^ b[f + 1];
	      b [15] = 1;
	   }
	   else {
	      memmove (&b [0], &b[1], sizeof (uint8_t ) * 15); // Shift
	      b [15] = 0;
	   }
	}

	for (i = 0; i < 16; i++)
	   Sum += b [i];

	return Sum == 0;
}

