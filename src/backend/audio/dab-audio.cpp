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
#include "dab-constants.h"
#include "dab-audio.h"
#include "mp2processor.h"
#include "mp4processor.h"
#include "eep-protection.h"
#include "uep-protection.h"

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
        uint8_t dabModus,
        int16_t fragmentSize,
        int16_t bitRate,
        bool shortForm,
        int16_t protLevel,
        RadioControllerInterface& mr,
        const std::string& mscFileName,
        const std::string& mp2FileName) :
    myRadioInterface(mr),
    mscBuffer(64 * 32768),
    mscFileName(mscFileName),
    mp2FileName(mp2FileName)
{
    int32_t i;
    this->dabModus         = dabModus;
    this->fragmentSize     = fragmentSize;
    this->bitRate          = bitRate;
    this->shortForm        = shortForm;
    this->protLevel        = protLevel;

    outV.resize(bitRate * 24);
    for (i = 0; i < 16; i ++) {
        interleaveData[i].resize(fragmentSize);
    }

    using std::make_unique;

    if (shortForm)
        protectionHandler = make_unique<UEPProtection>(bitRate, protLevel);
    else
        protectionHandler = make_unique<EEPProtection>(bitRate, protLevel);

    if (dabModus == DAB) {
        our_dabProcessor = make_unique<Mp2Processor>(
                myRadioInterface, bitRate, mp2FileName);
    }
    else {
        if (dabModus == DAB_PLUS) {
            our_dabProcessor = make_unique<Mp4Processor>(
                    myRadioInterface, bitRate, mscFileName);
        }
        else        // cannot happen
            our_dabProcessor = make_unique<DummyProcessor>();
    }

    std::clog << "dab-audio:"
        " we have now " << ((dabModus == DAB_PLUS) ? "DAB+" : "DAB") << std::endl;

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

int32_t DabAudio::process(int16_t *v, int16_t cnt)
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
    int16_t i, j;
    int16_t countforInterleaver = 0;
    int16_t interleaverIndex    = 0;
    uint8_t shiftRegister[9];
    int16_t Data[fragmentSize];
    int16_t tempX[fragmentSize];

    while (running) {
        std::unique_lock<std::mutex> lock(ourMutex);
        while (running && mscBuffer.GetRingBufferReadAvailable() <= fragmentSize) {
            mscDataAvailable.wait(lock);
        }

        if (!running)
            break;

        // mscBuffer is threadsafe to access, no need to keep the lock
        lock.unlock();


        mscBuffer.getDataFromBuffer(Data, fragmentSize);

        for (i = 0; i < fragmentSize; i ++) {
            tempX[i] = interleaveData[(interleaverIndex +
                    interleaveMap[i & 017]) & 017][i];
            interleaveData[interleaverIndex][i] = Data[i];
        }
        interleaverIndex = (interleaverIndex + 1) & 0x0F;

        //  only continue when de-interleaver is filled
        if (countforInterleaver <= 15) {
            countforInterleaver ++;
            continue;
        }

        protectionHandler->deconvolve(tempX, fragmentSize, outV.data());

        //  and the inline energy dispersal
        memset (shiftRegister, 1, 9);
        for (i = 0; i < bitRate * 24; i ++) {
            uint8_t b = shiftRegister[8] ^ shiftRegister[4];
            for (j = 8; j > 0; j--)
                shiftRegister[j] = shiftRegister[j - 1];
            shiftRegister[0] = b;
            outV[i] ^= b;
        }
        our_dabProcessor->addtoFrame(outV.data());
    }
}

