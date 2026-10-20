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
#ifndef DAB_PROCESSOR
#define DAB_PROCESSOR

#include    <stdint.h>
#include    <stddef.h>
#include    <stdio.h>

// Decodes one of the last logical frames of a subchannel again, with
// known bytes (iterative DAB+ decoding: the bytes of the correct
// Reed-Solomon codewords help the Viterbi decoder with the others)
class FrameRedecoder {
    public:
        virtual ~FrameRedecoder() = default;
        // age: 0 = the last frame given to addtoFrame(), 1 = the one before.
        // known: per byte of the frame 1 if bytes[i] is known.
        // bytes: in: the known bytes, out: the frame decoded again.
        // false if the frame is not available any more.
        virtual bool redecodeFrame(int age, const uint8_t *known, uint8_t *bytes) = 0;
};

//  virtual class, just for providing a common base
//  for the real decoder classes
class DabProcessor {
    public:
        virtual ~DabProcessor() = default;
        virtual void addtoFrame(uint8_t *) = 0;
        // For decoders that can use it (DAB+)
        virtual void setFrameRedecoder(FrameRedecoder *) {}
};

#endif

