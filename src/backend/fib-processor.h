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
#ifndef FIB_PROCESSOR
#define FIB_PROCESSOR

#include <vector>
#include <list>
#include <array>
#include <mutex>
#include <cstdint>
#include <cstdio>
#include "msc-handler.h"
#include "radio-controller.h"

class FIBProcessor {
    public:
        FIBProcessor(RadioControllerInterface& mr);

        // called from the demodulator
        void process_FIB(uint8_t *p, uint16_t fib);
        void clearEnsemble();
        bool syncReached();

        // Called from the frontend
        uint16_t getEnsembleId() const;
        uint8_t getEnsembleEcc() const;
        DabLabel getEnsembleLabel() const;
        std::vector<Service> getServiceList() const;
        std::list<ServiceComponent> getComponents(const Service& s) const;
        Subchannel getSubchannel(const ServiceComponent& sc) const;

    private:
        RadioControllerInterface& myRadioInterface;
        Service *findServiceId(uint32_t serviceId);
        ServiceComponent *findComponent(uint32_t serviceId, int16_t SCIdS);
        ServiceComponent *findPacketComponent(int16_t SCId);

        void bind_audioService(
                int8_t TMid,
                uint32_t SId,
                int16_t compnr,
                int16_t subChId,
                int16_t ps_flag,
                int16_t ASCTy);

        void bind_dataStreamService(
                int8_t TMid,
                uint32_t SId,
                int16_t compnr,
                int16_t subChId,
                int16_t ps_flag,
                int16_t DSCTy);

        void bind_packetService(
                int8_t TMid,
                uint32_t SId,
                int16_t compnr,
                int16_t SCId,
                int16_t ps_flag,
                int16_t CAflag);

        void process_FIG0(uint8_t *);
        void process_FIG1(uint8_t *);
        void FIG0Extension0(uint8_t *);
        void FIG0Extension1(uint8_t *);
        void FIG0Extension2(uint8_t *);
        void FIG0Extension3(uint8_t *);
        void FIG0Extension5(uint8_t *);
        void FIG0Extension8(uint8_t *);
        void FIG0Extension9(uint8_t *);
        void FIG0Extension10(uint8_t *);
        void FIG0Extension13(uint8_t *);
        void FIG0Extension14(uint8_t *);
        void FIG0Extension16(uint8_t *);
        void FIG0Extension17(uint8_t *);
        void FIG0Extension18(uint8_t *);
        void FIG0Extension19(uint8_t *);
        void FIG0Extension21(uint8_t *);
        void FIG0Extension22(uint8_t *);

        int16_t HandleFIG0Extension1(uint8_t *d, int16_t offset, uint8_t pd);

        int16_t HandleFIG0Extension2(
                uint8_t *d,
                int16_t offset,
                uint8_t cn,
                uint8_t pd);

        int16_t HandleFIG0Extension3(uint8_t *d, int16_t used);
        int16_t HandleFIG0Extension5(uint8_t *d, int16_t offset);
        int16_t HandleFIG0Extension8(uint8_t *d, int16_t used, uint8_t pdBit);
        int16_t HandleFIG0Extension13(uint8_t *d, int16_t used, uint8_t pdBit);
        int16_t HandleFIG0Extension22(uint8_t *d, int16_t used);

        bool timeOffsetReceived = false;
        dab_date_time_t dateTime = {};
        mutable std::mutex mutex;
        uint16_t ensembleId = 0;
        uint8_t ensembleEcc = 0;
        DabLabel ensembleLabel;
        std::vector<Subchannel> subChannels;
        std::vector<ServiceComponent> components;
        std::vector<Service> services;
        bool firstTime = true;
        bool isSynced = false;
};

#endif

