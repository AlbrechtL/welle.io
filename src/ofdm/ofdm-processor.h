/*
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
#include    "DabConstants.h"
#include    <thread>
#include    <atomic>
#include    <vector>
#include    "phasereference.h"
#include    "ofdm-decoder.h"
#include    "CVirtualInput.h"
#include    "fft.h"
//
//  Note:
//  It was found that enlarging the buffersize to e.g. 8192
//  cannot be handled properly by the underlying system.
#define DUMPSIZE        4096
class   CRadioController;
class   ficHandler;
class   mscHandler;

class ofdmProcessor
{
    public:
        ofdmProcessor(
                CVirtualInput  *theRig,
                CDABParams *params,
                CRadioController *mr,
                mscHandler     *msc,
                ficHandler     *fic,
                int16_t    threshold,
                uint8_t    freqsyncMethod,
                std::shared_ptr<std::vector<float>> impulseResponseBuffer);
        ~ofdmProcessor(void);
        void reset(void);
        void stop(void);
        void setOffset(int32_t);
        void coarseCorrectorOn(void);
        void coarseCorrectorOff(void);
        void set_scanMode(bool);
        void start(void);

    private:
        std::thread threadHandle;
        int32_t syncBufferIndex;
        CVirtualInput    *theRig;
        CDABParams       *params;
        CRadioController *myRadioInterface;
        ficHandler       *myFicHandler;
        std::shared_ptr<std::vector<float>> impulseResponseBuffer;

        std::atomic<bool> running;
        int16_t     gain;
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

        uint8_t     freqsyncMethod;
        bool        f2Correction;
        std::vector<DSPCOMPLEX> ofdmBuffer;
        uint32_t    ofdmBufferIndex;
        uint32_t    ofdmSymbolCount;
        phaseReference phaseSynchronizer;
        ofdmDecoder my_ofdmDecoder;
        std::vector<float> correlationVector;
        std::vector<float> refArg;
        int32_t     sampleCnt;
        DSPCOMPLEX  getSample(int32_t);
        void        getSamples(DSPCOMPLEX *, int16_t, int32_t);
        bool        scanMode;
        void        run(void);
        int32_t     bufferContent;
        int16_t     processBlock_0(DSPCOMPLEX *);
        int16_t     getMiddle(DSPCOMPLEX *);
        common_fft  fft_handler;
        DSPCOMPLEX  *fft_buffer;
};
#endif

