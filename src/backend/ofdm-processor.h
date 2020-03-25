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

#ifndef __OFDM_PROCESSOR__
#define __OFDM_PROCESSOR__

#include "dab-constants.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include "phasereference.h"
#include "ofdm-decoder.h"
#include "tii-decoder.h"
#include "virtual_input.h"
#include "fft.h"
#include "radio-controller.h"
#include "radio-receiver-options.h"
#include "fic-handler.h"
#include "msc-handler.h"

class OFDMProcessor
{
// Identifier "interface" is already defined in the w32api header basetype.h
    public:
        OFDMProcessor(InputInterface& inputInterface,
                const DABParams& params,
                RadioControllerInterface& ri,
                MscHandler& msc,
                FicHandler& fic,
                RadioReceiverOptions rro);
        ~OFDMProcessor();

        /* Start or restart the OFDMProcessor */
        void restart();

        void stop();
        void resetCoarseCorrector();
        void setReceiverOptions(const RadioReceiverOptions rro);
        void set_scanMode(bool);

    private:
        std::mutex receiver_options_mutex;
        RadioReceiverOptions receiver_options;

        std::thread threadHandle;
        int32_t syncBufferIndex = 0;
        RadioControllerInterface& radioInterface;
        InputInterface& input;
        const DABParams& params;
        FicHandler& ficHandler;
        std::vector<float> impulseResponseBuffer;
        TIIDecoder tiiDecoder;

        std::atomic<bool> running = ATOMIC_VAR_INIT(false);

        int32_t T_null;
        int32_t T_u;
        int32_t T_s;
        int32_t T_F;
        int32_t coarseSyncCounter = 0;

        std::vector<DSPCOMPLEX> oscillatorTable;

        int32_t localPhase = 0;

        float sLevel = 0;
        int32_t sampleCnt = 0;

        int16_t lastValidFineCorrector = 0;
        int32_t lastValidCoarseCorrector = 0;
        int16_t fineCorrector = 0;
        int32_t coarseCorrector = 0;

        uint32_t ofdmBufferIndex = 0;
        PhaseReference phaseRef;
        OfdmDecoder ofdmDecoder;
        std::vector<float> correlationVector;
        std::vector<float> refArg;

        bool scanMode = false;
        int attempts = 0;

        int32_t bufferContent = 0;

        fft::Forward fft_handler;
        DSPCOMPLEX *fft_buffer; // of size T_u

        DSPCOMPLEX getSample(int32_t);
        void getSamples(DSPCOMPLEX *, int16_t, int32_t);
        void run(void);
        int16_t processPRS(DSPCOMPLEX *v, const FreqsyncMethod& freqsyncMethod);
        int16_t getMiddle(DSPCOMPLEX *);
};
#endif

