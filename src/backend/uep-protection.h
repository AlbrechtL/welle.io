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

#ifndef UEP_PROTECTION
#define UEP_PROTECTION

#include    <stdio.h>
#include    <stdint.h>
#include    <vector>
#include    "protection.h"
#include    "viterbi.h"

class UEPProtection: public Protection, public Viterbi
{
    public:
        UEPProtection(int16_t bitRate, int16_t protLevel);
        bool deconvolve(const softbit_t *v, int32_t size, uint8_t *outBuffer);
    private:
        int16_t L1;
        int16_t L2;
        int16_t L3;
        int16_t L4;
        const int8_t *PI1;
        const int8_t *PI2;
        const int8_t *PI3;
        const int8_t *PI4;
        int32_t outSize;
        std::vector<softbit_t> viterbiBlock;
};

#endif

