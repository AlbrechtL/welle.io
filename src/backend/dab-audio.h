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
#ifndef __DAB_AUDIO
#define __DAB_AUDIO

#include "dab-virtual.h"
#include <memory>
#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdio>
#include "ringbuffer.h"
#include "energy_dispersal.h"
#include "radio-controller.h"
#include "dab-processor.h"

class Protection;

class DabAudio : public DabVirtual, public FrameRedecoder
{
    public:
        DabAudio(AudioServiceComponentType dabModus,
                  int16_t fragmentSize,
                  int16_t bitRate,
                  ProtectionSettings protection,
                  ProgrammeHandlerInterface& phi,
                  const std::string& dumpFileName);
        // Only the channel decoding: the decoded logical frames are given
        // to processor, e.g. to the SPI decoder of a packet mode subchannel
        DabAudio(int16_t fragmentSize,
                  int16_t bitRate,
                  ProtectionSettings protection,
                  std::unique_ptr<DabProcessor> processor);
        virtual ~DabAudio(void);
        DabAudio(const DabAudio&) = delete;
        DabAudio& operator=(const DabAudio&) = delete;

        int32_t process(const softbit_t *v, int16_t cnt);

        // FrameRedecoder: the soft bits of the last frames are kept
        bool redecodeFrame(int age, const uint8_t *known, uint8_t *bytes) override;

    private:
        void    run(void);
        std::atomic<bool> running;
        int16_t fragmentSize;
        int16_t bitRate;
        std::vector<uint8_t> outV;
        std::vector<softbit_t> interleaveData[16];
        EnergyDispersal energyDispersal;

        std::condition_variable  mscDataAvailable;
        std::mutex               ourMutex;
        std::thread              ourThread;

        std::unique_ptr<Protection> protectionHandler;

        // Input of the Viterbi decoder of the last frames (a DAB+ superframe
        // has 5), for redecodeFrame()
        static const int historyFrames = 5;
        std::vector<softbit_t> history[historyFrames];
        int historyNext = 0;
        int historyCount = 0;
        std::vector<uint8_t> dispersalSequence;
        std::vector<int8_t> knownBits;
        std::vector<uint8_t> redecodedBits;
        std::unique_ptr<DabProcessor> our_dabProcessor;
        RingBuffer<softbit_t> mscBuffer;
};

#endif

