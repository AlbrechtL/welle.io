/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
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
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <cstddef>
#include "ofdm-processor.h"
#include "various/profiling.h"
#include <iostream>
//
#define SEARCH_RANGE        (2 * 36)
#define CORRELATION_LENGTH  24

/**
  * \brief OFDMProcessor
  * The OFDMProcessor class is the driver of the processing
  * of the samplestream.
  * It takes as parameter (a.o) the handler for the
  * input device as well as the interpreters for
  * FIC symbols and for MSC symbols.
  * Local is a class ofdmDecoder that will - as the name suggests -
  * map samples to bits and that will pass on the bits
  * to the interpreters for FIC and MSC
  */


OFDMProcessor::OFDMProcessor(
        InputInterface& inputInterface,
        const DABParams& params,
        RadioControllerInterface& ri,
        MscHandler& msc,
        FicHandler& fic,
        RadioReceiverOptions rro) :
    receiver_options(rro),
    radioInterface(ri),
    input(inputInterface),
    params(params),
    ficHandler(fic),
    tiiDecoder(params, ri),
    T_null(params.T_null),
    T_u(params.T_u),
    T_s(params.T_s),
    T_F(params.T_F),
    oscillatorTable(INPUT_RATE),
    phaseRef(params, rro.fftPlacementMethod),
    ofdmDecoder(params, ri, fic, msc),
    fft_handler(params.T_u),
    fft_buffer(fft_handler.getVector())
{
    /**
     * the class phaseReference will take a number of samples
     * and indicate - using some threshold - whether there is
     * a strong correlation or not.
     * It is used to decide on the first non-null sample
     * of the frame.
     * The size of the symbols handed over for inspection
     * is T_u
     */
    /**
     * the ofdmDecoder takes time domain samples, will do an FFT,
     * map the result on (soft) bits and hand over control for handling
     * the decoded symbols
     */

    for (int i = 0; i < INPUT_RATE; i ++)
        oscillatorTable[i] = DSPCOMPLEX(cos(2.0 * M_PI * i / INPUT_RATE),
                sin(2.0 * M_PI * i / INPUT_RATE));

    //  and for the correlation
    refArg.resize(CORRELATION_LENGTH);
    for (int i = 0; i < CORRELATION_LENGTH; i ++)  {
        refArg[i] = arg(phaseRef[(T_u + i) % T_u] *
                conj(phaseRef[(T_u + i + 1) % T_u]));
    }

    correlationVector.resize(SEARCH_RANGE + CORRELATION_LENGTH);
}

OFDMProcessor::~OFDMProcessor()
{
    running = false;

    if (threadHandle.joinable()) {
        threadHandle.join();
    }
}

void OFDMProcessor::restart()
{
    std::clog << "OFDM-processor:restart" << std::endl;

    running = false;
    if (threadHandle.joinable()) {
        threadHandle.join();
    }

    coarseCorrector    = 0;
    fineCorrector      = 0;
    syncBufferIndex    = 0;
    sLevel             = 0;
    localPhase         = 0;
    input.restart();
    running            = true;
    threadHandle       = std::thread(&OFDMProcessor::run, this);
}

class InputFailure { };
class NotRunningAnymore { };

/**
 * \brief getSample
 * Profiling shows that getting a sample, together
 * with the frequency shift, is a real performance killer.
 * we therefore distinguish between getting a single sample
 * and getting a vector full of samples
 */

DSPCOMPLEX OFDMProcessor::getSample(int32_t phase)
{
    DSPCOMPLEX temp;
    if (!running)
        throw NotRunningAnymore();
    /// bufferContent is an indicator for the value of ...->Samples ()
    if (bufferContent == 0) {
        bufferContent = input.getSamplesToRead ();
        while ((bufferContent == 0) && running) {
            if (not input.is_ok()) {
                throw InputFailure();
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            bufferContent = input.getSamplesToRead ();
        }
    }

    if (!running)
        throw NotRunningAnymore();
    //
    //  so here, bufferContent > 0
    input.getSamples (&temp, 1);
    bufferContent --;

    //
    //  OK, we have a sample!!
    //  first: adjust frequency. We need Hz accuracy
    localPhase  -= phase;
    localPhase  = (localPhase + INPUT_RATE) % INPUT_RATE;
    temp        *= oscillatorTable[localPhase];
    sLevel      = 0.00001 * l1_norm(temp) + (1 - 0.00001) * sLevel;
#define N   5
    sampleCnt   ++;
    if (++ sampleCnt > INPUT_RATE / N) {
        radioInterface.onFrequencyCorrectorChange(
                fineCorrector, coarseCorrector);
        sampleCnt = 0;
    }
    return temp;
}

void OFDMProcessor::getSamples(DSPCOMPLEX *v, int16_t n, int32_t phase)
{
    int32_t     i;

    if (!running)
        throw NotRunningAnymore();
    if (n > bufferContent) {
        bufferContent = input.getSamplesToRead ();
        while ((bufferContent < n) && running) {
            if (not input.is_ok()) {
                throw InputFailure();
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            bufferContent = input.getSamplesToRead();
        }
    }
    if (!running)
        throw NotRunningAnymore();
    //
    //  so here, bufferContent >= n
    n = input.getSamples (v, n);
    bufferContent -= n;

    //  OK, we have samples!!
    //  first: adjust frequency. We need Hz accuracy
    for (i = 0; i < n; i ++) {
        localPhase  -= phase;
        localPhase   = (localPhase + INPUT_RATE) % INPUT_RATE;
        v[i]    *= oscillatorTable[localPhase];
        sLevel   = 0.00001 * l1_norm(v[i]) + (1 - 0.00001) * sLevel;
    }

    sampleCnt += n;
    if (sampleCnt > INPUT_RATE / N) {
        radioInterface.onFrequencyCorrectorChange(
                fineCorrector, coarseCorrector);
        sampleCnt = 0;
    }
}


/***
 *    \brief run
 *    The main thread, reading samples,
 *    time synchronization and frequency synchronization
 *    Identifying symbols in the DAB frame
 *    and sending them to the ofdmDecoder who will transfer the results
 *    Finally, estimating the small freqency error
 */
void OFDMProcessor::run()
{
    int32_t startIndex;
    int32_t i;
    int32_t counter;
    float currentStrength;
    constexpr int32_t syncBufferSize  = 32768;
    constexpr int32_t syncBufferMask  = syncBufferSize - 1;
    float envBuffer[syncBufferSize];

    std::vector<DSPCOMPLEX> ofdmBuffer(params.L * params.T_s);
    std::vector<std::vector<DSPCOMPLEX> > allSymbols;

    try {

        //Initing:
        /// first, we need samples to get a reasonable sLevel
        sLevel   = 0;
        for (i = 0; i < T_F / 2; i ++) {
            l1_norm(getSample (0));
        }
notSynced:
        PROFILE(NotSynced);
        if (scanMode && ++attempts > 5) {
            radioInterface.onSignalPresence(false);
            scanMode  = false;
            attempts  = 0;
        }
        syncBufferIndex  = 0;
        currentStrength  = 0;

        //  read in T_s samples for a next attempt;
        syncBufferIndex = 0;
        currentStrength  = 0;
        for (i = 0; i < 50; i ++) {
            DSPCOMPLEX sample         = getSample (0);
            envBuffer [syncBufferIndex]   = l1_norm(sample);
            currentStrength           += envBuffer [syncBufferIndex];
            syncBufferIndex ++;
        }
        /**
         * We now have initial values for currentStrength (i.e. the sum
         * over the last 50 samples) and sLevel, the long term average.
         */
        //SyncOnNull:
        /**
         * here we start looking for the null level, i.e. a dip
         */
        counter  = 0;
        radioInterface.onSyncChange(false);
        while (currentStrength / 50  > 0.50 * sLevel) {
            DSPCOMPLEX sample =
                getSample (coarseCorrector + fineCorrector);
            envBuffer [syncBufferIndex] = l1_norm(sample);
            //  update the levels
            currentStrength += envBuffer [syncBufferIndex] -
                envBuffer [(syncBufferIndex - 50) & syncBufferMask];
            syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
            counter ++;
            if (counter > T_F) { // hopeless
                //           fprintf (stderr, "%f %f\n", currentStrength / 50, sLevel);
                goto notSynced;
            }
        }
        /**
         * It seemed we found a dip that started app 65/100 * 50 samples earlier.
         * We now start looking for the end of the null period.
         */
        counter  = 0;
        //SyncOnEndNull:
        PROFILE(SyncOnEndNull);
        while (currentStrength / 50 < 0.75 * sLevel) {
            DSPCOMPLEX sample = getSample (coarseCorrector + fineCorrector);
            envBuffer [syncBufferIndex] = l1_norm(sample);
            //  update the levels
            currentStrength += envBuffer [syncBufferIndex] -
                envBuffer [(syncBufferIndex - 50) & syncBufferMask];
            syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
            counter   ++;
            //
            if (counter > T_null + 50) { // hopeless
                std::clog << "ofdm-processor: " << "SyncOnEndNull failed" << std::endl;
                goto notSynced;
            }
        }
        /**
         * The end of the null period is identified, probably about 40
         * samples earlier.
         */
SyncOnPhase:
        PROFILE(SyncOnPhase);
        /**
         * We now have to find the exact first sample of the non-null period.
         * We use a correlation that will find the first sample after the
         * cyclic prefix.
         * When in "sync", i.e. pretty sure that we know were we are,
         * we skip the "dip" identification and come here right away.
         *
         * now read in Tu samples. The precise number is not really important
         * as long as we can be sure that the first sample to be identified
         * is part of the samples read.
         */
        getSamples(ofdmBuffer.data(), T_u, coarseCorrector + fineCorrector);
        //
        /// and then, call upon the phase synchronizer to verify/compute
        /// the real "first" sample
        startIndex = phaseRef.findIndex(ofdmBuffer.data(),
                impulseResponseBuffer);
        PROFILE(FindIndex);
        radioInterface.onNewImpulseResponse(std::move(impulseResponseBuffer));
        impulseResponseBuffer.clear();

        if (startIndex < 0) { // no sync, try again
            std::clog << "ofdm-processor: " << "SyncOnPhase failed" << std::endl;
            goto notSynced;
        }
        if (scanMode) {
            radioInterface.onSignalPresence(true);
            scanMode  = false;
            attempts  = 0;
        }

        /**
         * Once here, we are synchronized, we need to copy the data we
         * used for synchronization for the PRS */
        memmove(ofdmBuffer.data(), &ofdmBuffer[startIndex],
                (params.T_u - startIndex) * sizeof (DSPCOMPLEX));
        ofdmBufferIndex  = params.T_u - startIndex;

        //Symbol 0: Phase reference symbol symbol
        /**
         * Symbol 0 is special in that it is used for fine time synchronization
         * and its content is used as a reference for decoding the
         * first data symbol.
         * We read the missing samples in the ofdm buffer
         */
        radioInterface.onSyncChange(true);
        getSamples(&ofdmBuffer[ofdmBufferIndex],
                T_u - ofdmBufferIndex,
                coarseCorrector + fineCorrector);

        RadioReceiverOptions rro;
        {
            std::lock_guard<std::mutex> lock(receiver_options_mutex);
            rro = receiver_options;
        }

        std::vector<complexf> prs;
        if (rro.decodeTII) {
            prs.resize(T_u);
            std::copy(ofdmBuffer.begin(), ofdmBuffer.begin() + T_u, prs.begin());
        }

        //  Here we look only at the PRS when we need a coarse
        //  frequency synchronization.
        //  The width is limited to 2 * 35 kHz (i.e. positive and negative)
        //
        //  We inhibit touching the coarse corrector if more than 50% of
        //  FICs had correct CRC, because enabling the coarse corrector during a short
        //  reception glitch might provoke a long delay until it resyncs properly.
        //  As long as some FICs have correct CRC, we assume the coarse corrector cannot
        //  be off.
        if (!rro.disableCoarseCorrector and ficHandler.getFicDecodeRatioPercent() < 50) {
            if (!coarseSyncCounter) {
                std::clog << "ofdm-processor: " << "Lost coarse sync (coarseCorrector: " << lastValidCoarseCorrector << "; fineCorrector: " <<  lastValidFineCorrector << ")" << std::endl;
            }

            coarseSyncCounter++;
            int correction = processPRS(ofdmBuffer.data(), rro.freqsyncMethod);
            if (correction != 100) {
                coarseCorrector += correction * params.carrierDiff;
                if (abs (coarseCorrector) > kHz(35))
                    coarseCorrector = 0;
            }
        }
        else {
            if (coarseSyncCounter) {
                 std::clog << "ofdm-processor: " << "Found sync (coarseCorrector: " << lastValidCoarseCorrector << "; fineCorrector: " <<  lastValidFineCorrector << " after " << coarseSyncCounter << " frames)" << std::endl;
            }
            coarseSyncCounter = 0;

            lastValidFineCorrector = fineCorrector;
            lastValidCoarseCorrector = coarseCorrector;
        }

        allSymbols.resize(params.L);
        allSymbols[0] = move(ofdmBuffer);
        ofdmBuffer.resize(params.L * params.T_s);

        /**
         * after symbol 0, we will just read in the other (params.L - 1) symbols
         */
        //Data_symbols:
        PROFILE(DataSymbols);
        /**
         * The first ones are the FIC symbols, followed by all MSC
         * symbols.  We immediately start with building up an average of the
         * phase difference between the samples in the cyclic prefix and the
         * corresponding samples in the datapart.
         */
        DSPCOMPLEX FreqCorr = DSPCOMPLEX(0, 0);
        for (int sym = 1; sym < params.L; sym ++) {
            auto& buf = allSymbols[sym];
            buf.resize(T_s);
            getSamples(buf.data(), T_s, coarseCorrector + fineCorrector);
            for (int i = T_u; i < T_s; i ++)
                FreqCorr += buf[i] * conj(buf[i - T_u]);
        }

        PROFILE(PushAllSymbols);
        ofdmDecoder.pushAllSymbols(move(allSymbols));

        //NewOffset:
        /// we integrate the newly found frequency error with the
        /// existing frequency error.
        fineCorrector += 0.1 * arg(FreqCorr) / M_PI *
            (params.carrierDiff / 2);
        //
        /**
         * OK,  here we are at the end of the frame
         * Assume everything went well and skip T_null samples
         */
        syncBufferIndex  = 0;
        currentStrength  = 0;

        PROFILE(DecodeTII);
        // The NULL is interesting to save because it carries the TII.
        std::vector<DSPCOMPLEX> nullSymbol(T_null);
        getSamples(nullSymbol.data(), T_null, coarseCorrector + fineCorrector);
        if (rro.decodeTII) {
            tiiDecoder.pushSymbols(nullSymbol, prs);
        }

        PROFILE(OnNewNull);
        radioInterface.onNewNullSymbol(std::move(nullSymbol));

        /**
         * The first sample to be found for the next frame should be T_g
         * samples ahead
         * Here we just check the fineCorrector
         */
        counter  = 0;

        if (fineCorrector > params.carrierDiff / 2) {
            coarseCorrector += params.carrierDiff;
            fineCorrector -= params.carrierDiff;
        }
        else
            if (fineCorrector < -params.carrierDiff / 2) {
                coarseCorrector -= params.carrierDiff;
                fineCorrector += params.carrierDiff;
            }
        //ReadyForNewFrame:
        /// and off we go, up to the next frame
        PROFILE_FRAME_DECODED();
        goto SyncOnPhase;
    }
    catch (const NotRunningAnymore&) {
        std::clog << "OFDM-processor: closing down" << std::endl;
    }
    catch (const InputFailure&) {
        std::clog << "OFDM-processor: input not ok, closing down" << std::endl;
        running = false; //Needed before onInputFailure, because subsequent calls will call OFDMProcessor::stop()
        radioInterface.onInputFailure();
    }
    running = false;
}

void OFDMProcessor::stop()
{
    if (running) {
        running = false;
        if (threadHandle.joinable()) {
            threadHandle.join();
        }
    }
}

void OFDMProcessor::resetCoarseCorrector()
{
    coarseCorrector = 0;
}

void OFDMProcessor::setReceiverOptions(const RadioReceiverOptions rro)
{
    std::unique_lock<std::mutex> lock(receiver_options_mutex);
    bool need_reset = (receiver_options.disableCoarseCorrector != rro.disableCoarseCorrector);
    receiver_options = rro;
    phaseRef.selectFFTWindowPlacement(rro.fftPlacementMethod);
    lock.unlock();

    if (need_reset) {
        restart();
    }
}

void OFDMProcessor::set_scanMode(bool b)
{
    scanMode = b;
}

#define RANGE 36
int16_t OFDMProcessor::processPRS(DSPCOMPLEX *v, const FreqsyncMethod& freqsyncMethod)
{
    int16_t i, j, index = 100;

    memcpy(fft_buffer, v, T_u * sizeof(DSPCOMPLEX));
    fft_handler.do_FFT();

    switch (freqsyncMethod) {
        case FreqsyncMethod::GetMiddle:
            return getMiddle(fft_buffer);
        case FreqsyncMethod::CorrelatePRS:
        {
            //  The "best" approach for computing the coarse frequency
            //  offset is to look at the spectrum of symbol 0 and relate that
            //  with the spectrum as it should be, i.e. the refTable
            //  However, since there might be
            //  a pretty large phase offset between the incoming data and
            //  the reference table data, we correlate the
            //  phase differences between the subsequent carriers rather
            //  than the values in the segments themselves.
            //  It seems to work pretty well
            //
            //  The phase differences are computed once
            for (i = 0; i < SEARCH_RANGE + CORRELATION_LENGTH; i ++) {
                int16_t baseIndex = T_u - SEARCH_RANGE / 2 + i;
                correlationVector[i] =
                    arg(fft_buffer[baseIndex % T_u] *
                    conj(fft_buffer[(baseIndex + 1) % T_u]));
            }

            float    MMax    = 0;
            for (i = 0; i < SEARCH_RANGE; i ++) {
                float sum = 0;
                for (j = 0; j < CORRELATION_LENGTH; j ++) {
                    sum += abs(refArg [j] * correlationVector[i + j]);
                    if (sum > MMax) {
                        MMax = sum;
                        index = i;
                    }
                }
            }

            //  Now map the index back to the right carrier
            return T_u - SEARCH_RANGE / 2 + index - T_u;
        }
        case FreqsyncMethod::PatternOfZeros:
        {
            //  An alternative way is to look at a special pattern consisting
            //  of zeros in the row of args between successive carriers.
            float Mmin   = 1000;
            for (i = T_u - SEARCH_RANGE / 2; i < T_u + SEARCH_RANGE / 2; i ++) {
                float a1  =  abs (abs (arg (fft_buffer [(i + 1) % T_u] *
                                conj (fft_buffer [(i + 2) % T_u])) / M_PI) - 1);
                float a2  =  abs (abs (arg (fft_buffer [(i + 2) % T_u] *
                                conj (fft_buffer [(i + 3) % T_u])) / M_PI) - 1);
                float a3   = abs (arg (fft_buffer [(i + 3) % T_u] *
                            conj (fft_buffer [(i + 4) % T_u])));
                float a4   = abs (arg (fft_buffer [(i + 4) % T_u] *
                            conj (fft_buffer [(i + 5) % T_u])));
                float a5   = abs (arg (fft_buffer [(i + 5) % T_u] *
                            conj (fft_buffer [(i + 6) % T_u])));
                float b1   = abs (abs (arg (fft_buffer [(i + 16 + 1) % T_u] *
                                conj (fft_buffer [(i + 16 + 3) % T_u])) / M_PI) - 1);
                float b2   = abs (arg (fft_buffer [(i + 16 + 3) % T_u] *
                            conj (fft_buffer [(i + 16 + 4) % T_u])));
                float b3   = abs (arg (fft_buffer [(i + 16 + 4) % T_u] *
                            conj (fft_buffer [(i + 16 + 5) % T_u])));
                float b4   = abs (arg (fft_buffer [(i + 16 + 5) % T_u] *
                            conj (fft_buffer [(i + 16 + 6) % T_u])));
                float sum = a1 + a2 + a3 + a4 + a5 + b1 + b2 + b3 + b4;
                if (sum < Mmin) {
                    Mmin = sum;
                    index = i;
                }
            }
            return index - T_u;
        }
    }
    throw std::logic_error("Unimplemented freqsyncMethod");
}

int16_t OFDMProcessor::getMiddle (DSPCOMPLEX *v)
{
    int16_t     i;
    DSPFLOAT    sum = 0;
    int16_t     maxIndex = 0;
    DSPFLOAT    oldMax  = 0;
    //
    //  basic sum over K carriers that are - most likely -
    //  in the range
    //  The range in which the carrier should be is
    //  T_u / 2 - K / 2 .. T_u / 2 + K / 2
    //  We first determine an initial sum over params.K carriers
    for (i = 40; i < params.K + 40; i ++)
        sum += abs (v [(T_u / 2 + i) % T_u]);
    //
    //  Now a moving sum, look for a maximum within a reasonable
    //  range (around (T_u - K) / 2, the start of the useful frequencies)
    for (i = 40; i < T_u - (params.K - 40); i ++) {
        sum -= abs (v [(T_u / 2 + i) % T_u]);
        sum += abs (v [(T_u / 2 + i + params.K) % T_u]);
        if (sum > oldMax) {
            sum = oldMax;
            maxIndex = i;
        }
    }
    return maxIndex - (T_u - params.K) / 2;
}
