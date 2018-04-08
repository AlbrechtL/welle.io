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
#include    "dab-constants.h"
#include    "msc-handler.h"
#include    "dab-virtual.h"
#include    "audio/dab-audio.h"

//  Interface program for processing the MSC.
//  Merely a dispatcher for the selected service
//
//  The ofdm processor assumes the existence of an msc-handler, whether
//  a service is selected or not.

#define CUSize  (4 * 16)
//  Note CIF counts from 0 .. 3
MscHandler::MscHandler(
        RadioControllerInterface& mr,
        const DABParams& p,
        bool show_crcErrors,
        const std::string& mscFileName,
        const std::string& mp2FileName) :
    radioInterface(mr),
    mscFileName(mscFileName),
    mp2FileName(mp2FileName),
    bitsperBlock(2 * p.K),
    show_crcErrors(show_crcErrors),
    cifVector(864 * CUSize)
{
    if (p.dabMode == 4) {  // 2 CIFS per 76 blocks
        numberofblocksperCIF = 36;
    }
    else {
        if (p.dabMode == 1) {  // 4 CIFS per 76 blocks
            numberofblocksperCIF = 18;
        }
        else {
            if (p.dabMode == 2)  // 1 CIF per 76 blocks
                numberofblocksperCIF = 72;
            else            // shouldnot/cannot happen
                numberofblocksperCIF = 18;
        }
    }
}

void MscHandler::setSubChannel(AudioServiceComponentType ascty,
        const Subchannel& sc)
{
    std::lock_guard<std::mutex> lock(mutex);
    audioType = ascty;
    subChannel = sc;
    newChannel = true;
}

//  add blocks. First is (should be) block 5, last is (should be) 76
//  Note that this method is called from within the ofdm-processor thread
//  while the set_xxx methods are called from within the 
//  gui thread
//
//  Any change in the selected service will only be active
//  during te next process_mscBlock call.
void MscHandler::process_mscBlock(int16_t *fbits, int16_t blkno)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!work_to_be_done && !newChannel)
        return;

    int16_t currentblk = (blkno - 4) % numberofblocksperCIF;

    if (newChannel) {
        newChannel = false;
        if (dabHandler) {
            dabHandler.reset();
        }

        dabHandler = std::make_shared<DabAudio>(
                audioType,
                subChannel.length * CUSize,
                subChannel.bitrate(),
                subChannel.shortForm,
                subChannel.protLevel,
                radioInterface,
                mscFileName,
                mp2FileName);
         /* TODO dealing with data
                    dabHandler = std::make_shared<DabData>(radioInterface,
                                              new_DSCTy,
                                              new_packetAddress,
                                              subChannel.length * CUSize,
                                              subChannel.bitrate(),
                                              subChannel.shortForm,
                                              subChannel.protLevel,
                                              new_DGflag,
                                              new_FEC_scheme,
                                              show_crcErrors);
            */

        //  these we need for actual processing
        startAddr = subChannel.startAddr;
        length    = subChannel.length;
        //  and this one to get started
        work_to_be_done  = true;
    }

    //  and the normal operation is:
    memcpy (&cifVector[currentblk * bitsperBlock],
            fbits, bitsperBlock * sizeof (int16_t));

    if (currentblk < numberofblocksperCIF - 1)
        return;

    //  OK, now we have a full CIF
    blkCount    = 0;
    cifCount    = (cifCount + 1) & 03;

    int16_t *myBegin = &cifVector[startAddr * CUSize];

    if (dabHandler) {
        (void)dabHandler->process(myBegin, length * CUSize);
    }
}

void MscHandler::stopProcessing()
{
    work_to_be_done = false;
}

