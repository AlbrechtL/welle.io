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
#include "freq-interleaver.h"
#include "radio-controller.h"
#include "fic-handler.h"
#include "msc-handler.h"

class OfdmDecoder
{
    public:
        OfdmDecoder(
                const DABParams& p,
                RadioControllerInterface& mr,
                FicHandler& ficHandler,
                MscHandler& mscHandler);
        ~OfdmDecoder();
        void    pushAllSymbols(std::vector<std::vector<DSPCOMPLEX> >&& sym);
        void    reset();
    private:
        int16_t get_snr(DSPCOMPLEX *);

        const DABParams& params;
        RadioControllerInterface& radioInterface;
        FicHandler& ficHandler;
        MscHandler& mscHandler;
        std::atomic<bool> running = ATOMIC_VAR_INIT(false);

        std::condition_variable pending_symbols_cv;
        std::mutex mutex;
        int num_pending_symbols = 0;
        std::vector<std::vector<DSPCOMPLEX> > pending_symbols;

        std::thread thread;
        void workerthread(void);
        void processPRS();
        void decodeDataSymbol(int32_t n);

        int32_t T_g;
        std::vector<DSPCOMPLEX> phaseReference;
        fft::Forward fft_handler;
        DSPCOMPLEX   *fft_buffer;
        FrequencyInterleaver interleaver;

        std::vector<softbit_t> ibits;
        int16_t snrCount = 0;
        int16_t snr = 0;

        const double mer_alpha = 1e-7;
        std::atomic<double> mer = ATOMIC_VAR_INIT(0.0);

    public:
        // Plotting all points is too costly, we decimate the number of points.
        // The decimation factor should divide K for all transmission modes.
        static const size_t constellationDecimation = 96;
    private:
        std::vector<DSPCOMPLEX> constellationPoints;
};

#endif

