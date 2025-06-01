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

#ifndef MSC_DATA_HANDLER
#define MSC_DATA_HANDLER

#include <mutex>
#include <list>
#include <memory>
#include <vector>
#include <cstdio>
#include <cstdint>
#include "dab-constants.h"
#include "radio-controller.h"
#include "fib-processor.h"

#include <chrono>
#include <array>
#include <vector>
#include <algorithm>
//__attribute__((optimize(0)))
#include "dab-virtual.h"

class DabDataVirtual;

//class SubchannelDataSink;
//class DecoderDataAdapter;


class MscDataHandler
{
    public:


    
        MscDataHandler(const DABParams& p, bool show_crcErrors);

        // Stop processing and remove all subchannels
        void stopProcessing(void);


        bool addDataSubchannel(
                ProgrammeHandlerInterface& handlerData,
                const std::string& dumpFileName,
                uint16_t DSCTTy,
                const Subchannel& subDataCh,
                const ServiceComponent& sc,
                const Service& s);

        bool removeDataSubchannel(const Subchannel& sub);
        
        friend class DecoderDataAdapter;
        friend class OfdmDecoder;

        void processMscDataBlock(const softbit_t *fbits, int16_t blkno);
//#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#
        struct SelectedDataStream {
                SelectedDataStream(
                ProgrammeHandlerInterface& handlerData,
                uint16_t DSCTTy,
                const std::string& dumpFileName,
                const Subchannel& subDataCh,
                uint32_t SId,
                const ServiceComponent& sc,
                const Service& s) :
                    handlerData(handlerData),
                    DSCTTy(DSCTTy),
                    dumpFileName(dumpFileName),
                    subDataCh(subDataCh),
                    SId(SId),
                    sc(sc) {}

            ProgrammeHandlerInterface& handlerData;
            
            uint16_t DSCTTy;
            const std::string dumpFileName;
            uint32_t    packetAddress;
            uint16_t    DGflag;
            uint16_t appType;
            const Subchannel subDataCh;
            uint32_t SId;
            RingBuffer<std::complex<uint8_t>> *mscDataBuffer;

            std::shared_ptr<DabDataVirtual> dabDataHandler;
            ServiceComponent sc;
        };  
//#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#


        std::mutex Datamutex;


        const int16_t bitsperBlock;
        int16_t numberofblocksperCIF;
        bool show_crcErrors;


        std::vector<softbit_t> cifDataVector;
        int16_t cifCount = 0; // msc blocks in CIF
        int16_t blkCount = 0;

        bool work_to_be_data_done = false;
        
       
        std::list<SelectedDataStream> Datastreams;
        
     bool   set_dataChannel (packetdata &d,RingBuffer<std::complex<uint8_t>>  *b);
     bool  set_Channel (descriptorType &d,/* RingBuffer<std::complex<int16_t>> *audioBuffer,*/ RingBuffer<uint8_t> *dataBuffer, FILE *dump);

         
        
};

#endif

