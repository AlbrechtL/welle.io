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
/*
 *
 */
#include "dab-constants.h"
#include <thread>
#include <atomic>
#include <vector>
#include "phasereference.h"
#include "ofdm-decoder.h"
#include "CVirtualInput.h"
#include "fft.h"
#include "radio-controller.h"
#include "fic-handler.h"
#include "msc-handler.h"
//
//  Note:
//  It was found that enlarging the buffersize to e.g. 8192
//  cannot be handled properly by the underlying system.
#define DUMPSIZE        4096

class OFDMProcessor
{
    public:
        OFDMProcessor(
                InputInterface& interface,
                const DABParams& params,
                RadioControllerInterface& ri,
                MscHandler& msc,
                FicHandler& fic,
                int16_t    threshold,
                uint8_t    freqsyncMethod);
        ~OFDMProcessor(void);
        void reset(void);
        void stop(void);
        void setOffset(int32_t);
        void coarseCorrectorOn(void);
        void set_scanMode(bool);
        void start(void);

    private:
        std::thread threadHandle;
        int32_t syncBufferIndex;
        RadioControllerInterface& radioInterface;
        InputInterface& input;
        const DABParams& params;
        FicHandler& ficHandler;
        std::vector<float> impulseResponseBuffer;

        std::atomic<bool> running;
        int32_t     T_null;
        int32_t     T_u;
        int32_t     T_s;
        int32_t     T_g;
        int32_t     T_F;
        float       sLevel;
        std::vector<DSPCOMPLEX> oscillatorTable;
        int32_t     localPhase;
        int16_t     fineCorrector;
        int32_t     coarseCorrector;
        int attempts = 0;


        uint8_t     freqsyncMethod;
        std::vector<DSPCOMPLEX> ofdmBuffer;
        uint32_t    ofdmBufferIndex;
        PhaseReference phaseRef;
        OfdmDecoder ofdmDecoder;
        std::vector<float> correlationVector;
        std::vector<float> refArg;
        int32_t     sampleCnt;
        bool        scanMode;
        int32_t     bufferContent;
        common_fft  fft_handler;
        DSPCOMPLEX  *fft_buffer; // of size T_u

        DSPCOMPLEX  getSample(int32_t);
        void        getSamples(DSPCOMPLEX *, int16_t, int32_t);
        void        run(void);
        int16_t     processPRS(DSPCOMPLEX *v);
        int16_t     getMiddle(DSPCOMPLEX *);
};
#endif

