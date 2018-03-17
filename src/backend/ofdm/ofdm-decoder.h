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
#ifndef __OFDM_DECODER
#define __OFDM_DECODER

#include    "DabConstants.h"
#include    <vector>
#include    <thread>
#include    <condition_variable>
#include    <mutex>
#include    <atomic>
#include    "fft.h"
#include    <stdint.h>
#include    "freq-interleaver.h"

class   CRadioController;
class   ficHandler;
class   mscHandler;

class   ofdmDecoder
{
    public:
        ofdmDecoder(
                CDABParams *p,
                CRadioController *mr,
                ficHandler *my_ficHandler,
                mscHandler *my_mscHandler);
        ~ofdmDecoder(void);
        void    processBlock_0      (DSPCOMPLEX *);
        void    decodeFICblock      (DSPCOMPLEX *, int32_t n);
        void    decodeMscblock      (DSPCOMPLEX *, int32_t n);
        int16_t get_snr         (DSPCOMPLEX *);
        void    stop            (void);
    private:
        CDABParams  *params;
        CRadioController    *myRadioInterface;
        ficHandler  *my_ficHandler;
        mscHandler  *my_mscHandler;
        std::atomic<bool>   running;

        std::condition_variable commandHandler;
        std::mutex  myMutex;
        int16_t     amount;
        DSPCOMPLEX  **command;

        std::thread myThread;
        void        workerthread(void);
        void        processBlock_0(void);
        void        decodeFICblock(int32_t n);
        void        decodeMscblock(int32_t n);

        int32_t     T_s;
        int32_t     T_u;
        int32_t     T_g;
        int32_t     carriers;
        std::vector<DSPCOMPLEX> phaseReference;
        common_fft  fft_handler;
        DSPCOMPLEX  *fft_buffer;
        interLeaver myMapper;

        std::vector<int16_t> ibits;
        int16_t     snrCount;
        int16_t     snr;
};

#endif

