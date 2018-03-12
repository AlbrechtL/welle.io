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
        ~mscHandler(void);
        void process_mscBlock(int16_t *fbits, int16_t blkno);
        void set_audioChannel(audiodata  *);
        void set_dataChannel(packetdata *);
        void stopProcessing(void);
    private:
        CRadioController    *myRadioInterface;
        std::shared_ptr<RingBuffer<int16_t>> buffer;
        bool        show_crcErrors;
        std::mutex  mutex;
        bool        audioService;
        dabVirtual *dabHandler;
        int16_t    *cifVector;
        int16_t     cifCount;
        int16_t     blkCount;
        bool        work_to_be_done;
        bool        newChannel;
        int16_t     new_packetAddress;
        int16_t     new_ASCTy;
        int16_t     new_DSCTy;
        int16_t     new_startAddr;
        int16_t     new_Length;
        bool        new_shortForm;
        int16_t     new_protLevel;
        uint8_t     new_DGflag;
        int16_t     new_bitRate;
        int16_t     new_language;
        int16_t     new_type;
        int16_t     new_FEC_scheme;
        int16_t     startAddr;
        int16_t     Length;
        int8_t      dabModus;
        int8_t      new_dabModus;
        int16_t     BitsperBlock;
        int16_t     numberofblocksperCIF;
        int16_t     blockCount;
};

#endif


