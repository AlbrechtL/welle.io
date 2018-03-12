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
#ifndef FIB_PROCESSOR
#define FIB_PROCESSOR

#include    <stdint.h>
#include    <stdio.h>
#include    <vector>
#include    <QObject>
#include    "msc-handler.h"

struct dabLabel {
    //     uint8_t  label [17];
    QString  label;
    uint8_t  mask = 0x00;
    bool     hasName = false;
};


//  from FIG1/2
struct Service {
    uint32_t serviceId = 0;
    dabLabel serviceLabel;
    bool     hasPNum = false;
    bool     hasLanguage = false;
    int16_t  language = -1;
    int16_t  programType = 0;
    uint16_t pNum = 0;
};

//      The service component describes the actual service
//      It really should be a union
struct ServiceComponent {
    int8_t       TMid;           // the transport mode
    Service     *service;       // belongs to the service
    int16_t      componentNr;    // component

    int16_t      ASCTy;          // used for audio
    int16_t      PS_flag;        // use for both audio and packet
    int16_t      subchannelId;   // used in both audio and packet
    uint16_t     SCId;           // used in packet
    uint8_t      CAflag;         // used in packet (or not at all)
    int16_t      DSCTy;          // used in packet
    uint8_t      DGflag;     // used for TDC
    int16_t      packetAddress;  // used in packet
};

struct ChannelMap {
    int32_t  SubChId;
    int32_t  StartAddr;
    int32_t  Length;
    bool     shortForm;
    int32_t  protLevel;
    int32_t  BitRate;
    int16_t  language;
    int16_t  FEC_scheme;
};

class   CRadioController;

class   fib_processor: public QObject {
    Q_OBJECT
    public:
        fib_processor(CRadioController *);
        void    process_FIB(uint8_t *, uint16_t);

        void    setupforNewFrame(void);
        void    clearEnsemble(void);
        bool    syncReached(void);
        void    setSelectedService(QString &);
        uint8_t kindofService(QString &);
        void    dataforAudioService(QString &, audiodata *);
        void    dataforDataService(QString &, packetdata *);
    private:
        CRadioController *myRadioInterface;
        Service *findServiceId(uint32_t serviceId);
        ServiceComponent *find_packetComponent(int16_t SCId);

        void bind_audioService(
                int8_t TMid,
                uint32_t SId,
                int16_t compnr,
                int16_t subChId,
                int16_t ps_flag,
                int16_t ASCTy);

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

        int16_t  HandleFIG0Extension2(
                uint8_t *d,
                int16_t offset,
                uint8_t cn,
                uint8_t pd);

        int16_t HandleFIG0Extension3(uint8_t *d, int16_t used);
        int16_t HandleFIG0Extension5(uint8_t* d, int16_t offset);
        int16_t HandleFIG0Extension8(uint8_t *d, int16_t used, uint8_t pdBit);
        int16_t HandleFIG0Extension13(uint8_t *d, int16_t used, uint8_t pdBit);
        int16_t HandleFIG0Extension22(uint8_t *d, int16_t used);

        int32_t dateTime[8];
        ChannelMap ficList[64];
        std::vector<ServiceComponent> components;
        std::vector<Service> listofServices;
        bool        dateFlag;
        bool        firstTime;
        bool        isSynced;

    signals:
        void addtoEnsemble(quint32 SId, const QString& label);
        void nameofEnsemble(int SId, const QString& name);
        void changeinConfiguration(void);
        void newDateTime(int *dateTime);
};

#endif

