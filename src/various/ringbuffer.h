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
 *    Copyright (C) 2018
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    The ringbuffer here is a rewrite of the ringbuffer used in the PA code
 *    All rights remain with their owners
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with ESDR; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include    <stdlib.h>
#include    <atomic>
#include    <vector>
#include    <stdio.h>
#include    <string.h>
#include    <stdint.h>
#include    <iostream>

/*
 *  a simple ringbuffer, lockfree, however only for a
 *  single reader and a single writer.
 *  Mostly used for getting samples from or to the soundcard
 */
// Base implementation
template <class elementtype>
class RingBuffer
{
    private:
        uint32_t    bufferSize;
        /* All synchronization between the producer and the consumer thread
           happens through these indices: the data writes/reads are ordered
           by acquire loads and release stores of the index owned by the
           other side. */
        std::atomic<uint32_t>   writeIndex;
        std::atomic<uint32_t>   readIndex;
        uint32_t    bigMask;
        uint32_t    smallMask;
        std::vector<char> buffer;

    protected:
        void onDroppedData(int32_t droppedElements) {
            (void) droppedElements;
            // In case a warning should be output, do it here
        }

    public:
        RingBuffer(uint32_t elementCount) {
            if (((elementCount - 1) & elementCount) != 0)
                elementCount = 2 * 16384;   /* default  */

            bufferSize  = elementCount;
            buffer.resize(2 * bufferSize * sizeof (elementtype));
            writeIndex  = 0;
            readIndex   = 0;
            smallMask   = (elementCount)- 1;
            bigMask     = (elementCount * 2) - 1;
        }

        /*
         *  functions for checking available data for reading and space
         *  for writing
         */
        int32_t GetBufferSize(void) {
            return bufferSize;
        }

        int32_t GetRingBufferReadAvailable (void) {
            return (writeIndex.load(std::memory_order_acquire) -
                    readIndex.load(std::memory_order_acquire)) & bigMask;
        }

        int32_t ReadSpace   (void){
            return GetRingBufferReadAvailable ();
        }

        int32_t GetRingBufferWriteAvailable (void) {
            return  bufferSize - GetRingBufferReadAvailable ();
        }

        int32_t WriteSpace  (void) {
            return GetRingBufferWriteAvailable ();
        }

        void    FlushRingBuffer () {
            writeIndex.store(0, std::memory_order_release);
            readIndex.store(0, std::memory_order_release);
        }

        /* Producer only: the release store publishes the data written into
           the buffer before the new write index becomes visible. */
        int32_t AdvanceRingBufferWriteIndex (int32_t elementCount) {
            uint32_t index = (writeIndex.load(std::memory_order_relaxed) + elementCount) & bigMask;
            writeIndex.store(index, std::memory_order_release);
            return index;
        }

        /* Consumer only: the release store orders the copies out of the
           buffer before the space is handed back to the producer. */
        int32_t AdvanceRingBufferReadIndex (int32_t elementCount) {
            uint32_t index = (readIndex.load(std::memory_order_relaxed) + elementCount) & bigMask;
            readIndex.store(index, std::memory_order_release);
            return index;
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
            /* The acquire load of readIndex (inside the call below) pairs
               with the consumer's release store, so the space returned here
               is safe to overwrite. */
            uint32_t   available = GetRingBufferWriteAvailable ();

            if (elementCount > available)
                elementCount = available;

            /* Check to see if write is not contiguous. */
            index = writeIndex.load(std::memory_order_relaxed) & smallMask;
            if ((index + elementCount) > bufferSize ) {
                /* Write data in two blocks that wrap the buffer. */
                int32_t   firstHalf = bufferSize - index;
                *dataPtr1    = &buffer[index * sizeof(elementtype)];
                *sizePtr1    = firstHalf;
                *dataPtr2    = &buffer[0];
                *sizePtr2    = elementCount - firstHalf;
            }
            else {      // fits
                *dataPtr1    = &buffer[index * sizeof(elementtype)];
                *sizePtr1    = elementCount;
                *dataPtr2    = NULL;
                *sizePtr2    = 0;
            }

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
            /* The acquire load of writeIndex (inside the call below) pairs
               with the producer's release store, so the data returned here
               is safe to read. */
            uint32_t   available = GetRingBufferReadAvailable ();

            if (elementCount > available)
                elementCount = available;

            /* Check to see if read is not contiguous. */
            index = readIndex.load(std::memory_order_relaxed) & smallMask;
            if ((index + elementCount) > bufferSize) {
                /* Write data in two blocks that wrap the buffer. */
                int32_t firstHalf = bufferSize - index;
                *dataPtr1 = &buffer[index * sizeof(elementtype)];
                *sizePtr1 = firstHalf;
                *dataPtr2 = &buffer[0];
                *sizePtr2 = elementCount - firstHalf;
            }
            else {
                *dataPtr1 = &buffer[index * sizeof(elementtype)];
                *sizePtr1 = elementCount;
                *dataPtr2 = NULL;
                *sizePtr2 = 0;
            }

            return elementCount;
        }

        int32_t putDataIntoBuffer (const void *data, int32_t elementCount) {
            int32_t size1, size2, numWritten;
            void    *data1;
            void    *data2;

            int32_t freeSpace = GetRingBufferWriteAvailable();
            int32_t droppedElements = elementCount - freeSpace;
            if(droppedElements > 0)
                onDroppedData(droppedElements);

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
            int32_t size1, size2, numRead;
            void    *data1;
            void    *data2;

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

        int32_t skipDataInBuffer (int32_t n_values) {
            if (n_values > GetRingBufferReadAvailable ())
                n_values = GetRingBufferReadAvailable ();
            AdvanceRingBufferReadIndex (n_values);
            return n_values;
        }

};

#endif // RING_BUFFER_H
