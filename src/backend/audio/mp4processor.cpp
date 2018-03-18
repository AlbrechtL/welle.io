/*
 *    Copyright (C) 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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
 *  superframer for the SDR-J DAB+ receiver
 *  This processor handles the whole DAB+ specific part
 ************************************************************************
 *  may 15 2015. A real improvement on the code
 *  is the addition from Stefan Poeschel to create a
 *  header for the aac that matches, really a big help!!!!
 ************************************************************************
 */

#include <cstring>
#include <iostream>

#include "mp4processor.h"
#include "charsets.h"
#include "MathHelper.h"

/**
 * \class mp4Processor is the main handler for the aac frames
 * the class proper processes input and extracts the aac frames
 * that are processed by the "faadDecoder" class
 */
mp4Processor::mp4Processor(
        RadioControllerInterface& mr,
        int16_t bitRate,
        const std::string& mscFileName) :
    myRadioInterface(mr),
    the_rsDecoder(8, 0435, 0, 1, 10),
    padDecoder(this, true)
{
    // Open a MSC file (XPADxpert) if the user defined it
    if (!mscFileName.empty()) {
        FILE* fd = fopen(mscFileName.c_str(), "wb");  // w for write, b for binary
        if (fd != nullptr) {
            mscFile.reset(fd);
        }
    }

    this->bitRate  = bitRate;  // input rate

    superFramesize = 110 * (bitRate / 8);
    RSDims         = bitRate / 8;
    frameBytes.resize(RSDims * 120);   // input
    outVector.resize(RSDims * 110);
    blockFillIndex = 0;
    blocksInBuffer = 0;
    frameCount     = 0;
    frameErrors    = 0;
    aacErrors      = 0;
    aacFrames      = 0;
    successFrames  = 0;
    rsErrors       = 0;
    //
    //  error display
    au_count    = 0;
    au_errors   = 0;
    aacAudioMode = AACAudioMode::Unknown;
}

void mp4Processor::PADChangeDynamicLabel(const DL_STATE& dl)
{
    myRadioInterface.onNewDynamicLabel(
            toUtf8StringUsingCharset(
                (const char *)&dl.raw[0],
                (CharacterSet) dl.charset,
                dl.raw.size()));
}

void mp4Processor::PADChangeSlide(const MOT_FILE& slide)
{
    myRadioInterface.onMOT(slide.data, slide.content_sub_type);
}

/**
 * \brief addtoFrame
 *
 * a DAB+ superframe consists of 5 consecutive DAB frames
 * we add vector for vector to the superframe. Once we have
 * 5 lengths of "old" frames, we check
 * Note that the packing in the entry vector is still one bit
 * per Byte, nbits is the number of Bits (i.e. containing bytes)
 * the function adds nbits bits, packed in bytes, to the frame
 */
void mp4Processor::addtoFrame(uint8_t *V)
{
    int16_t i, j;
    uint8_t temp    = 0;
    int16_t nbits   = 24 * bitRate;

    for (i = 0; i < nbits / 8; i ++) {  // in bytes
        temp = 0;
        for (j = 0; j < 8; j ++) {
            temp = (temp << 1) | (V[i * 8 + j] & 01);
        }
        frameBytes[blockFillIndex * nbits / 8 + i] = temp;
    }

    blocksInBuffer ++;
    blockFillIndex = (blockFillIndex + 1) % 5;

    /**
     * we take the last five blocks to look at
     */
    if (blocksInBuffer >= 5) {
        /// first, we show the "successrate"
        if (++frameCount >= 25) {
            frameCount = 0;
            myRadioInterface.onFrameErrors(frameErrors);
            frameErrors = 0;
        }

        /**
         * starting for real: check the fire code
         * if the firecode is OK, we handle the frame
         * and adjust the buffer here for the next round
         */
        if (fc.check (&frameBytes[blockFillIndex * nbits / 8]) &&
                (processSuperframe (frameBytes.data(),
                                    blockFillIndex * nbits / 8))) {
            //  since we processed a full cycle of 5 blocks, we just start a
            //  new sequence, beginning with block blockFillIndex
            blocksInBuffer    = 0;
            if (++successFrames > 25) {
                myRadioInterface.onRsErrors(rsErrors);
                successFrames  = 0;
                rsErrors   = 0;
            }
        }
        else {
            /**
             * we were wrong, virtual shift to left in block sizes
             */
            blocksInBuffer  = 4;
            frameErrors ++;
        }
    }
}

/**
 * \brief processSuperframe
 *
 * First, we know the firecode checker gave green light
 * We correct the errors using RS
 */
bool mp4Processor::processSuperframe(uint8_t frameBytes[], int16_t base)
{
    uint8_t     num_aus;
    int16_t     i, j, k;
    uint8_t     rsIn[120];
    uint8_t     rsOut[110];
    uint8_t     dacRate;
    uint8_t     sbrFlag;
    uint8_t     aacChannelMode;
    //uint8_t     psFlag;
    uint16_t    mpegSurround;

    /**
     * apply reed-solomon error repar
     * OK, what we now have is a vector with RSDims * 120 uint8_t's
     * the superframe, containing parity bytes for error repair
     * take into account the interleaving that is applied.
     */
    for (j = 0; j < RSDims; j ++) {
        int16_t ler  = 0;
        for (k = 0; k < 120; k ++) {
            rsIn[k] = frameBytes[(base + j + k * RSDims) % (RSDims * 120)];
        }
        ler = the_rsDecoder. dec (rsIn, rsOut, 135);
        if (ler < 0) {
            rsErrors ++;
            return false;
        }

        for (k = 0; k < 110; k ++) {
            outVector[j + k * RSDims] = rsOut[k];
        }
    }

    // MSC file
    if (mscFile) {
        fwrite(outVector.data(), outVector.size(), 1, mscFile.get());
    }

    //  bits 0 .. 15 is firecode
    //  bit 16 is unused
    dacRate     = (outVector[2] >> 6) & 01;    // bit 17
    sbrFlag     = (outVector[2] >> 5) & 01;    // bit 18
    aacChannelMode  = (outVector[2] >> 4) & 01;    // bit 19
    //  psFlag      = (outVector[2] >> 3) & 01;    // bit 20
    mpegSurround    = (outVector[2] & 07);     // bits 21 .. 23

    switch (2 * dacRate + sbrFlag) {
        default:      // cannot happen
        case 0:
            num_aus = 4;
            au_start[0] = 8;
            au_start[1] = outVector[3] * 16 + (outVector[4] >> 4);
            au_start[2] = (outVector[4] & 0xf) * 256 +
                outVector[5];
            au_start[3] = outVector[6] * 16 +
                (outVector[7] >> 4);
            au_start[4] = 110 *  (bitRate / 8);
            break;
            //
        case 1:
            num_aus = 2;
            au_start[0] = 5;
            au_start[1] = outVector[3] * 16 +
                (outVector[4] >> 4);
            au_start[2] = 110 *  (bitRate / 8);
            break;
            //
        case 2:
            num_aus = 6;
            au_start[0] = 11;
            au_start[1] = outVector[3] * 16 + (outVector[4] >> 4);
            au_start[2] = (outVector[4] & 0xf) * 256 + outVector[ 5];
            au_start[3] = outVector[6] * 16 + (outVector[7] >> 4);
            au_start[4] = (outVector[7] & 0xf) * 256 + outVector[8];
            au_start[5] = outVector[9] * 16 + (outVector[10] >> 4);
            au_start[6] = 110 *  (bitRate / 8);
            break;
            //
        case 3:
            num_aus = 3;
            au_start[0] = 6;
            au_start[1] = outVector[3] * 16 + (outVector[4] >> 4);
            au_start[2] = (outVector[4] & 0xf) * 256 + outVector[5];
            au_start[3] = 110 * (bitRate / 8);
            break;
    }
    /**
     * OK, the result is N * 110 * 8 bits (still single bit per byte!!!)
     * extract the AU's, and prepare a buffer,  with the sufficient
     * lengthy for conversion to PCM samples
     */
    for (i = 0; i < num_aus; i ++) {
        int16_t  aac_frame_length;
        au_count ++;

        /// sanity check 1
        if (au_start[i + 1] < au_start[i]) {
            //  should not happen, all errors were corrected
            fprintf (stderr, "%d %d\n", au_start[i + 1], au_start[i]);
            return false;
        }

        aac_frame_length = au_start[i + 1] - au_start[i] - 2;
        //  just a sanity check
        if ((aac_frame_length >=  960) || (aac_frame_length < 0)) {
            return false;
        }

        //  but first the crc check
        if (check_crc_bytes (&outVector[au_start[i]], aac_frame_length)) {
            bool err;
            handle_aacFrame(&outVector[au_start[i]],
                    aac_frame_length,
                    dacRate,
                    sbrFlag,
                    mpegSurround,
                    aacChannelMode,
                    &err);
            if (err) {
                aacErrors ++;
            }

            if (++aacFrames > 25) {
                myRadioInterface.onAacErrors(aacErrors);
                aacErrors  = 0;
                aacFrames  = 0;
            }
        }
        else {
            std::clog << "mp4processor:" <<  "CRC failure with dab+ frame" << i << "(" << num_aus << ")" << std::endl;
        }
    }
    return true;
}

void mp4Processor::handle_aacFrame(
        uint8_t *v,
        int16_t frame_length,
        uint8_t dacRate,
        uint8_t sbrFlag,
        uint8_t mpegSurround,
        uint8_t aacChannelMode,
        bool *error)
{
    bool isParametricStereo = false;
    uint32_t sampleRate = 0;
    uint8_t theAudioUnit[2 * 960 + 10];    // sure, large enough

    memcpy(theAudioUnit, v, frame_length);
    memset(&theAudioUnit[frame_length], 0, 10);

    if (((theAudioUnit[0] >> 5) & 07) == 4) {
        processPAD(theAudioUnit);
    }

    auto audio = aacDecoder.MP42PCM(dacRate,
            sbrFlag,
            mpegSurround,
            aacChannelMode,
            theAudioUnit,
            frame_length,
            &sampleRate,
            &isParametricStereo);

    if (error) {
        *error = audio.empty();
    }

    AACAudioMode aacAudioMode_tmp = AACAudioMode::Unknown;
    myRadioInterface.onNewAudio(move(audio), sampleRate);

    if (aacChannelMode == 0 && isParametricStereo == true) // Parametric stereo
        aacAudioMode_tmp = AACAudioMode::ParametricStereo;
    else if (aacChannelMode == 1) // Stereo
        aacAudioMode_tmp = AACAudioMode::Stereo;
    else if (aacChannelMode == 0) // Mono
        aacAudioMode_tmp = AACAudioMode::Mono;

    if(aacAudioMode != aacAudioMode_tmp) {
        aacAudioMode = aacAudioMode_tmp;
        switch(aacAudioMode) {
            case AACAudioMode::Mono:
                std::clog << "mp4processor:"
                    "Detected mono audio signal" << std::endl;
                myRadioInterface.onStereoChange(false);
                break;
            case AACAudioMode::Stereo:
                std::clog << "mp4processor:"
                    "Detected stereo audio signal" << std::endl;
                myRadioInterface.onStereoChange(true);
                break;
            case AACAudioMode::ParametricStereo:
                std::clog << "mp4processor:"
                    "Detected parametric stereo audio signal" << std::endl;
                myRadioInterface.onStereoChange(true);
                break;
            default:
                std::clog << "mp4processor:"
                    "Unknown audio mode" << std::endl;
        }
    }
}

void mp4Processor::processPAD(const uint8_t *data)
{
    // Get PAD length
    uint8_t pad_start = 2;
    uint8_t pad_len = data[1];
    if (pad_len == 255) {
        pad_len += data[2];
        pad_start++;
    }

    // Adapt to PADDecoder
    uint8_t FPAD_LEN = 2;
    size_t xpad_len = pad_len - FPAD_LEN;
    const uint8_t *fpad = data + pad_start + pad_len - FPAD_LEN;

    // Run PADDecoder
    padDecoder.Process(data + pad_start, xpad_len, true, fpad);
}

