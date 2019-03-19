/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2013 2015
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
 *  Once the bits are "in", interpretation and manipulation
 *  should reconstruct the data symbols.
 *  Ofdm_decoder is called once every Ts samples, and
 *  its invocation results in 2 * Tu bits
 */

#include <cstddef>
#include "ofdm-decoder.h"
#include "various/profiling.h"
#include <iostream>

/**
 * \brief OfdmDecoder
 * The class OfdmDecoder is - when implemented in a separate thread -
 * taking the data from the ofdmProcessor class in, and
 * will extract the Tu samples, do an FFT and extract the
 * carriers and map them on (soft) bits
 */
OfdmDecoder::OfdmDecoder(
        const DABParams& p,
        RadioControllerInterface& mr,
        FicHandler& ficHandler,
        MscHandler& mscHandler) :
    params(p),
    radioInterface(mr),
    ficHandler(ficHandler),
    mscHandler(mscHandler),
    pending_symbols(params.L),
    phaseReference(params.T_u),
    fft_handler(p.T_u),
    interleaver(p),
    ibits(2 * params.K)
{
    T_g = params.T_s - params.T_u;
    fft_buffer = fft_handler.getVector();

    /**
     * When implemented in a thread, the thread controls the
     * reading in of the data and processing the data through
     * functions for handling symbol 0, FIC symbols and MSC symbols.
     */
    thread = std::thread(&OfdmDecoder::workerthread, this);
}

OfdmDecoder::~OfdmDecoder()
{
    running = false;
    pending_symbols_cv.notify_all();
    if (thread.joinable()) {
        thread.join();
    }
}

void OfdmDecoder::reset()
{
    running = false;
    pending_symbols_cv.notify_all();
    if (thread.joinable()) {
        thread.join();
    }

    thread = std::thread(&OfdmDecoder::workerthread, this);
}

/**
 * The code in the thread executes a simple loop,
 * waiting for the next symbols and executing the interpretation
 * operation for that symbols.
 */
void OfdmDecoder::workerthread()
{
    int currentSym = 0;

    running = true;

    while (running) {
        std::unique_lock<std::mutex> lock(mutex);
        pending_symbols_cv.wait_for(lock, std::chrono::milliseconds(100));

        if (currentSym == 0) {
            constellationPoints.clear();
            constellationPoints.reserve(
                    (params.L-1) * params.K / constellationDecimation);
        }

        while (num_pending_symbols > 0 && running) {

            if (currentSym == 0)
                processPRS();
            else
                decodeDataSymbol(currentSym);

            currentSym = (currentSym + 1) % (params.L);
            num_pending_symbols -= 1;

            if (currentSym == 0) {
                radioInterface.onConstellationPoints(
                        std::move(constellationPoints));
                constellationPoints.clear();
                constellationPoints.reserve(
                        (params.L-1) * params.K / constellationDecimation);
            }
        }
    }

    std::clog << "OFDM-decoder:" <<  "closing down now" << std::endl;
}

void OfdmDecoder::pushAllSymbols(std::vector<std::vector<DSPCOMPLEX> >&& syms)
{
    std::unique_lock<std::mutex> lock(mutex);

    pending_symbols = std::move(syms);
    num_pending_symbols = pending_symbols.size();
    pending_symbols_cv.notify_one();
}

/**
 * handle symbol 0 as collected from the buffer
 */
void OfdmDecoder::processPRS()
{
    PROFILE(ProcessPRS);
    memcpy (fft_buffer,
            pending_symbols[0].data(),
            params.T_u * sizeof(DSPCOMPLEX));
    fft_handler.do_FFT ();
    /**
     * The SNR is determined by looking at a segment of bins
     * within the signal region and bits outside.
     * It is just an indication
     */
    snr = 0.7 * snr + 0.3 * get_snr(fft_buffer);
    if (++snrCount > 10) {
        radioInterface.onSNR(snr);
        snrCount = 0;
    }
    /**
     * we are now in the frequency domain, and we keep the carriers
     * as coming from the FFT as phase reference.
     */
    memcpy(phaseReference.data(), fft_buffer, params.T_u * sizeof (DSPCOMPLEX));
}

/**
 * For the other symbols, the first step is to go from
 * time to frequency domain, to get the carriers.
 *
 * \brief decodeDataSymbol
 * do the transforms and hand over the result to the fichandler or mschandler
 */
void OfdmDecoder::decodeDataSymbol(int32_t sym_ix)
{
    PROFILE(ProcessSymbol);
    memcpy (fft_buffer,
            pending_symbols[sym_ix].data() + T_g,
            params.T_u * sizeof (DSPCOMPLEX));
    //fftlabel:
    /**
     * first step: do the FFT
     */
    fft_handler.do_FFT();

    /**
     * a little optimization: we do not interchange the
     * positive/negative frequencies to their right positions.
     * The de-interleaving understands this
     */

    PROFILE(Deinterleaver);
    /**
     * Note that from here on, we are only interested in the
     * K useful carriers of the FFT output
     */
    for (int16_t i = 0; i < params.K; i ++) {
        int16_t index = interleaver.mapIn(i);
        if (index < 0)
            index += params.T_u;
        /**
         * decoding is computing the phase difference between
         * carriers with the same index in subsequent symbols.
         * The carrier of a symbols is the reference for the carrier
         * on the same position in the next symbols
         */
        const DSPCOMPLEX r1 = fft_buffer[index] * conj (phaseReference[index]);
        phaseReference[index] = fft_buffer[index];
        const DSPFLOAT ab1 = 127.0f / l1_norm(r1);
        /// split the real and the imaginary part and scale it

        ibits[i]            = -real (r1) * ab1;
        ibits[params.K + i] = -imag (r1) * ab1;

        if (i % constellationDecimation == 0) {
            constellationPoints.push_back(r1);
        }
    }

    if (sym_ix < 4) {
        PROFILE(FICHandler);
        ficHandler.processFicBlock(ibits.data(), sym_ix);
    }
    else {
        PROFILE(MSCHandler);
        mscHandler.processMscBlock(ibits.data(), sym_ix);
    }
    PROFILE(SymbolProcessed);
}

/**
 * for the snr we have a full T_u wide vector, with in the middle
 * K carriers.
 * Just get the strength from the selected carriers compared
 * to the strength of the carriers outside that region
 */
int16_t OfdmDecoder::get_snr(DSPCOMPLEX *v)
{
    int16_t i;
    DSPFLOAT    noise   = 0;
    DSPFLOAT    signal  = 0;
    const auto T_u = params.T_u;
    int16_t low = T_u / 2 -  params.K / 2;
    int16_t high    = low + params.K;

    for (i = 10; i < low - 20; i ++)
        noise += abs (v[(T_u / 2 + i) % T_u]);

    for (i = high + 20; i < T_u - 10; i ++)
        noise += abs (v[(T_u / 2 + i) % T_u]);

    noise   /= (low - 30 + T_u - high - 30);
    for (i = T_u / 2 - params.K / 4;  i < T_u / 2 + params.K / 4; i ++)
        signal += abs (v[(T_u / 2 + i) % T_u]);

    return get_db_over_256(signal / (params.K / 2)) - get_db_over_256(noise);
}

