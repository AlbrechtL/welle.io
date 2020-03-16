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

#include <iostream>
#include <vector>
#include "dab-constants.h"
#include "dab-audio.h"
#include "decoder_adapter.h"
#include "eep-protection.h"
#include "uep-protection.h"
#include "profiling.h"

//  As an experiment a version of the backend is created
//  that will be running in a separate thread. Might be
//  useful for multicore processors.
//
//  Interleaving is - for reasons of simplicity - done
//  inline rather than through a special class-object
//static
//int8_t    interleaveDelays[] = {
//       15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0};
//
//
//  fragmentsize == Length * CUSize
DabAudio::DabAudio(
        AudioServiceComponentType dabModus,
        int16_t fragmentSize,
        int16_t bitRate,
        ProtectionSettings protection,
        ProgrammeHandlerInterface& phi,
        const std::string& dumpFileName) :
    myProgrammeHandler(phi),
    mscBuffer(64 * 32768),
    dumpFileName(dumpFileName)
{
    this->dabModus         = dabModus;
    this->fragmentSize     = fragmentSize;
    this->bitRate          = bitRate;

    outV.resize(bitRate * 24);
    for (int i = 0; i < 16; i ++) {
        interleaveData[i].resize(fragmentSize);
    }

    using std::make_unique;

    if (protection.shortForm) {
        protectionHandler = make_unique<UEPProtection>(bitRate, protection.uepLevel);
    }
    else {
        const bool profile_is_eep_a =
            protection.eepProfile == EEPProtectionProfile::EEP_A;
        protectionHandler = make_unique<EEPProtection>(
                bitRate, profile_is_eep_a, (int)protection.eepLevel);
    }

    our_dabProcessor = make_unique<DecoderAdapter>(
            myProgrammeHandler, bitRate, dabModus, dumpFileName);

    running = true;
    ourThread = std::thread(&DabAudio::run, this);
}

DabAudio::~DabAudio()
{
    running = false;

    if (ourThread.joinable()) {
        mscDataAvailable.notify_all();
        ourThread.join();
    }
}

int32_t DabAudio::process(const softbit_t *v, int16_t cnt)
{
    int32_t fr;

    if (mscBuffer.GetRingBufferWriteAvailable () < cnt)
        fprintf (stderr, "dab-concurrent: buffer full\n");

    while ((fr = mscBuffer.GetRingBufferWriteAvailable ()) <= cnt) {
        if (!running)
            return 0;
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    mscBuffer.putDataIntoBuffer(v, cnt);
    mscDataAvailable.notify_all();
    return fr;
}

const int16_t interleaveMap[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};

void DabAudio::run()
{
    int16_t i;
    int16_t countforInterleaver = 0;
    int16_t interleaverIndex    = 0;
    std::vector<softbit_t> data(fragmentSize);
    std::vector<softbit_t> tempX(fragmentSize);

    while (running) {
        std::unique_lock<std::mutex> lock(ourMutex);
        while (running && mscBuffer.GetRingBufferReadAvailable() <= fragmentSize) {
            mscDataAvailable.wait(lock);
        }
        if (!running)
            break;

        // mscBuffer is threadsafe to access, no need to keep the lock
        lock.unlock();

        PROFILE(DAGetMSCData);
        mscBuffer.getDataFromBuffer(data.data(), fragmentSize);

        PROFILE(DADeinterleave);
        for (i = 0; i < fragmentSize; i ++) {
            tempX[i] = interleaveData[(interleaverIndex +
                    interleaveMap[i & 017]) & 017][i];
            interleaveData[interleaverIndex][i] = data[i];
        }
        interleaverIndex = (interleaverIndex + 1) & 0x0F;

        //  only continue when de-interleaver is filled
        if (countforInterleaver <= 15) {
            countforInterleaver ++;
            continue;
        }

        PROFILE(DADeconvolve);
        protectionHandler->deconvolve(tempX.data(), fragmentSize, outV.data());

        PROFILE(DADispersal);
        // and the inline energy dispersal
        energyDispersal.dedisperse(outV);

        if (our_dabProcessor) {
            PROFILE(DADecode);
            our_dabProcessor->addtoFrame(outV.data());
        }
        PROFILE(DADone);
    }
}

