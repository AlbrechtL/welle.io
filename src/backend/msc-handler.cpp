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
#include <algorithm>
#include "dab-constants.h"
#include "msc-handler.h"
#include "dab-virtual.h"
#include "dab-audio.h"

//  Interface program for processing the MSC.
//  Merely a dispatcher for the selected service
//
//  The ofdm processor assumes the existence of an msc-handler, whether
//  a service is selected or not.

#define CUSize  (4 * 16)
//  Note CIF counts from 0 .. 3
MscHandler::MscHandler(
        const DABParams& p,
        bool show_crcErrors,
        bool etiMode) :
    bitsperBlock(2 * p.K),
    show_crcErrors(show_crcErrors),
    etiMode(etiMode),
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

bool MscHandler::addSubchannel(
        ProgrammeHandlerInterface& handler,
        AudioServiceComponentType ascty,
        const std::string& dumpFileName,
        const Subchannel& sub)
{
    std::lock_guard<std::mutex> lock(mutex);

    // check not already in list
    for (const auto& stream : streams) {
        if (stream.subCh.subChId == sub.subChId) {
            return true;
        }
    }

    SelectedStream s(handler, ascty, dumpFileName, sub);

    if (etiMode) {
        auto audioType = ascty;
        s.etiDecoder = std::make_unique<DecoderAdapter>(
                handler,
                sub.bitrate(),
                audioType,
                dumpFileName);
    }
    else {
        s.dabHandler = std::make_shared<DabAudio>(
                    ascty,
                    sub.length * CUSize,
                    sub.bitrate(),
                    sub.protectionSettings,
                    handler,
                    dumpFileName);
    }

     /* TODO dealing with data
      s.dabHandler = std::make_shared<DabData>(radioInterface,
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

    streams.push_back(std::move(s));

    work_to_be_done = true;
    return true;
}

bool MscHandler::removeSubchannel(const Subchannel& sub)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto it = std::find_if(streams.begin(), streams.end(),
            [&](const SelectedStream& stream) {
                return stream.subCh.subChId == sub.subChId;
            } );

    if (it != streams.end()) {
        streams.erase(it);
        return true;
    }

    return false;
}

//  add blocks. First is (should be) block 5, last is (should be) 76
//  Note that this method is called from within the ofdm-processor thread
//  while the set_xxx methods are called from within the
//  gui thread
//
//  Any change in the selected service will only be active
//  during the next processMscBlock call.
void MscHandler::processMscBlock(const softbit_t *fbits, int16_t blkno)
{
    if (etiMode) {
        (void)fbits;
        (void)blkno;
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);

    if (!work_to_be_done)
        return;

    int16_t currentblk = (blkno - 4) % numberofblocksperCIF;

    //  and the normal operation is:
    memcpy(&cifVector[currentblk * bitsperBlock], fbits, bitsperBlock * sizeof(softbit_t));

    if (currentblk < numberofblocksperCIF - 1)
        return;

    //  OK, now we have a full CIF
    blkCount = 0;
    cifCount = (cifCount + 1) & 03;

    for (auto& stream : streams) {
        softbit_t *myBegin = &cifVector[stream.subCh.startAddr * CUSize];

        if (stream.dabHandler) {
            (void)stream.dabHandler->process(myBegin, stream.subCh.length * CUSize);
        }
        else {
            throw std::logic_error("No dabHandler!");
        }
    }
}

// AI: ETI path — dispatches one subchannel's payload from an ETI frame to
// its registered DecoderAdapter.  Matching is done by subchannel ID (primary)
// or by start-address + length (fallback).  The STL unit is 64 bits = 8 bytes
// per ETSI EN 300 799, so expectedBytes = stl * 8.
void MscHandler::processEtiStream(
        uint8_t subChId,
        uint16_t startAddr,
        uint16_t stl,
        const uint8_t *data,
        size_t size)
{
    if (!etiMode || data == nullptr || size == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);

    if (!work_to_be_done) {
        return;
    }

    for (auto& stream : streams) {
        const bool sidMatch = stream.subCh.subChId == subChId;
        const bool sadLenMatch =
            (stream.subCh.startAddr == static_cast<int16_t>(startAddr)) &&
            (stream.subCh.length == static_cast<int16_t>(stl));

        if (!(sidMatch || sadLenMatch)) {
            continue;
        }

        if (!stream.etiDecoder) {
            continue;
        }

        // ETI stream payload length is STL * 8 bytes (STL unit = 64 bits per ETSI EN 300 799).
        const size_t expectedBytes = static_cast<size_t>(stl) * 8;
        if (expectedBytes == 0) {
            continue;
        }

        if (size < expectedBytes) {
            continue;
        }

        stream.etiDecoder->feedRawFrame(data, expectedBytes);
    }
}

void MscHandler::stopProcessing()
{
    std::lock_guard<std::mutex> lock(mutex);
    work_to_be_done = false;
    streams.clear();
}

