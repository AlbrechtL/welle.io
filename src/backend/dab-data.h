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
#ifndef __DAB_DATA
#define __DAB_DATA

#include "dab-virtual.h"
#include <memory>
#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdio>
#include "energy_dispersal.h"
#include "radio-controller.h"
#include "dab-constants.h"
#include "msc-data-handler.h"
#include <chrono>
#include <array>
#include <vector>
#include <algorithm>
#include	"ringbuffer.h"

class MscDataHandler;
class DabDataProcessor;
class Protection;


class DabData : public DabDataVirtual
{
    public:
        DabData(
        uint16_t DSCTy,   
        int16_t bitDataRate,
        int16_t fragmentDataSize,
        ProtectionSettings protection,
        uint    packetAddress,
        ProgrammeHandlerInterface& phidata,
        const std::string& dumpFileName,
        uint16_t subID,
        uint16_t DGflag,
        uint16_t appType,
        uint32_t SId,
        const ServiceComponent& sc,
        const Service& s);
        
        virtual ~DabData(void)  ;
        DabData(const DabData&)=delete;
        DabData& operator=(const DabData&) = delete; // wofür?
        
 //std::list<SelectedDataStream> Datastreams;

// Aufruf kommt vom msc-handler.cpp mit CIFs genau wie bei qt-dab, nur dort in backend.cpp int32_t	Backend::process	(int16_t *v, int16_t cnt) 
        int32_t processData(softbit_t *v, uint32_t cnt, std::string Global_dumpFileName);
        
    protected:
        ProgrammeHandlerInterface& myProgrammeDataHandler;

    private:
        void    run(void);
        std::atomic<bool> runningData;
        
       std::vector<uint8_t> outDataV;
        std::vector<softbit_t> interleaveData[16];
        EnergyDispersal energyDispersal;

        std::condition_variable  mscDataDataAvailable;
        std::mutex               ourDataMutex;
        std::thread              ourDataThread;

        std::unique_ptr<Protection> protectionHandler;
        std::shared_ptr<DabDataProcessor> our_dabDataProcessor;
       


    	int64_t	serviceId;
    	// int		startAddr;
    	// int		Length;
    	// bool	shortForm;
    	// int		protLevel;
    	int16_t	bitDataRate;
    	int16_t	subChId;
        int16_t	appType;
    	std::string	serviceName;
        std::string	subName;
        uint16_t fragmentDataSize;
        uint32_t SId;
        int16_t DSCTy;
        uint16_t DGflag;
        int16_t	expectedIndex;
        std::vector<uint8_t>	series;
        // int16_t packetState;
        uint16_t subID ;
        RingBuffer<softbit_t> mscDataBuffer;
        std::string dumpFileName;

  

};

#endif

