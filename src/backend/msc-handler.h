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

/*
 *  MSC data
 */

#ifndef MSC_HANDLER
#define MSC_HANDLER

#include <mutex>
#include <list>
#include <memory>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstdio>
#include "dab-constants.h"
#include "ringbuffer.h"
#include "radio-controller.h"

class DabVirtual;

class MscHandler
{
    public:
        MscHandler(const DABParams& p, bool show_crcErrors);

        // Stop processing and remove all subchannels
        void stopProcessing(void);

        bool addSubchannel(
                ProgrammeHandlerInterface& handler,
                AudioServiceComponentType ascty,
                const std::string& dumpFileName,
                const Subchannel& sub);

        bool removeSubchannel(const Subchannel& sub);

    private:
        friend class OfdmDecoder;
        void processMscBlock(const softbit_t *fbits, int16_t blkno);

        struct SelectedStream {
            SelectedStream(
                ProgrammeHandlerInterface& handler,
                AudioServiceComponentType ascty,
                const std::string& dumpFileName,
                const Subchannel& subCh) :
                    handler(handler),
                    audioType(ascty),
                    dumpFileName(dumpFileName),
                    subCh(subCh) {}

            ProgrammeHandlerInterface& handler;

            AudioServiceComponentType audioType;
            const std::string dumpFileName;
            const Subchannel subCh;

            std::shared_ptr<DabVirtual> dabHandler;
        };

        std::mutex mutex;
        std::list<SelectedStream> streams;

        const int16_t bitsperBlock;
        int16_t numberofblocksperCIF;
        bool show_crcErrors;

        std::vector<softbit_t> cifVector;
        int16_t cifCount = 0; // msc blocks in CIF
        int16_t blkCount = 0;
        bool work_to_be_done = false;
};

#endif

