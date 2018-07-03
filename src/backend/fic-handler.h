/*
 *    Copyright (C) 2018
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
        void    process_ficBlock    (int16_t *data, int16_t blkno);
        void    setBitsperBlock     (int16_t b);
        void    clearEnsemble       (void);
        bool    syncReached         (void);
        int16_t get_ficRatio        (void);

        FIBProcessor fibProcessor;

    private:
        RadioControllerInterface& myRadioInterface;
        void        process_ficInput(int16_t *ficblock, int16_t ficno);
        const int8_t *PI_15;
        const int8_t *PI_16;
        std::vector<uint8_t> bitBuffer_out;
        std::vector<int16_t> ofdm_input;
        int16_t     index = 0;
        int16_t     BitsperBlock = 2 * 1536;
        int16_t     ficno = 0;
        int16_t     ficRatio = 0;
        uint8_t     PRBS[768];
        uint8_t     shiftRegister[9];
};

#endif


