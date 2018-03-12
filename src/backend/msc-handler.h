/*
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

/*
 *  MSC data
 */

#ifndef MSC_HANDLER
#define MSC_HANDLER

#include    <mutex>
#include    <memory>
#include    <vector>
#include    <stdio.h>
#include    <stdint.h>
#include    <stdio.h>
#include    "DabConstants.h"
#include    "ringbuffer.h"

class   CRadioController;
class   dabVirtual;

class mscHandler
{
    public:
        mscHandler(CRadioController *,
                CDABParams *,
                std::shared_ptr<RingBuffer<int16_t> >,
                bool show_crcErrors);
        void process_mscBlock(int16_t *fbits, int16_t blkno);
        void set_audioChannel(audiodata *d);
        void set_dataChannel(packetdata *d);
        void stopProcessing(void);
    private:
        CRadioController    *myRadioInterface;
        std::shared_ptr<RingBuffer<int16_t>> buffer;
        bool        show_crcErrors;
        std::mutex  mutex;
        bool        audioService = true;     // default
        std::shared_ptr<dabVirtual> dabHandler;
        std::vector<int16_t> cifVector;
        int16_t     cifCount = 0; // msc blocks in CIF
        int16_t     blkCount = 0;
        bool        work_to_be_done = false;
        bool        newChannel = false;
        int16_t     new_packetAddress = 0;
        int16_t     new_ASCTy = 0;
        int16_t     new_DSCTy = 0;
        int16_t     new_startAddr = 0;
        int16_t     new_Length = 0;
        bool        new_shortForm = 0;
        int16_t     new_protLevel = 0;
        uint8_t     new_DGflag = 0;
        int16_t     new_bitRate = 0;
        int16_t     new_language = 0;
        int16_t     new_type = 0;
        int16_t     new_FEC_scheme = 0;
        int16_t     startAddr = 0;
        int16_t     Length = 0;
        int8_t      dabModus = 0;
        int8_t      new_dabModus = 0;
        int16_t     BitsperBlock;
        int16_t     numberofblocksperCIF;
};

#endif

