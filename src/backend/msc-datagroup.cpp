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
#include	"mot-data.h"
#include	"gui.h"
#include	<QUdpSocket>

#define	SERVER	"127.0.0.1"
#define	PORT	8888
#define	BUFLEN	512
//
//	Interleaving is - for reasons of simplicity - done
//	inline rather than through a special class-object
//	We could make a single handler for interleaving
//	and deconvolution
//
//	\class mscDatagroup
//	The main function of this class is to assemble the 
//	MSCdatagroups and dispatch to the appropriate handler
static
int8_t	interleaveDelays [] = {
	     15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0};


#ifdef	__MINGW32__
//
//      It sems that the function inet_aton is not
//      available under Windows.
//      Author: Paul Vixie, 1996.
#define NS_INADDRSZ  4
bool inet_aton (const char *src, struct in_addr *x) {
char	*dst	= (char *)x;
uint8_t tmp[NS_INADDRSZ], *tp;
int saw_digit = 0;
int octets = 0;

	*(tp = tmp) = 0;
	int ch;
	while ((ch = *src++) != '\0') {
           if (ch >= '0' && ch <= '9') {
              uint32_t n = *tp * 10 + (ch - '0');
	      if (saw_digit && *tp == 0)
	         return 0;
	      if (n > 255)
	         return false;

	      *tp = n;
	      if (!saw_digit) {
	         if (++octets > 4)
	            return false;
	         saw_digit = 1;
	      }
	   }
	   else
	   if (ch == '.' && saw_digit) {
	      if (octets == 4)
	         return false;
	      *++tp = 0;
	      saw_digit = 0;
	   }
	   else
	      return 0;
	}
	if (octets < 4)
	   return false;

	memcpy(dst, tmp, NS_INADDRSZ);
	return true;
}
#endif
//
//	fragmentsize == Length * CUSize
	mscDatagroup::mscDatagroup	(RadioInterface *mr,
	                         	 uint8_t DSCTy,
	                         	 int16_t packetAddress,
	                         	 int16_t fragmentSize,
	                         	 int16_t bitRate,
	                         	 int16_t uepFlag,
	                         	 int16_t protLevel,
	                                 uint8_t DGflag,
	                         	 int16_t FEC_scheme) {
int32_t i, j;
	this	-> myRadioInterface	= mr;
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

	connect (this, SIGNAL (showLabel (const QString &)),
	         mr, SLOT (showLabel (const QString &)));

	switch (DSCTy) {
	   default:
	   case 5:
	      showLabel (QString ("Transparent Channel not implemented"));
	      break;
	   case 60:
	      showLabel (QString ("MOT partially implemented"));
	      break;
	   case 59:
	      showLabel (QString ("Embedded IP: UDP data sent to 8888"));
	      break;
	}
	opt_motHandler	= NULL;

//	todo: we should make a class for each of the
//	recognized DSCTy values
	if (DSCTy == 59) {	// embedded IP
	   memset ((char *)(&si_other), 0, sizeof (si_other));
	   si_other. sin_family	= AF_INET;
	   si_other. sin_port	= htons (PORT);
	   socketAddr		= socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	   if (socketAddr != -1)
	      if (inet_aton (SERVER, &si_other. sin_addr) == 0) {
	         fprintf (stderr, "inet_aton () failed\n");
	   }
	}
	else
	if (DSCTy == 60) 	// MOT
	   opt_motHandler	= new motHandler (mr);
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
	if (DSCTy == 60)
	   delete opt_motHandler;
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
//	data compartment, for an empty mix, there may be many more
void	mscDatagroup::handlePackets (uint8_t *data, int16_t length) {
	while (true) {
	   if (length < (getBits_2 (data, 0) + 1) * 24 * 8)
	      return;
	   handlePacket (data);
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
void	mscDatagroup::handlePacket (uint8_t *data) {
int16_t	packetLength	= (getBits_2 (data, 0) + 1) * 24;
int16_t	continuityIndex	= getBits_2 (data, 2);
int16_t	firstLast	= getBits_2 (data, 4);
int16_t	address		= getBits   (data, 6, 10);
uint16_t command	= getBits_1 (data, 16);
int16_t	usefulLength	= getBits_7 (data, 17);
int16_t	i;

	(void)continuityIndex;
	(void)command;
	if (!check_CRC_bits (data, packetLength * 8))
	   return;
	if (address == 0)
	   return;		// padding packet
//
//	In this early stage we only collect packets for a single
//	i.e. the first, stream
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
///	It took a while but at last, we have a MSCdatagroup
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
int16_t	i;

	(void)CI;
	if (msc. size () == 0)
	   return;
	if (crcFlag && !check_CRC_bits (data, msc.size ())) 
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

	uint16_t	ipLength	= 0;
	int16_t		sizeinBits	=
	              msc. size () - next - (crcFlag != 0 ? 16 : 0);
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
	      if (transportIdFlag) {
	         QByteArray motVector;
	         motVector. resize (sizeinBits / 8);
	         for (i = 0; i < sizeinBits / 8; i ++)
	            motVector [i] = getBits_8 (data, next + 8 * i);

	         processMOT (motVector,
	                     groupType,
	                     lastSegment,
	                     segmentNumber,
	                     transportId);
	      }
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
	      checkSum +=  ((data [2 * i] << 8) | data [2 * i + 1]);
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
void	mscDatagroup::processMOT (QByteArray	d,
	                          uint8_t	groupType,
	                          bool		lastSegment,
	                          int16_t	segmentNumber,
	                          uint16_t	transportId) {
uint8_t	*data		= (uint8_t *)(d. data ());
uint16_t segmentSize	= ((data [0] & 0x1F) << 8) | data [1];

	if ((segmentNumber == 0) && (groupType == 3)) { // header
	   uint32_t headerSize	= ((data [5] & 0x0F) << 9) |
	                           (data [6])              |
	                           (data [7] >> 7);
	   uint32_t bodySize	= (data [2] << 20) |
	                          (data [3] << 12) |
	                          (data [4] << 4 ) |
	                          ((data [5] & 0xF0) >> 4);
	   opt_motHandler	-> processHeader (transportId,
	                                          &data [2],
	                                          segmentSize,
	                                          headerSize,
	                                          bodySize,
	                                          lastSegment);
	                           
	}
	else
	if ((segmentNumber == 0) && (groupType == 6)) 	// MOT directory
	   opt_motHandler	-> processDirectory (transportId,
	                                             &data [2],
	                                             segmentSize,
	                                             lastSegment);
	else
	if (groupType == 6) 	// fields for MOT directory
	   opt_motHandler	-> directorySegment (transportId,
	                                             &data [2],
	                                             segmentNumber,
	                                             segmentSize,
	                                             lastSegment);
	else
	if (groupType == 4) {
//	   fprintf (stderr, "grouptype = %d, Ti = %d, sn = %d, ss = %d\n",
//	                     groupType, transportId, segmentNumber, segmentSize);

	   opt_motHandler	-> processSegment  (transportId,
	                                            data,
	                                            segmentNumber,
	                                            segmentSize,
	                                            lastSegment);
	}
//	else
//	   fprintf (stderr, "grouptype = %d, Ti = %d, sn = %d, ss = %d\n",
//	                     groupType, transportId, segmentNumber, segmentSize);
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
	if (!check_CRC_bits (data, packetLength * 8))
	   return;
}
//
