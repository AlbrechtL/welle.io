#
/*
 * $Id: pa_ringbuffer.c 1738 2011-08-18 11:47:28Z rossb $
 * Portable Audio I/O Library
 * Ring Buffer utility.
 *
 * Author: Phil Burk, http://www.softsynth.com
 * modified for SMP safety on Mac OS X by Bjorn Roche
 * modified for SMP safety on Linux by Leland Lucius
 * also, allowed for const where possible
 * modified for multiple-byte-sized data elements by Sven Fischer 
 *
 * Note that this is safe only for a single-thread reader and a
 * single-thread writer.
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 *
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    The ringbuffer here is a rewrite of the ringbuffer used in the PA code
 *    All rights remain with their owners
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
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
 *    along with ESDR; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __RINGBUFFER
#define	__RINGBUFFER
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdint.h>
/*
 *	a simple ringbuffer, lockfree, however only for a
 *	single reader and a single writer.
 *	Mostly used for getting samples from or to the soundcard
 */
#if defined(__APPLE__)
#   include <libkern/OSAtomic.h>
    /* Here are the memory barrier functions. Mac OS X only provides
       full memory barriers, so the three types of barriers are the same,
       however, these barriers are superior to compiler-based ones. */
#   define PaUtil_FullMemoryBarrier()  OSMemoryBarrier()
#   define PaUtil_ReadMemoryBarrier()  OSMemoryBarrier()
#   define PaUtil_WriteMemoryBarrier() OSMemoryBarrier()
#elif defined(__GNUC__)
    /* GCC >= 4.1 has built-in intrinsics. We'll use those */
#   if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
# define PaUtil_FullMemoryBarrier()  __sync_synchronize()
# define PaUtil_ReadMemoryBarrier()  __sync_synchronize()
# define PaUtil_WriteMemoryBarrier() __sync_synchronize()
    /* as a fallback, GCC understands volatile asm and "memory" to mean it
     * should not reorder memory read/writes */
#   elif defined( __PPC__ )
#      define PaUtil_FullMemoryBarrier()  asm volatile("sync":::"memory")
#      define PaUtil_ReadMemoryBarrier()  asm volatile("sync":::"memory")
#      define PaUtil_WriteMemoryBarrier() asm volatile("sync":::"memory")
#   elif defined( __i386__ ) || defined( __i486__ ) || defined( __i586__ ) || defined( __i686__ ) || defined( __x86_64__ )
#      define PaUtil_FullMemoryBarrier()  asm volatile("mfence":::"memory")
#      define PaUtil_ReadMemoryBarrier()  asm volatile("lfence":::"memory")
#      define PaUtil_WriteMemoryBarrier() asm volatile("sfence":::"memory")
#   else
#      ifdef ALLOW_SMP_DANGERS
#         warning Memory barriers not defined on this system or system unknown
#         warning For SMP safety, you should fix this.
#         define PaUtil_FullMemoryBarrier()
#         define PaUtil_ReadMemoryBarrier()
#         define PaUtil_WriteMemoryBarrier()
#      else
#         error Memory barriers are not defined on this system. You can still compile by defining ALLOW_SMP_DANGERS, but SMP safety will not be guaranteed.
#      endif
#   endif
#else
#   ifdef ALLOW_SMP_DANGERS
#      warning Memory barriers not defined on this system or system unknown
#      warning For SMP safety, you should fix this.
#      define PaUtil_FullMemoryBarrier()
#      define PaUtil_ReadMemoryBarrier()
#      define PaUtil_WriteMemoryBarrier()
#   else
#      error Memory barriers are not defined on this system. You can still compile by defining ALLOW_SMP_DANGERS, but SMP safety will not be guaranteed.
#   endif
#endif

template <class elementtype>
class RingBuffer {
private:
		uint32_t	bufferSize;
volatile	uint32_t	writeIndex;
volatile	uint32_t	readIndex;
		uint32_t	bigMask;
	        uint32_t	smallMask;
		char		*buffer;
public:
	RingBuffer (uint32_t elementCount) {
	if (((elementCount - 1) & elementCount) != 0)
	    elementCount = 2 * 16384;	/* default	*/

	bufferSize	= elementCount;
	buffer		= new char [2 * bufferSize * sizeof (elementtype)];
	writeIndex	= 0;
	readIndex	= 0;
	smallMask	= (elementCount)- 1;
	bigMask		= (elementCount * 2) - 1;
}

	~RingBuffer () {
	   delete[]	 buffer;
}

/*
 * 	functions for checking available data for reading and space
 * 	for writing
 */
int32_t	GetRingBufferReadAvailable (void) {
	return (writeIndex - readIndex) & bigMask;
}

int32_t	ReadSpace	(void){
	return GetRingBufferReadAvailable ();
}

int32_t	GetRingBufferWriteAvailable (void) {
	return  bufferSize - GetRingBufferReadAvailable ();
}

int32_t	WriteSpace	(void) {
	return GetRingBufferWriteAvailable ();
}

void	FlushRingBuffer () {
	writeIndex	= 0;
	readIndex	= 0;
}
/* ensure that previous writes are seen before we update the write index 
   (write after write)
 */
int32_t AdvanceRingBufferWriteIndex (int32_t elementCount) {
	PaUtil_WriteMemoryBarrier();
	return writeIndex = (writeIndex + elementCount) & bigMask;
}

/* ensure that previous reads (copies out of the ring buffer) are
 * always completed before updating (writing) the read index. 
 * (write-after-read) => full barrier
 */
int32_t AdvanceRingBufferReadIndex (int32_t elementCount) {
    PaUtil_FullMemoryBarrier();
    return readIndex = (readIndex + elementCount) & bigMask;
}

/***************************************************************************
** Get address of region(s) to which we can write data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be written or elementCount, whichever is smaller.
*/
int32_t GetRingBufferWriteRegions (uint32_t elementCount,
                                   void **dataPtr1, int32_t *sizePtr1,
                                   void **dataPtr2, int32_t *sizePtr2 ) {
uint32_t   index;
uint32_t   available = GetRingBufferWriteAvailable ();

	if (elementCount > available)
	   elementCount = available;

/* Check to see if write is not contiguous. */
	index = writeIndex & smallMask;
	if ((index + elementCount) > bufferSize ) {
        /* Write data in two blocks that wrap the buffer. */
           int32_t   firstHalf = bufferSize - index;
           *dataPtr1	= &buffer[index * sizeof(elementtype)];
	   *sizePtr1	= firstHalf;
	   *dataPtr2	= &buffer [0];
	   *sizePtr2	= elementCount - firstHalf;
	}
	else {		// fits
	   *dataPtr1	= &buffer [index * sizeof(elementtype)];
	   *sizePtr1	= elementCount;
	   *dataPtr2	= NULL;
	   *sizePtr2	= 0;
	}

	if (available > 0)
           PaUtil_FullMemoryBarrier(); /* (write-after-read) => full barrier */

	return elementCount;
}

/***************************************************************************
** Get address of region(s) from which we can read data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be read or elementCount, whichever is smaller.
*/
int32_t GetRingBufferReadRegions (uint32_t elementCount,
	                          void **dataPtr1, int32_t *sizePtr1,
	                          void **dataPtr2, int32_t *sizePtr2) {
uint32_t   index;
uint32_t   available = GetRingBufferReadAvailable (); /* doesn't use memory barrier */

	if (elementCount > available)
	   elementCount = available;

/* Check to see if read is not contiguous. */
	index = readIndex & smallMask;
	if ((index + elementCount) > bufferSize) {
        /* Write data in two blocks that wrap the buffer. */
           int32_t firstHalf = bufferSize - index;
	   *dataPtr1 = &buffer [index * sizeof(elementtype)];
	   *sizePtr1 = firstHalf;
	   *dataPtr2 = &buffer [0];
	   *sizePtr2 = elementCount - firstHalf;
	}
	else {
	   *dataPtr1 = &buffer [index * sizeof(elementtype)];
	   *sizePtr1 = elementCount;
	   *dataPtr2 = NULL;
	   *sizePtr2 = 0;
	}
    
	if (available)
           PaUtil_ReadMemoryBarrier(); /* (read-after-read) => read barrier */

	return elementCount;
}

int32_t	putDataIntoBuffer (const void *data, int32_t elementCount) {
int32_t size1, size2, numWritten;
void	*data1;
void	*data2;

	numWritten = GetRingBufferWriteRegions (elementCount,
	                                        &data1, &size1,
	                                        &data2, &size2 );
	if (size2 > 0) {
           memcpy (data1, data, size1 * sizeof(elementtype));
	   data = ((char *)data) + size1 * sizeof(elementtype);
	   memcpy (data2, data, size2 * sizeof(elementtype));
	}
	else 
	   memcpy (data1, data, size1 * sizeof(elementtype));

	AdvanceRingBufferWriteIndex (numWritten );
	return numWritten;
}

int32_t getDataFromBuffer (void *data, int32_t elementCount ) {
int32_t	size1, size2, numRead;
void	*data1;
void	*data2;

	numRead = GetRingBufferReadRegions (elementCount,
	                                    &data1, &size1,
	                                    &data2, &size2 );
	if (size2 > 0) {
	   memcpy (data, data1, size1 * sizeof(elementtype));
	   data = ((char *)data) + size1 *  sizeof(elementtype);
	   memcpy (data, data2, size2 * sizeof(elementtype));
	}
	else
           memcpy (data, data1, size1 * sizeof(elementtype));

	AdvanceRingBufferReadIndex (numRead );
	return numRead;
}

int32_t	skipDataInBuffer (uint32_t n_values) {
//	ensure that we have the correct read and write indices
	PaUtil_FullMemoryBarrier ();
	if (n_values > GetRingBufferReadAvailable ())
	   n_values = GetRingBufferReadAvailable ();
	AdvanceRingBufferReadIndex (n_values);
	return n_values;
}

};
#endif

