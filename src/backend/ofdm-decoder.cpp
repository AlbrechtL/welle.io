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
    stopWorker();
}

void OfdmDecoder::stopWorker()
{
    // Stop between symbols, even while the worker owns the frame mutex.
    running = false;
    {
        // Pair the notification with the wait predicates. Without this barrier
        // a waiter could check the flag, miss notify_all(), then sleep forever.
        std::lock_guard<std::mutex> lock(mutex);
        ++generation;
    }
    pending_symbols_cv.notify_all();
    if (thread.joinable()) {
        thread.join();
    }
}

void OfdmDecoder::reset()
{
    // Lifecycle calls are serialized by the owner. An old blocked submission
    // must not cross into this new decoder generation.
    stopWorker();
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (num_pending_symbols > 0) {
            ++cancelled_frames;
        }
        num_pending_symbols = 0;
        pending_symbols.clear();
        constellationPoints.clear();
        running = true;
    }
    thread = std::thread(&OfdmDecoder::workerthread, this);
}

void OfdmDecoder::workerthread()
{
    while (running) {
        std::unique_lock<std::mutex> lock(mutex);
        // A notification is only a hint. Work may already have arrived before
        // this thread started waiting, or the wakeup may be spurious.
        pending_symbols_cv.wait(lock, [this] {
            return !running || num_pending_symbols > 0;
        });
        if (!running) {
            break;
        }

        constellationPoints.clear();
        constellationPoints.reserve(
                (params.L-1) * params.K / constellationDecimation);
        int currentSym = 0;
        while (num_pending_symbols > 0 && running) {
            if (currentSym == 0)
                processPRS();
            else
                decodeDataSymbol(currentSym);
            ++currentSym;
            --num_pending_symbols;
        }
        if (num_pending_symbols == 0) {
            radioInterface.onConstellationPoints(std::move(constellationPoints));
            ++processed_frames;
            // Roughly once per minute in DAB mode I. These counters describe
            // this handoff only, not RF quality or USB sample continuity.
            if (processed_frames % 600 == 0) {
                std::clog << "OFDM_HANDOFF submitted=" << submitted_frames
                          << " processed=" << processed_frames
                          << " producer_waits=" << producer_waits
                          << " cancelled=" << cancelled_frames << std::endl;
            }
        }
        // Keep frame storage protected throughout decoding; release the single
        // handoff slot only after its last symbol has been consumed.
        lock.unlock();
        pending_symbols_cv.notify_all();
    }
    std::clog << "OFDM-decoder:closing down now" << std::endl;
}

void OfdmDecoder::pushAllSymbols(std::vector<std::vector<DSPCOMPLEX> >&& syms)
{
    std::unique_lock<std::mutex> lock(mutex);
    const auto submission_generation = generation;
    if (running && num_pending_symbols > 0) {
        ++producer_waits;
    }
    // Never replace a frame the consumer has not read. Waiting also releases
    // the mutex, so the consumer can drain the slot even under scheduling delay.
    pending_symbols_cv.wait(lock, [this, submission_generation] {
        return !running || generation != submission_generation ||
               num_pending_symbols == 0;
    });
    if (!running || generation != submission_generation) {
        return;
    }
    pending_symbols = std::move(syms);
    num_pending_symbols = pending_symbols.size();
    ++submitted_frames;
    lock.unlock();
    pending_symbols_cv.notify_all();
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
    snr = 0.7 * snr + 0.3 * get_snr(fft_buffer, 1);
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
 * method:  0 Jans method. This method are originally developed by Jan and is not working if neighbor channels are used because it uses occupied bins for the noise calculation
 *          1 New method. This method is working also if neighbor channels are used
 */
int16_t OfdmDecoder::get_snr(DSPCOMPLEX *v, uint8_t method)
{
    int16_t i;
    DSPFLOAT    noise   = 0;
    DSPFLOAT    signal  = 0;
    const auto T_u = params.T_u;
    const auto K = params.K;
    int16_t low = T_u / 2 -  K / 2;
    int16_t high    = low + K;

    if(method)
    {
        for (i = 70; i < low - 20; i ++) // low - 90 samples
            noise += abs (v[(T_u / 2 + i) % T_u]);

        for (i = high + 20; i < high + 120; i ++) // 100 samples
            noise += abs (v[(T_u / 2 + i) % T_u]);

        noise   /= (low - 90 + 100);
        for (i = T_u / 2 - K / 4;  i < T_u / 2 + K / 4; i ++)
            signal += abs (v[(T_u / 2 + i) % T_u]);

        const auto dB_signal_new = get_db_over_256(signal / (K / 2));
        const auto dB_noise_new = get_db_over_256(noise);
        const auto snr_new = dB_signal_new - dB_noise_new;
        return  snr_new;
    }
    else
    {
        noise   = 0;
        signal  = 0;
        for (i = 10; i < low - 20; i ++)
            noise += abs (v[(T_u / 2 + i) % T_u]);

        for (i = high + 20; i < T_u - 10; i ++)
            noise += abs (v[(T_u / 2 + i) % T_u]);

        noise   /= (low - 30 + T_u - high - 30);
        for (i = T_u / 2 - K / 4;  i < T_u / 2 + K / 4; i ++)
            signal += abs (v[(T_u / 2 + i) % T_u]);

        const auto dB_signal_old = get_db_over_256(signal / (K / 2));
        const auto dB_noise_old = get_db_over_256(noise);
        const auto snr_old = dB_signal_old - dB_noise_old;
        return  snr_old;
    }
}
