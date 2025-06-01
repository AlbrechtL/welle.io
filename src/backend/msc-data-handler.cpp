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

#include "dab-constants.h"
#include "dab-data.h"
#include "ctime"
#include <array>
#include <vector>
#include <algorithm>
#include <stdlib.h> 
#include "msc-data-handler.h"
#include "ringbuffer.h"
#include "dab-virtual.h"



//  Interface program for processing the MSC.
//  Merely a dispatcher for the selected service
//
//  The ofdm processor assumes the existence of an msc-handler, whether
//  a service is selected or not.

//  "MSC stream audio" (PAD part) -- ETSI TS 101 499
//  The Transport Mechanisms "MSC stream data" and "FIDC" shall not be used for the SlideShow application. 

//  An MSC data group shall carry one or more complete TPEG transport frames.
/*
 Main Service Channel: 4 * 864(CUs) * 64 bits : 2304 kbit/s
     [CIF #0   bis #3]
            [Symbol #5 bis #76]
                 [Guard  + (Symbol: 1536 carriers * 2 bits)]
                  246 us     1000 us       48 CU * 64 bits

*/


//__attribute__((optimize(0)))
#define CUSize  (4 * 16)
//  Note CIF counts from 0 .. 3
MscDataHandler::MscDataHandler(
        const DABParams& p,
        bool show_crcErrors) :
    bitsperBlock(2 * p.K),
    show_crcErrors(show_crcErrors),
    cifDataVector(864 * CUSize)
{
    if (p.dabMode == 4) {  // 2 CIFS per 76 blocks
        numberofblocksperCIF = 36;
    }
    else {
        if (p.dabMode == 1) {  // 4 CIFS per 76 blocks
            numberofblocksperCIF = 18; // ist wohl Standard?
        }
        else {
            if (p.dabMode == 2)  // 1 CIF per 76 blocks
                numberofblocksperCIF = 72;
            else            // shouldnot/cannot happen
                numberofblocksperCIF = 18;
        }
    }
}




// Aufruf von radio-receiver.cpp,  MscDataHandler.addDataSubchannel(handler, sc.DSCTy , dumpFileName, sc.packetAddress, sc.DGflag,sc.componentLabel.fig1_label_utf8(),sc.appType, subdatach);
bool MscDataHandler::addDataSubchannel(ProgrammeHandlerInterface& handlerData, const std::string& dumpFileName,uint16_t DSCTTy,  const Subchannel& subData, const ServiceComponent& sc, const Service& s)
{
   
   std::lock_guard<std::mutex> Datalock(Datamutex);
  
// check not already in list
    for (const auto& Datastream : Datastreams) {
        if (Datastream.subDataCh.subChId == subData.subChId) {
            return true;
        }
		
    }


    // ***************************************************
    // hier Global_* indirekt füllen
   uint16_t subID = sc.subchannelId;
   uint16_t DGflag = sc.DGflag;
   uint packetAddress = subData.startAddr;
   uint16_t bitRate =  subData.bitrate();
   uint16_t fragmentDataSize = subData.length * CUSize;
   uint16_t appType = sc.appType;
   uint32_t SId = sc.SId;

   
   SelectedDataStream d(handlerData,DSCTTy,dumpFileName, subData,SId,sc,s);
 // fprintf(stderr,"+++++++++++++++++++++++ Data stream dabHandler=%8x, DSCTy=%x, Datastreams=%x, fragmentsize=%x, d=%x, bitrate=%x\n",dumpFileName,DSCTTy,Datastreams.size(),fragmentSize,d,bitrate);

      if ((subData.length * CUSize==0) )  
            return false;
       d.dabDataHandler = std::make_shared<DabData>( // Datenübergabe an Datastream 
                DSCTTy,
                bitRate,
                subData.length * CUSize,
                // fragmentDataSize,
                subData.protectionSettings,
                packetAddress,
                handlerData,
                dumpFileName,
                subID,
                DGflag,
                appType,
                SId,
                sc,
                s);
    Datastreams.push_back(std::move(d));
    work_to_be_data_done = true;
    uint8_t   dss = Datastreams.size();
    fprintf(stderr, "\033[1;34m+++++++++++++++++++++++ Data stream %x eingerichtet mit fragmentSize=%x, packetAddress=%x, DSCTTy=%x, appType=%x\033[0m\n\n", dss, fragmentDataSize,packetAddress,DSCTTy,appType);
    
    return true;
}

bool MscDataHandler::removeDataSubchannel(const Subchannel& subdata)
{
   std::lock_guard<std::mutex> lock(Datamutex);

    auto it = std::find_if(Datastreams.begin(), Datastreams.end(),
            [&](const SelectedDataStream& Datastream) {
                return Datastream.subDataCh.subChId == subdata.subChId;
            } );

    if (it != Datastreams.end()) {
        Datastreams.erase(it);
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
//  during te next processMscBlock call.
//  MscDataHandler.processMscBlock(ibits.data(), sym_ix); //gebe Symbols (5 bis 76) einzelnd an msc-handler.cpp bestehend aus CIFs (Common Interleaved Frame(s))

/*
 Main Service Channel: 4 * 864(CUs) * 64 bits : 2304 kbit/s
     [CIF #0   bis #3]
            [Symbol #5 bis #76]
                 [Guard  + (Symbol: 1536 carriers * 2 bits)]
                  246 us     1000 us       48 CU * 64 bits

*/




// Aufruf kommt vom ofdm-decoder.cpp
//  MscDataHandler.processMscDataBlock(ibits.data(), sym_ix); //gebe Symbols (5 bis 76) einzelnd an msc-handler.cpp bestehend aus CIFs (Common Interleaved Frame(s))
void MscDataHandler::processMscDataBlock(const softbit_t *fbits, int16_t blkno) // Aufruf kommt vom ofdm-handler.cpp mit (Symbol,laufende Symbol Nummer)
{
   std::lock_guard<std::mutex> datalock(Datamutex);
 
    if (!work_to_be_data_done)
        return;

    int16_t currentblk = (blkno - 4) % numberofblocksperCIF;  // #76 Symbol - #5 Symbol = #71 -> 4 x CIFs à #17 Symbole, starte mit CIF[0]
   // fprintf(stderr," currentblk=%i, %i, %i\n ",currentblk,blkno,numberofblocksperCIF);
    //  and the normal operation is:
    memcpy(&cifDataVector[currentblk * bitsperBlock], fbits, bitsperBlock * sizeof(softbit_t));   // memcopy(ziel,quelle,anzahl)

    if (currentblk < numberofblocksperCIF - 1)
        return;                                 // erst alle Symbole holen, damit  4 CIFs per transmission frame of 96 ms

//	OK, now we have a full CIF and it seems there is some work to
//	be done.  We assume that the backend (streams) itself
//	does the work in a separate thread.
    blkCount = 0;
    cifCount = (cifCount + 1) & 03;

    for (auto& Datastream : Datastreams) {  // qt-dab = theBackends
        softbit_t *myDataBegin = &cifDataVector[Datastream.subDataCh.startAddr   *CUSize ];  // in &cifDataVector befinden sich die CIFs
   
        if ((Datastream.dabDataHandler) && (Datastream.subDataCh.length>0)  && (Datastream.subDataCh.startAddr>0)) {
            //std::this_thread::sleep_for(std::chrono::microseconds(1));
           (void)Datastream.dabDataHandler->processData(myDataBegin, Datastream.subDataCh.length   *CUSize ,Global_dumpFileName);  // qt-dab (void) b -> process (&cifDataVector [startAddr * CUSize], Length * CUSize);
       
        }
        else {
          // fprintf(stderr,"No dabDataHandler!");
        }
       // Datamutex.unlock();
        
    }
}

       
void MscDataHandler::stopProcessing()
{
    std::lock_guard<std::mutex> lock(Datamutex);
   
   work_to_be_data_done = false;
    Datastreams.clear();
}
/////88888888888888888888888888888888888888888888888888888888888888888888888888

