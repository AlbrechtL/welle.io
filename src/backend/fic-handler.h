/*
 *    Copyright (C) 2019
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
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
/*
 *  FIC data
 */
#ifndef __FIC_HANDLER
#define __FIC_HANDLER

#include <mutex>
#include <cstdio>
#include <cstdint>
#include "viterbi.h"
#include "fib-processor.h"
#include "radio-controller.h"

class FicHandler: public Viterbi
{
    public:
        FicHandler(RadioControllerInterface& mr);
        void    processFicBlock(const softbit_t *data, int16_t blkno);
        void    setBitsperBlock(int16_t b);
        void    clearEnsemble();
        int     getFicDecodeRatioPercent();

        FIBProcessor fibProcessor;

    private:
        RadioControllerInterface& myRadioInterface;
        void        processFicInput(const softbit_t *ficblock, int16_t ficno);
        const int8_t *PI_15;
        const int8_t *PI_16;
        std::vector<uint8_t> bitBuffer_out;
        std::vector<softbit_t> ofdm_input;
        std::vector<softbit_t> viterbiBlock;
        int16_t     index = 0;
        int16_t     bitsperBlock = 2 * 1536;
        int16_t     ficno = 0;
        uint8_t     PRBS[768];

        // Saturating up/down-counter in range [0, 10] corresponding
        // to the number of FICs with correct CRC
        int         fic_decode_success_ratio = 0;
};

#endif


