/*
 *    Copyright (C) 2020
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
#ifndef FIB_PROCESSOR
#define FIB_PROCESSOR

#include <vector>
#include <list>
#include <unordered_map>
#include <chrono>
#include <array>
#include <mutex>
#include <cstdint>
#include <cstdio>
#include "msc-handler.h"
#include "radio-controller.h"
#include "dab-constants.h"
#include "channels.h"


class FIBProcessor {
    public:
        FIBProcessor(RadioControllerInterface& mr);

        // called from the demodulator
        void processFIB(uint8_t *p, uint16_t fib);
        void clearEnsemble();

        // Called from the frontend
        uint32_t getEnsembleId() const;
        uint8_t getEnsembleEcc() const;
        DabLabel getEnsembleLabel() const;
        std::vector<Service> getServiceList() const;
        Service getService(uint32_t sId) const;
        std::list<ServiceComponent> getComponents(const Service& s) const;
        Subchannel getSubchannel(const ServiceComponent& sc) const;
        std::chrono::system_clock::time_point getTimeLastFCT0Frame() const;
        std::string announcements (uint8_t a);
        
        void getchannelInfo (channel_data *, int);

        // ##### EPG ######
        void set_epgData(uint32_t, int32_t, const std::string &, const std::string &);
        std::vector<epgElement> get_timeTable	(uint32_t);
	std::vector<epgElement> get_timeTableStr	( std::string &);
	bool	has_timeTable	(uint32_t SId);
	std::vector<epgElement>	find_epgData	(uint32_t);
        void	data_for_packetservice	(const std::string& , packetdata *, int16_t);
        // ##### EPG ######
        uint16_t findServiceIdPOSStr(const std::string &s);
        uint32_t findServiceIdSIdStr(const std::string &s);
        void get_parameters	(const std::string &s, uint32_t *p_SId, uint32_t *p_SCIds);
//        bool	syncReached();
        int findPacketComponentSCIdPOS(uint16_t SCId);
        uint32_t findServiceIdPOS(uint32_t serviceId);
    
        RadioControllerInterface& myRadioInterface;
        Service *findServiceId(uint32_t serviceId);
 
        double dateToJulianDate(int year, int month, int day);
        int	findPacketComponentSIDPOS( uint32_t SId);
        int	findPacketComponentSCId2SId( uint16_t SCId) ;
        int     findServiceComponent(uint32_t serviceId, int16_t SCIdS);
       // int	findComponentPOS(uint32_t SId, int16_t subChId);
        //	find serviceComponent using the SId 
 //       int	findComponentPOSSId	(uint32_t SId);
        ServiceComponent *findComponent(uint32_t serviceId, int16_t SCIdS);
        ServiceComponent *findPacketComponent(int16_t SCId);
       dab_date_time_t dateTime = {};
       std::vector<Service> services;
       std::vector<ServiceComponent> components;
// private:       

       

        void bindAudioService(
                int8_t TMid,
                uint32_t SId,
                int16_t compnr,
                int16_t subChId,
                int16_t ps_flag,
                int16_t ASCTy);

        void bindDataStreamService(
                int8_t TMid,
                uint32_t SId,
                int16_t compnr,
                int16_t subChId,
                int16_t ps_flag,
                int16_t DSCTy);

        void bindFIDCDataStreamService(
                int8_t TMid,
                uint32_t SId,
                int16_t compnr,
                int16_t subChId,
                int16_t ps_flag,
                int16_t DSCTy);                

        void bindPacketService(
                int8_t TMid,
                uint32_t SId,
                int16_t compnr,
                int16_t SCId,
                int16_t ps_flag,
                int16_t CAflag);

        void dropService(uint32_t SId);

        

        void process_FIG0(uint8_t *);
        void process_FIG1(uint8_t *);
        void process_FIG2(uint8_t *);
        void process_FIG5(uint8_t *); //TMC ??
        void process_FIG7(uint8_t *);

        void FIG0Extension0(uint8_t *);
        void FIG0Extension1(uint8_t *);
        void FIG0Extension2(uint8_t *);
        void FIG0Extension3(uint8_t *);
        void FIG0Extension4(uint8_t *);
        void FIG0Extension5(uint8_t *);
        void FIG0Extension6(uint8_t *);
        void FIG0Extension7(uint8_t *);
        void FIG0Extension8(uint8_t *);
        void FIG0Extension9(uint8_t *);
        void FIG0Extension10(uint8_t *);
        void FIG0Extension11(uint8_t *);
        void FIG0Extension13(uint8_t *);
        void FIG0Extension14(uint8_t *);
        void FIG0Extension16(uint8_t *);
        void FIG0Extension17(uint8_t *);
        void FIG0Extension18(uint8_t *);
        void FIG0Extension19(uint8_t *);
        void FIG0Extension20(uint8_t *);
        void FIG0Extension21(uint8_t *);
        void FIG0Extension22(uint8_t *);
        void FIG0Extension24(uint8_t *);

        void FIG5Extension1(uint8_t *);

        int16_t HandleFIG0Extension1(uint8_t *d, int16_t offset, uint8_t pd);

        int16_t HandleFIG0Extension2(
                uint8_t *d,
                int16_t offset,
                uint8_t cn,
                uint8_t pd);

        int16_t HandleFIG0Extension3(uint8_t *d, int16_t used);
        int16_t HandleFIG0Extension4(uint8_t *d, int16_t used);
        int16_t HandleFIG0Extension5(uint8_t *d, int16_t offset);
        int16_t HandleFIG0Extension8(uint8_t *d, int16_t used, uint8_t pdBit);
        int16_t HandleFIG0Extension13(uint8_t *d, int16_t used, uint8_t pdBit);
        int16_t	HandleFIG0Extension21(uint8_t* d, uint8_t, uint8_t, uint8_t, int16_t);
        int16_t HandleFIG0Extension22(uint8_t *d, int16_t used);

        int16_t HandleFIG5Extension1(uint8_t *d, int16_t used, uint8_t pdBit);

        void	createService (const std::string &name, const std::string &shortName, uint32_t SId, int SCIds);

        bool timeOffsetReceived = false;
        
        mutable std::mutex mutex;
        uint16_t ensembleId = 0;
        uint8_t ensembleEcc = 0;
        DabLabel ensembleLabel;
        std::vector<Subchannel> subChannels;
        
        //std::vector<Service> services;
        std::unordered_map<uint32_t, uint8_t> serviceRepeatCount;
        std::chrono::steady_clock::time_point timeLastServiceDecrement;
        std::chrono::system_clock::time_point timeLastFCT0Frame;

	void		setCluster		(int16_t, int16_t, uint16_t);
	Cluster		*getCluster		( int16_t);

        // ##################### DAB-Frequenzliste mit Daten füllen
        DABFrequenzData DABFrequenzDatas;
        ClusterConfig   ClusterConfigs;
        Channels channels;
        
        
};
	

#endif

