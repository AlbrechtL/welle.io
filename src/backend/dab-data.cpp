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
 #include "dab-data.h"
 #include "eep-protection.h"
 #include "uep-protection.h"
 #include "profiling.h"
 #include "MathHelper.h"
 #include "decoder_data_adapter.h"
 #include <chrono>
 
 
 
 
 
 
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
 //  fragmentDataSize == Length * CUSize#
 // Aufruf kommt vom msc-handler.cpp 
 // wie qt-dab data-processor.cpp
 DabData::DabData(
         uint16_t                    DSCTy,
         int16_t                     bitDataRate,
         int16_t                     fragmentDataSize,
         ProtectionSettings          protection,
         uint                        packetAddress,
         ProgrammeHandlerInterface&  phiData,
         const std::string& dumpFileName,
         uint16_t subID,
         uint16_t DGflag,
         uint16_t appType,
         uint32_t SId,
        const ServiceComponent& sc,
        const Service& s):
                   myProgrammeDataHandler(phiData),
                 mscDataBuffer(64 * 32768),
                 dumpFileName(dumpFileName)
 
 {
    
     this    -> DSCTy                    = sc.DSCTy;
     this	-> myProgrammeDataHandler	= phiData;
     this	-> bitDataRate		        = bitDataRate;
     this	-> expectedIndex	        = 0;
     this	-> subChId      	        = sc.subchannelId;
     this    -> fragmentDataSize         = fragmentDataSize;
     this    -> dumpFileName             = dumpFileName;
     this    -> subName                  = dumpFileName;
     this    -> DGflag                   = sc.DGflag;
     this    -> appType                   = sc.appType;
     this   ->serviceId                    = sc.SId;
     this   ->SId                          = sc.SId;
  //  this	-> mscDataBuffer	= mscDataBuffer;
     Global_DSCTy    = sc.DSCTy;
     Global_appType  = sc.appType;
     Global_subID    = sc.subchannelId;
     Global_DGFlag   = sc.DGflag;
     Global_subName  = dumpFileName;
     Global_bitDataRate = bitDataRate;
     Global_SId     = sc.SId;
    
 
 // 55555555555555555555555555555555555555555555555555555555555
     outDataV.resize(bitDataRate * 24*8);
 
     for (int i = 0; i < 16; i ++) {
         interleaveData[i].resize(fragmentDataSize);
     }
 
     using std::make_unique;
 
     if (protection.shortForm) {
         protectionHandler = make_unique<UEPProtection>(bitDataRate, protection.uepLevel);
     }
     else {
         const bool profile_is_eep_a = protection.eepProfile == EEPProtectionProfile::EEP_A;
         protectionHandler = make_unique<EEPProtection>(bitDataRate, profile_is_eep_a, (int)protection.eepLevel);
     }
 
 // ###############################################################
 runningData = true;
 our_dabDataProcessor = std::make_unique<DecoderDataAdapter>(myProgrammeDataHandler, bitDataRate, DSCTy, dumpFileName);
 ourDataThread = std::thread(&DabData::run, this); // Binde Data Thread inklusive PAD MOT
 //fprintf (stderr, "dabdata-thread: %s ",dumpFileName.c_str());
 
 // ###############################################################
 
 }
 
 DabData::~DabData()
 {
     runningData = false;
     if (ourDataThread.joinable()) {
         mscDataDataAvailable.notify_all();
         ourDataThread.join();
     }
 }
 
 // ###############################################################
 // Aufruf kommt vom msc-handler.cpp mit CIFs genau wie bei qt-dab, nur dort in backend.cpp int32_t	Backend::process	(int16_t *v, int16_t cnt) {
 int32_t DabData::processData( softbit_t *v, uint32_t cnt, std::string dumpFileName)// ständiger Durchlauf der DSCTy 60 und 5, wenn vorhanden
 {
     uint32_t fr;

     if (mscDataBuffer.GetRingBufferWriteAvailable () < cnt)
         fprintf (stderr, "dab-concurrent: data buffer full\n");
        
       while ((fr = mscDataBuffer.GetRingBufferWriteAvailable ()) <= cnt) {  // warten bis der mscBuffer gefüllt wurde 
         if ((!runningData) || (fragmentDataSize==0))
             return 0;
         std::this_thread::sleep_for(std::chrono::microseconds(1));
     }
   
   
   mscDataBuffer.putDataIntoBuffer(v, cnt);  // kopiere v in den RingBuffer mscDataBuffer // (void)stream.dabHandler->process(myBegin, stream.subCh.length * CUSize);
     
   mscDataDataAvailable.notify_all();
   //  fprintf (stderr, "DabData::processData: Global_DSCTy=%d, Global_DGFlag=%d\n",Global_DSCTy,Global_DGFlag);// ständiger Durchlauf der DSCTy 60 und 5, wenn vorhanden
     return fr;
 }
 
 const int16_t interleaveMap[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
 // Aufruf von ourThread = std::thread(&DabData::run, this);
 void DabData::run()
 {

     int16_t i;
     int16_t countforInterleaver = 0;
     int16_t interleaverIndex    = 0;
     std::vector<softbit_t> data(fragmentDataSize);
     std::vector<softbit_t> tempX(fragmentDataSize);
     std::string kk;
     while (runningData) {
         std::unique_lock<std::mutex> datalock(ourDataMutex);
         while (runningData && mscDataBuffer.GetRingBufferReadAvailable() <= fragmentDataSize) { // warte auf das Füllen des mscbuffer mit Daten
            mscDataDataAvailable.wait(datalock);
           // std::this_thread::sleep_for(std::chrono::microseconds(1));
         }
         if (!runningData)
             break;
 
         // mscBuffer is threadsafe to access, no need to keep the lock
         datalock.unlock();
       //  Global_audiorunning=true;
        PROFILE(DADataGetMSCData);
         mscDataBuffer.getDataFromBuffer(data.data(), fragmentDataSize); // übergebe Daten aus dem Ringbuffer mscBuffer an data
 
     //  fprintf (stderr, "dDabAudio::process: buffer=%x, databuffer=%x, xchangedata=%x\n",mscBuffer,mscBuffer,xchangedata);
 
 
 
        PROFILE(DADataDeinterleave);
         for (i = 0; i < fragmentDataSize; i ++) {
             tempX[i] = interleaveData[(interleaverIndex + interleaveMap[i & 017]) & 017][i];
             interleaveData[interleaverIndex][i] = data[i];
         }
         interleaverIndex = (interleaverIndex + 1) & 0x0F;
 
         //  only continue when de-interleaver is filled
         if (countforInterleaver <= 15) {
             countforInterleaver ++;
             continue;
         }
 
         PROFILE(DADataDeconvolve);
         protectionHandler->deconvolve(tempX.data(), fragmentDataSize, outDataV.data());
 
        PROFILE(DADataDispersal);
         // and the inline energy dispersal
         energyDispersal.dedisperse(outDataV); 
 
         if (our_dabDataProcessor) {
             //appType = this->appType;
             subID = this->subChId;
             Global_SId = this->SId;
            // DGflag = this->DGflag;
            // DSCTy = this->DSCTy;
            // dumpFileName = this->dumpFileName;
             PROFILE(DADataDecode);
            // std::this_thread::sleep_for(std::chrono::microseconds(6));
 //***************************************************************************** */
           our_dabDataProcessor->addtoDatenFrame(outDataV.data(), fragmentDataSize,dumpFileName, DSCTy, DGflag,Global_SId, subID,appType);  // so oft wie es Datenstreams gibt (Bitstream)
 //***************************************************************************** */
 
 
         }
         PROFILE(DADataDone);
     }
 }
 
 