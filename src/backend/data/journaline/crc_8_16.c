/* 
 *
 * This file is part of the 'NewsService Journaline(R) Decoder'
 * 
 * Copyright (c) 2003, 2001-2014 by Fraunhofer IIS, Erlangen, Germany
 * 
 * --------------------------------------------------------------------
 * 
 * For NON-COMMERCIAL USE,
 * the 'NewsService Journaline(R) Decoder' is free software;
 * you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * The 'NewsService Journaline(R) Decoder' is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with the 'NewsService Journaline(R) Decoder';
 * if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * 
 * If you use this software in a project with user interaction, please
 * provide the following text to the user in an appropriate place:
 * "Features NewsService Journaline(R) decoder technology by
 * Fraunhofer IIS, Erlangen, Germany.
 * For more information visit http://www.iis.fhg.de/dab"
 * 
 * --------------------------------------------------------------------
 * 
 * To use the 'NewsService Journaline(R) Decoder' software for
 * COMMERCIAL purposes, please contact Fraunhofer IIS for a
 * commercial license (see below for contact information)!
 * 
 * --------------------------------------------------------------------
 * 
 * Contact:
 *   Fraunhofer IIS, Department 'Broadcast Applications'
 *   Am Wolfsmantel 33, 91058 Erlangen, Germany
 *   http://www.iis.fraunhofer.de/dab
 *   mailto:bc-info@iis.fraunhofer.de
 * 
 */

/*!
 * @file crc_8_16.c
 * @brief CRC calculation
 *
 * functions for calculating and checking 8bit or 16bit
 * CRC (cyclic redundancy check) checksums.
 * 
 * For 8 bit CRC calculation, the polynomial used is
 * \f$ x^8+x^4+x^3+x^2+1 \f$
 *
 * For 16 bit CRC calculations, the polynomial used is
 * \f$ x^{16}+x^{12}+x^5+1 \f$
 *
 * $Id: crc_8_16.c,v 1.1 2008/12/17 17:15:54 jcable Exp $
 *
 * Author:      Monica Redon Segrera and Nuria Llombart Juan
 *
 * Copyright:  (C) 2003-2001-2014 by Fraunhofer IIS-A, IT-Services, Erlangen
 *
 * Created:     2001-02-01
 *
 * Version:     2001-2014-03-02
 */

#include "crc_8_16.h"

/*! Global variable in which the CRC-16 table is stored */ 
static const unsigned short CRC_Table_16[256] = {
   0x0,   0x1021,   0x2042,   0x3063,   0x4084,   0x50A5,   0x60C6,   0x70E7,   
0x8108,   0x9129,   0xA14A,   0xB16B,   0xC18C,   0xD1AD,   0xE1CE,   0xF1EF,   
0x1231,    0x210,   0x3273,   0x2252,   0x52B5,   0x4294,   0x72F7,   0x62D6,   
0x9339,   0x8318,   0xB37B,   0xA35A,   0xD3BD,   0xC39C,   0xF3FF,   0xE3DE,   
0x2462,   0x3443,    0x420,   0x1401,   0x64E6,   0x74C7,   0x44A4,   0x5485,   
0xA56A,   0xB54B,   0x8528,   0x9509,   0xE5EE,   0xF5CF,   0xC5AC,   0xD58D,   
0x3653,   0x2672,   0x1611,    0x630,   0x76D7,   0x66F6,   0x5695,   0x46B4,   
0xB75B,   0xA77A,   0x9719,   0x8738,   0xF7DF,   0xE7FE,   0xD79D,   0xC7BC,   
0x48C4,   0x58E5,   0x6886,   0x78A7,    0x840,   0x1861,   0x2802,   0x3823,   
0xC9CC,   0xD9ED,   0xE98E,   0xF9AF,   0x8948,   0x9969,   0xA90A,   0xB92B,   
0x5AF5,   0x4AD4,   0x7AB7,   0x6A96,   0x1A71,    0xA50,   0x3A33,   0x2A12,   
0xDBFD,   0xCBDC,   0xFBBF,   0xEB9E,   0x9B79,   0x8B58,   0xBB3B,   0xAB1A,   
0x6CA6,   0x7C87,   0x4CE4,   0x5CC5,   0x2C22,   0x3C03,    0xC60,   0x1C41,   
0xEDAE,   0xFD8F,   0xCDEC,   0xDDCD,   0xAD2A,   0xBD0B,   0x8D68,   0x9D49,   
0x7E97,   0x6EB6,   0x5ED5,   0x4EF4,   0x3E13,   0x2E32,   0x1E51,   0xE70,   
0xFF9F,   0xEFBE,   0xDFDD,   0xCFFC,   0xBF1B,   0xAF3A,   0x9F59,   0x8F78,   
0x9188,   0x81A9,   0xB1CA,   0xA1EB,   0xD10C,   0xC12D,   0xF14E,   0xE16F,   
0x1080,     0xA1,   0x30C2,   0x20E3,   0x5004,   0x4025,   0x7046,   0x6067,   
0x83B9,   0x9398,   0xA3FB,   0xB3DA,   0xC33D,   0xD31C,   0xE37F,   0xF35E,   
 0x2B1,   0x1290,   0x22F3,   0x32D2,   0x4235,   0x5214,   0x6277,   0x7256,   
0xB5EA,   0xA5CB,   0x95A8,   0x8589,   0xF56E,   0xE54F,   0xD52C,   0xC50D,   
0x34E2,   0x24C3,   0x14A0,    0x481,   0x7466,   0x6447,   0x5424,   0x4405,   
0xA7DB,   0xB7FA,   0x8799,   0x97B8,   0xE75F,   0xF77E,   0xC71D,   0xD73C,   
0x26D3,   0x36F2,    0x691,   0x16B0,   0x6657,   0x7676,   0x4615,   0x5634,   
0xD94C,   0xC96D,   0xF90E,   0xE92F,   0x99C8,   0x89E9,   0xB98A,   0xA9AB,   
0x5844,   0x4865,   0x7806,   0x6827,   0x18C0,    0x8E1,   0x3882,   0x28A3,   
0xCB7D,   0xDB5C,   0xEB3F,   0xFB1E,   0x8BF9,   0x9BD8,   0xABBB,   0xBB9A,   
0x4A75,   0x5A54,   0x6A37,   0x7A16,    0xAF1,   0x1AD0,   0x2AB3,   0x3A92,   
0xFD2E,   0xED0F,   0xDD6C,   0xCD4D,   0xBDAA,   0xAD8B,   0x9DE8,   0x8DC9,   
0x7C26,   0x6C07,   0x5C64,   0x4C45,   0x3CA2,   0x2C83,   0x1CE0,    0xCC1,   
0xEF1F,   0xFF3E,   0xCF5D,   0xDF7C,   0xAF9B,   0xBFBA,   0x8FD9,   0x9FF8,   
0x6E17,   0x7E36,   0x4E55,   0x5E74,   0x2E93,   0x3EB2,    0xED1,   0x1EF0 
};

/*! Global variable in which the CRC-8 table is stored */ 
static const unsigned char CRC_Table_8[256] = {
 0x0,   0x1D,   0x3A,   0x27,   0x74,   0x69,   0x4E,   0x53,   0xE8,   0xF5,   
0xD2,   0xCF,   0x9C,   0x81,   0xA6,   0xBB,   0xCD,   0xD0,   0xF7,   0xEA,   
0xB9,   0xA4,   0x83,   0x9E,   0x25,   0x38,   0x1F,    0x2,   0x51,   0x4C,   
0x6B,   0x76,   0x87,   0x9A,   0xBD,   0xA0,   0xF3,   0xEE,   0xC9,   0xD4,   
0x6F,   0x72,   0x55,   0x48,   0x1B,    0x6,   0x21,   0x3C,   0x4A,   0x57,   
0x70,   0x6D,   0x3E,   0x23,    0x4,   0x19,   0xA2,   0xBF,   0x98,   0x85,   
0xD6,   0xCB,   0xEC,   0xF1,   0x13,    0xE,   0x29,   0x34,   0x67,   0x7A,   
0x5D,   0x40,   0xFB,   0xE6,   0xC1,   0xDC,   0x8F,   0x92,   0xB5,   0xA8,   
0xDE,   0xC3,   0xE4,   0xF9,   0xAA,   0xB7,   0x90,   0x8D,   0x36,   0x2B,   
 0xC,   0x11,   0x42,   0x5F,   0x78,   0x65,   0x94,   0x89,   0xAE,   0xB3,   
0xE0,   0xFD,   0xDA,   0xC7,   0x7C,   0x61,   0x46,   0x5B,    0x8,   0x15,   
0x32,   0x2F,   0x59,   0x44,   0x63,   0x7E,   0x2D,   0x30,   0x17,    0xA,   
0xB1,   0xAC,   0x8B,   0x96,   0xC5,   0xD8,   0xFF,   0xE2,   0x26,   0x3B,   
0x1C,    0x1,   0x52,   0x4F,   0x68,   0x75,   0xCE,   0xD3,   0xF4,   0xE9,   
0xBA,   0xA7,   0x80,   0x9D,   0xEB,   0xF6,   0xD1,   0xCC,   0x9F,   0x82,   
0xA5,   0xB8,    0x3,   0x1E,   0x39,   0x24,   0x77,   0x6A,   0x4D,   0x50,   
0xA1,   0xBC,   0x9B,   0x86,   0xD5,   0xC8,   0xEF,   0xF2,   0x49,   0x54,   
0x73,   0x6E,   0x3D,   0x20,    0x7,   0x1A,   0x6C,   0x71,   0x56,   0x4B,   
0x18,    0x5,   0x22,   0x3F,   0x84,   0x99,   0xBE,   0xA3,   0xF0,   0xED,   
0xCA,   0xD7,   0x35,   0x28,    0xF,   0x12,   0x41,   0x5C,   0x7B,   0x66,   
0xDD,   0xC0,   0xE7,   0xFA,   0xA9,   0xB4,   0x93,   0x8E,   0xF8,   0xE5,   
0xC2,   0xDF,   0x8C,   0x91,   0xB6,   0xAB,   0x10,    0xD,   0x2A,   0x37,   
0x64,   0x79,   0x5E,   0x43,   0xB2,   0xAF,   0x88,   0x95,   0xC6,   0xDB,   
0xFC,   0xE1,   0x5A,   0x47,   0x60,   0x7D,   0x2E,   0x33,   0x14,    0x9,   
0x7F,   0x62,   0x45,   0x58,    0xB,   0x16,   0x31,   0x2C,   0x97,   0x8A,   
0xAD,   0xB0,   0xE3,   0xFE,   0xD9,   0xC4
};


/*!
 * @brief create CRC-16 table
 *
 * creates the necessary table for being used in the 
 * efficient calculation of the 16 bit CRC.
 *
 * @note
 * this is obsolete - the CRC calculations are done with the
 * (hardcoded) CRC_Table_16
 *
 * CreationDate: 2001-02-02
 *
 * Version:      2001-2014-03-02
 *
 * @param usCRC_Table
 *   the table is returned into an array of length 256
 */
void CRC_Init_16(unsigned short usCRC_Table[])
{
  /* declare necessary variables: */
  unsigned short i, c, c8_5, c15_12, c_mb, c_lb, c7_5, c7_4, c12_9, c15_13;

  /* calculate the CRC-16 value for the input data i 
     and the previous CRC-16 equal to zero */
  for(i=0; i<256; i++)
  {
    /* shift the four most significant bits to different positions */
    c_mb = (unsigned short) ((i & 0xf0) >> 4);
    c7_4 = (unsigned short) (c_mb << 4);
    c7_5 = (unsigned short) (c7_4 << 1);
    c12_9 = (unsigned short) (c7_5 << 4);
    c15_13 = (unsigned short) (c12_9 << 3);

    /* shift the four least significant bits to different positions */
    c_lb = (unsigned short) (i & 0x0f);
    c8_5 = (unsigned short) (c_lb << 5);
    c15_12 = (unsigned short) (c8_5 << 7);

    /* calculate the CRC-16 value */
    c = (unsigned short) ((c_mb^c_lb) & 0x000f);
    c |= (c7_4^c8_5^c7_5) & 0x00f0;
    c |= (c12_9^c8_5^c7_5) & 0x0f00;
    c |= (c15_12^c15_13^c12_9) & 0xf000;

    usCRC_Table[i] = c;
  }
}


/*!
 * @brief create CRC-8 table
 *
 * creates the necessary table for being used in the 
 * efficient calculation of the 8 bit CRC
 *
 * @note
 * this is obsolete - the CRC calculations are done with the
 * (hardcoded) CRC_Table_8
 *
 * CreationDate: 2001-02-02
 *
 * Version:      2001-2014-03-02
 *
 * @param ucCRC_Table
 *   the table is returned into an array of length 256
 */
void CRC_Init_8(unsigned char ucCRC_Table[])
{
    /* declare necessary variables: */
  unsigned char c, c_mb, c7_4, c7_2, c7_3, c5_4, c_lb, c1_0a, c1_0, c2_0, 
                c2a, c2, c3;
  unsigned short i;

    /* calculate the CRC-8 value for the input data i 
       and the previous CRC-8 equal to zero */
  for(i=0; i<256; i++)
  {
      /* shift the data to different positions */
    c7_2 = (unsigned char) (i << 2);
    c7_3 = (unsigned char) (c7_2 << 1);
    c7_4 = (unsigned char) (c7_3 << 1);
    c5_4 = (unsigned char) (i >> 2);

      /* shift the four most significant bits to different positions */
    c_mb = (unsigned char) ((i & 0xf0) >> 4);
    c1_0a = (unsigned char) (c_mb & 0x03);
    c3 = (unsigned char) (c1_0a << 3);
    c2a = (unsigned char) ((c1_0a << 2) & 0x04);
    c2 = (unsigned char) ((c1_0a << 1) & 0x04);
    c2_0 = (unsigned char) (c_mb >>1);
    c1_0 = (unsigned char) (c2_0 >> 1);
   
      /* get the four least significant bits */
    c_lb = (unsigned char) (i & 0x0f);

      /* calculate the CRC-8 value */
    c = (unsigned char) ((c_lb^c1_0a^c2_0^c1_0^c7_2^c7_3^c3^c2a^c2) & 0x0f);
    c |= (c7_2^c7_3^c7_4^c5_4) & 0xf0;

    ucCRC_Table[i] = c;
  }
}


/*!
 * @brief calculate CRC-16
 *
 * calculates the 16 bit CRC value of the provided data and
 * returns it
 *
 * CreationDate: 2001-02-02
 *
 * Version:      2001-2014-03-02
 *
 * @param cDataPointer
 *   pointer to the first data byte.
 * @param iLength
 *   size of the data in bytes
 * @return
 *   CRC-16 value
 */
unsigned short CRC_Build_16(const unsigned char *cDataPointer, 
                            const unsigned long iLength)
{
  /* declare and initialize necessary variables: */
  unsigned long l;
  register unsigned short iData;
  unsigned short iCRC_16=0xffff;

  /* check for valid parameters: */
  assert(iLength > 0);

  /* calculate recursively the CRC-16, byte by byte of the data */
  for(l=0; l<iLength; l++)
  {
    /* get the next data byte */
    iData = (unsigned short) (*cDataPointer++ & 0xff);
    /* calculate the table index with the data and the most significant
       byte of the previous CRC-16 */
    iData ^= ((iCRC_16 >> 8) & 0x00ff);
    /* access to the table */
    iData = *(CRC_Table_16 + iData);
    /* calculate the new CRC-16 value with the table output and the
       least significant byte of the previous CRC-16 */
    iCRC_16 = (unsigned short) (iData ^ (iCRC_16 << 8));  
  }

  /* return the inverted CRC-16 value */
  return((unsigned short) (iCRC_16 ^ 0xffff));
}


/*!
 * @brief calculate 8 bit CRC
 *
 * calculates the 8 bit CRC value of the provided data and
 * returns it
 *
 * CreationDate: 2001-02-02
 *
 * Version:      2001-2014-03-02
 *
 * @param cDataPointer
 *   pointer to the first data byte.
 * @param iLength
 *   size of the data in bytes
 * @return unsigned char with the CRC-8 value
 */
unsigned char CRC_Build_8(const unsigned char *cDataPointer, 
                          const unsigned long iLength)
{
  /* declare and initialize necessary variables: */
  unsigned long l;
  register unsigned char cData=0;
  unsigned char iCRC_8=0xff;

  /* check for valid parameters: */
  assert(iLength > 0);

  /* calculate recursively the CRC-16, byte by byte of the data */
  for(l=0; l<iLength; l++)
  {
    /* get the next data byte */
    cData = *cDataPointer++;
    /* calculate the new CRC-8 value by accessing to the table with 
       the data and the previuos CRC-8 */
    iCRC_8 = *(CRC_Table_8 + (iCRC_8 ^ cData));
  }

  /* return the inverted CRC-8 value */
  return((unsigned char) (iCRC_8 ^ 0xff));
}
  

/*!
 * @brief check whether data matches CRC-16
 *
 * compares the 16 bit CRC value of the provided data
 * to the provided 16 bit value
 *
 * CreationDate: 2001-02-02
 *
 * Version:      2001-2014-03-02
 *
 * @param cDataPointer
 *   pointer to the first data byte.
 * @param  iLength
 *   size of the data in bytes
 * @param iCRC_16
 *   received CRC-16 value
 * @retval 1  There are no errors
 * @retval 0  The received CRC-16 is different to the CRC-16
 *            calculated in the receiver.
 */
char CRC_Check_16(const unsigned char *cDataPointer, 
                  const unsigned long iLength, 
                  const unsigned short iCRC_16)
{
  unsigned short iReceiverCRC;
  
  /* calculate the CRC-16 value with the received data */  
  iReceiverCRC = CRC_Build_16(cDataPointer,iLength);

  /* compare the two CRC-16 values */
  return((char) (iCRC_16 == iReceiverCRC));
}
 

/*!
 * @brief check whether data matches CRC-8
 *
 * compares the 8 bit CRC value of the provided data
 * to the provided 8 bit value
 *
 * CreationDate: 2001-02-02
 *
 * Version:      2001-2014-03-02
 *
 * @param cDataPointer
 *   pointer to the first data byte.
 * @param iLength
 *   size of the data in bytes
 * @param iCRC_8
 *   received CRC-8 value
 * @retval 1  There are no errors
 * @retval 0  The received CRC-8 is different to the CRC-8
 *            calculated in the receiver.
 */
char CRC_Check_8(const unsigned char *cDataPointer, 
                 const unsigned long iLength, 
                 const unsigned char iCRC_8)
{
  unsigned char iReceiverCRC;

  /* calculate the CRC-8 value with the received data */
  iReceiverCRC = CRC_Build_8(cDataPointer,iLength);

  /* compare the two CRC-8 values */   
  return ((char) (iReceiverCRC == iCRC_8));
} 

