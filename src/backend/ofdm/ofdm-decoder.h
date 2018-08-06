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
 */
#ifndef __OFDM_DECODER
#define __OFDM_DECODER

#include <cstddef>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <cstdint>
#include "fft.h"
#include "dab-constants.h"
#include "ofdm/freq-interleaver.h"
#include "radio-controller.h"
#include "fic-handler.h"
#include "msc-handler.h"

class   OfdmDecoder
{
    public:
        OfdmDecoder(
                const DABParams& p,
                RadioControllerInterface& mr,
                FicHandler& ficHandler,
                MscHandler& mscHandler);
        ~OfdmDecoder(void);
        void    processPRS(DSPCOMPLEX *);
        void    decodeFICblock(DSPCOMPLEX *, int32_t n);
        void    decodeMscblock(DSPCOMPLEX *, int32_t n);
        int16_t get_snr         (DSPCOMPLEX *);
        void    stop            (void);
    private:
        const DABParams& params;
        RadioControllerInterface& radioInterface;
        FicHandler& ficHandler;
        MscHandler& mscHandler;
        std::atomic<bool> running;

        std::condition_variable commandHandler;
        std::mutex mutex;
        int16_t amount;
        DSPCOMPLEX **command;

        std::thread thread;
        void workerthread(void);
        void processPRS(void);
        void decodeFICblock(int32_t n);
        void decodeMscblock(int32_t n);

        int32_t T_g;
        std::vector<DSPCOMPLEX> phaseReference;
        fft::Forward  fft_handler;
        DSPCOMPLEX  *fft_buffer;
        FrequencyInterleaver interleaver;

        std::vector<int16_t> ibits;
        int16_t     snrCount;
        int16_t     snr;

    public:
        // Plotting all points is too costly, we decimate the number of points.
        // The decimation factor should divide K for all transmission modes.
        static const size_t constellationDecimation = 96;
    private:
        std::vector<DSPCOMPLEX> constellationPoints;
};

#endif

