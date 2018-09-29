/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2014
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
 *  fib and fig processor
 */
#include <iostream>
#include <algorithm>
#include <cstring>

#include "fib-processor.h"
#include "charsets.h"
#include "MathHelper.h"

FIBProcessor::FIBProcessor(RadioControllerInterface& mr) :
    myRadioInterface(mr)
{
    clearEnsemble();
}

//  FIB's are segments of 256 bits. When here, we already
//  passed the crc and we start unpacking into FIGs
//  This is merely a dispatcher
void FIBProcessor::process_FIB(uint8_t *p, uint16_t fib)
{
    int8_t  processedBytes  = 0;
    uint8_t *d = p;

    std::lock_guard<std::mutex> lock(mutex);

    (void)fib;
    while (processedBytes  < 30) {
        const uint8_t FIGtype = getBits_3 (d, 0);
        switch (FIGtype) {
            case 0:
                process_FIG0 (d);
                break;

            case 1:
                process_FIG1 (d);
                break;

            case 7:
                return;

            default:
                //std::clog << "FIG%d present" << FIGtype << std::endl;
                break;
        }
        //  Thanks to Ronny Kunze, who discovered that I used
        //  a p rather than a d
        processedBytes += getBits_5 (d, 3) + 1;
        d = p + processedBytes * 8;
    }
}
//
//  Handle ensemble is all through FIG0
//
void FIBProcessor::process_FIG0 (uint8_t *d)
{
    uint8_t extension   = getBits_5 (d, 8 + 3);
    //uint8_t   CN  = getBits_1 (d, 8 + 0);

    switch (extension) {
        case 0: FIG0Extension0 (d); break;
        case 1: FIG0Extension1 (d); break;
        case 2: FIG0Extension2 (d); break;
        case 3: FIG0Extension3 (d); break;
        case 5: FIG0Extension5 (d); break;
        case 8: FIG0Extension8 (d); break;
        case 9: FIG0Extension9 (d); break;
        case 10: FIG0Extension10 (d); break;
        case 14: FIG0Extension14 (d); break;
        case 13: FIG0Extension13 (d); break;
        case 16: FIG0Extension16 (d); break;
        case 17: FIG0Extension17 (d); break;
        case 18: FIG0Extension18 (d); break;
        case 19: FIG0Extension19 (d); break;
        case 21: FIG0Extension21 (d); break;
        case 22: FIG0Extension22 (d); break;
        default:
            //        std::clog << "fib-processor:" << "FIG0/%d passed by\n", extension) << std::endl;
            break;
    }
}


//  FOG0/0 indicated a change in channel organization
//  we are not equipped for that, so we just return
//  control to the init
void FIBProcessor::FIG0Extension0 (uint8_t *d)
{
    uint8_t     changeflag;
    uint16_t    highpart, lowpart;
    int16_t     occurrenceChange;
    uint8_t CN  = getBits_1 (d, 8 + 0);
    (void)CN;

    ensembleId  = getBits(d, 16, 16);

    changeflag  = getBits_2 (d, 16 + 16);
    if (changeflag == 0)
        return;

    highpart        = getBits_5 (d, 16 + 19) % 20;
    (void)highpart;
    lowpart         = getBits_8 (d, 16 + 24) % 250;
    (void)lowpart;
    occurrenceChange    = getBits_8 (d, 16 + 32);
    (void)occurrenceChange;

    //  if (changeflag == 1) {
    //     std::clog << "fib-processor:" << "Changes in sub channel organization\n") << std::endl;
    //     std::clog << "fib-processor:" << "cifcount = %d\n", highpart * 250 + lowpart) << std::endl;
    //     std::clog << "fib-processor:" << "Change happening in %d CIFs\n", occurrenceChange) << std::endl;
    //  }
    //  else if (changeflag == 3) {
    //     std::clog << "fib-processor:" << "Changes in subchannel and service organization\n") << std::endl;
    //     std::clog << "fib-processor:" << "cifcount = %d\n", highpart * 250 + lowpart) << std::endl;
    //     std::clog << "fib-processor:" << "Change happening in %d CIFs\n", occurrenceChange) << std::endl;
    //  }
    std::clog << "fib-processor: " << "changes in config not supported, choose again" << std::endl;
    // Were, the signal ensembleChanged was called, which was ignored by the
    // frontend
}

//  FIG0 extension 1 creates a mapping between the
//  sub channel identifications and the positions in the
//  relevant CIF.
void FIBProcessor::FIG0Extension1 (uint8_t *d)
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);
    //uint8_t   CN  = getBits_1 (d, 8 + 0);

    while (used < Length - 1)
        used = HandleFIG0Extension1 (d, used, PD_bit);
}

//  defining the channels
int16_t FIBProcessor::HandleFIG0Extension1(
        uint8_t *d,
        int16_t offset,
        uint8_t pd)
{
    int16_t bitOffset = offset * 8;
    const int16_t subChId   = getBits_6 (d, bitOffset);
    const int16_t startAdr  = getBits(d, bitOffset + 6, 10);
    subChannels[subChId].programmeNotData = pd;
    subChannels[subChId].subChId = subChId;
    subChannels[subChId].startAddr = startAdr;
    if (getBits_1 (d, bitOffset + 16) == 0) {   // UEP, short form
        int16_t tableIx = getBits_6 (d, bitOffset + 18);
        auto& ps = subChannels[subChId].protectionSettings;
        ps.uepTableIndex = tableIx;
        ps.shortForm = true;
        ps.uepLevel = ProtLevel[tableIx][1];

        subChannels[subChId].length = ProtLevel[tableIx][0];
        bitOffset += 24;
    }
    else {  // EEP, long form
        auto& ps = subChannels[subChId].protectionSettings;
        ps.shortForm  = false;
        int16_t option = getBits_3(d, bitOffset + 17);
        if (option == 0) {
            ps.eepProfile = EEPProtectionProfile::EEP_A;
        }
        else if (option == 1) {
            ps.eepProfile = EEPProtectionProfile::EEP_B;
        }

        if (option == 0 or   // EEP-A protection
            option == 1) {   // EEP-B protection
            int16_t protLevel = getBits_2(d, bitOffset + 20);
            switch (protLevel) {
                case 0:
                    ps.eepLevel = EEPProtectionLevel::EEP_1;
                    break;
                case 1:
                    ps.eepLevel = EEPProtectionLevel::EEP_2;
                    break;
                case 2:
                    ps.eepLevel = EEPProtectionLevel::EEP_3;
                    break;
                case 3:
                    ps.eepLevel = EEPProtectionLevel::EEP_4;
                    break;
                default:
                    std::clog << "Warning, FIG0/1 for " << subChId <<
                        " has invalid EEP protection level " << protLevel <<
                        std::endl;
                    break;
            }

            int16_t subChanSize = getBits(d, bitOffset + 22, 10);
            subChannels[subChId].length = subChanSize;
        }
        else {
            std::clog << "Warning, FIG0/1 for " << subChId <<
                " has invalid protection option " << option << std::endl;
        }

        bitOffset += 32;
    }

    return bitOffset / 8;   // we return bytes
}

void FIBProcessor::FIG0Extension2 (uint8_t *d)
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);
    uint8_t CN      = getBits_1 (d, 8 + 0);

    while (used < Length) {
        used = HandleFIG0Extension2 (d, used, CN, PD_bit);
    }
}

//  Note Offset is in bytes
//  With FIG0/2 we bind the channels to Service Ids
int16_t FIBProcessor::HandleFIG0Extension2(
        uint8_t *d,
        int16_t offset,
        uint8_t cn,
        uint8_t pd)
{
    (void) cn;
    int16_t     lOffset = 8 * offset;
    int16_t     i;
    uint8_t     ecc;
    uint8_t     cId;
    uint32_t    SId;
    int16_t     numberofComponents;

    if (pd == 1) {      // long Sid
        ecc = getBits_8 (d, lOffset);   (void)ecc;
        cId = getBits_4 (d, lOffset + 1);
        SId = getBits(d, lOffset, 32);
        lOffset += 32;
    }
    else {
        cId = getBits_4 (d, lOffset);   (void)cId;
        SId = getBits (d, lOffset + 4, 12);
        SId = getBits (d, lOffset, 16);
        lOffset += 16;
    }

    numberofComponents  = getBits_4 (d, lOffset + 4);
    lOffset += 8;

    for (i = 0; i < numberofComponents; i ++) {
        uint8_t TMid    = getBits_2 (d, lOffset);
        if (TMid == 00)  {  // Audio
            uint8_t ASCTy   = getBits_6 (d, lOffset + 2);
            uint8_t SubChId = getBits_6 (d, lOffset + 8);
            uint8_t PS_flag = getBits_1 (d, lOffset + 14);
            bind_audioService(TMid, SId, i, SubChId, PS_flag, ASCTy);
        }
        else if (TMid == 1) { // MSC stream data
            uint8_t DSCTy   = getBits_6 (d, lOffset + 2);
            uint8_t SubChId = getBits_6 (d, lOffset + 8);
            uint8_t PS_flag = getBits_1 (d, lOffset + 14);
            bind_dataStreamService(TMid, SId, i, SubChId, PS_flag, DSCTy);
        }
        else if (TMid == 3) { // MSC packet data
            int16_t SCId    = getBits (d, lOffset + 2, 12);
            uint8_t PS_flag = getBits_1 (d, lOffset + 14);
            uint8_t CA_flag = getBits_1 (d, lOffset + 15);
            bind_packetService(TMid, SId, i, SCId, PS_flag, CA_flag);
        }
        else {
            // reserved
        }
        lOffset += 16;
    }
    return lOffset / 8;     // in Bytes
}

//      The Extension 3 of FIG type 0 (FIG 0/3) gives
//      additional information about the service component
//      description in packet mode.
//      manual: page 55
void FIBProcessor::FIG0Extension3 (uint8_t *d)
{
    int16_t used    = 2;
    int16_t Length  = getBits_5 (d, 3);

    while (used < Length)
        used = HandleFIG0Extension3 (d, used);
}

//      DSCTy   DataService Component Type
int16_t FIBProcessor::HandleFIG0Extension3(uint8_t *d, int16_t used)
{
    int16_t SCId            = getBits (d, used * 8, 12);
    //int16_t CAOrgflag       = getBits_1 (d, used * 8 + 15);
    int16_t DGflag          = getBits_1 (d, used * 8 + 16);
    int16_t DSCTy           = getBits_6 (d, used * 8 + 18);
    int16_t SubChId         = getBits_6 (d, used * 8 + 24);
    int16_t packetAddress   = getBits (d, used * 8 + 30, 10);
    //uint16_t        CAOrg   = getBits (d, used * 8 + 40, 16);

    ServiceComponent *packetComp = findPacketComponent(SCId);

    used += 56 / 8;
    if (packetComp == NULL)     // no ServiceComponent yet
        return used;
    packetComp->subchannelId = SubChId;
    packetComp->DSCTy = DSCTy;
    packetComp->DGflag = DGflag;
    packetComp->packetAddress = packetAddress;
    return used;
}

void FIBProcessor::FIG0Extension5 (uint8_t *d)
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);

    while (used < Length) {
        used = HandleFIG0Extension5 (d, used);
    }
}

int16_t FIBProcessor::HandleFIG0Extension5(uint8_t* d, int16_t offset)
{
    int16_t loffset = offset * 8;
    uint8_t lsFlag  = getBits_1 (d, loffset);
    int16_t subChId, serviceComp, language;

    if (lsFlag == 0) {  // short form
        if (getBits_1 (d, loffset + 1) == 0) {
            subChId = getBits_6 (d, loffset + 2);
            language = getBits_8 (d, loffset + 8);
            subChannels[subChId].language = language;
        }
        loffset += 16;
    }
    else {          // long form
        serviceComp = getBits (d, loffset + 4, 12);
        language    = getBits_8 (d, loffset + 16);
        loffset += 24;
    }
    (void)serviceComp;

    return loffset / 8;
}

void FIBProcessor::FIG0Extension8 (uint8_t *d)
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);

    while (used < Length) {
        used = HandleFIG0Extension8 (d, used, PD_bit);
    }
}

int16_t FIBProcessor::HandleFIG0Extension8(
        uint8_t *d,
        int16_t used,
        uint8_t pdBit)
{
    int16_t  lOffset = used * 8;
    uint32_t SId = getBits(d, lOffset, pdBit == 1 ? 32 : 16);
    uint8_t  lsFlag;
    uint16_t SCIds;
    int16_t  SCid;
    int16_t  MSCflag;
    int16_t  SubChId;
    uint8_t  extensionFlag;

    lOffset += pdBit == 1 ? 32 : 16;
    extensionFlag   = getBits_1 (d, lOffset);
    SCIds   = getBits_4 (d, lOffset + 4);
    lOffset += 8;

    lsFlag  = getBits_1 (d, lOffset + 8);
    if (lsFlag == 1) {
        SCid = getBits (d, lOffset + 4, 12);
        lOffset += 16;
        //           if (findPacketComponent ((SCIds << 4) | SCid) != NULL) {
        //              std::clog << "fib-processor:" << "packet component bestaat !!\n") << std::endl;
        //           }
    }
    else {
        MSCflag = getBits_1 (d, lOffset + 1);
        SubChId = getBits_6 (d, lOffset + 2);
        lOffset += 8;
    }
    if (extensionFlag)
        lOffset += 8;   // skip Rfa
    (void)SId;
    (void)SCIds;
    (void)SCid;
    (void)SubChId;
    (void)MSCflag;
    return lOffset / 8;
}

//  FIG0/9 and FIG0/10 are copied from the work of
//  Michael Hoehn
void FIBProcessor::FIG0Extension9(uint8_t *d)
{
    int16_t offset  = 16;

    dateTime.hourOffset = (getBits_1 (d, offset + 2) == 1) ?
        -1 * getBits_4 (d, offset + 3):
        getBits_4 (d, offset + 3);
    dateTime.minuteOffset = (getBits_1 (d, offset + 7) == 1) ? 30 : 0;
    timeOffsetReceived = true;

    ensembleEcc = getBits(d, offset + 8, 8);
}

void FIBProcessor::FIG0Extension10(uint8_t *fig)
{
    int16_t     offset = 16;
    int32_t     mjd = getBits(fig, offset + 1, 17);
    // Convert Modified Julian Date (according to wikipedia)
    int32_t J   = mjd + 2400001;
    int32_t j   = J + 32044;
    int32_t g   = j / 146097;
    int32_t dg  = j % 146097;
    int32_t c   = ((dg / 36524) + 1) * 3 / 4;
    int32_t dc  = dg - c * 36524;
    int32_t b   = dc / 1461;
    int32_t db  = dc%1461;
    int32_t a   = ((db / 365) + 1) * 3 / 4;
    int32_t da  = db - a * 365;
    int32_t y   = g * 400 + c * 100 + b * 4 + a;
    int32_t m   = ((da * 5 + 308) / 153) - 2;
    int32_t d   = da - ((m + 4) * 153 / 5) + 122;
    int32_t Y   = y - 4800 + ((m + 2) / 12);
    int32_t M   = ((m + 2) % 12) + 1;
    int32_t D   = d + 1;

    dateTime.year = Y;
    dateTime.month = M;
    dateTime.day = D;
    dateTime.hour = getBits_5(fig, offset + 21);
    if (getBits_6(fig, offset + 26) != dateTime.minutes)
        dateTime.seconds =  0;  // handle overflow

    dateTime.minutes = getBits_6(fig, offset + 26);
    if (fig [offset + 20] == 1) {
        dateTime.seconds = getBits_6(fig, offset + 32);
    }

    if (timeOffsetReceived) {
        myRadioInterface.onDateTimeUpdate(dateTime);
    }
}

void FIBProcessor::FIG0Extension13 (uint8_t *d)
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);

    while (used < Length) {
        used = HandleFIG0Extension13 (d, used, PD_bit);
    }
}

int16_t FIBProcessor::HandleFIG0Extension13(
        uint8_t *d,
        int16_t used,
        uint8_t pdBit)
{
    int16_t  lOffset = used * 8;
    uint32_t SId = getBits(d, lOffset, pdBit == 1 ? 32 : 16);
    uint16_t SCIds;
    int16_t  NoApplications;
    int16_t  i;

    lOffset     += pdBit == 1 ? 32 : 16;
    SCIds       = getBits_4 (d, lOffset);
    NoApplications = getBits_4 (d, lOffset + 4);
    lOffset += 8;

    for (i = 0; i < NoApplications; i++) {
        int16_t appType = getBits (d, lOffset, 11);
        int16_t length  = getBits_5 (d, lOffset + 11);
        lOffset += (11 + 5 + 8 * length);
        switch (appType) {
            case 0x000:     // reserved for future use
            case 0x001:     // not used
                break;

            case 0x002:     // MOT slideshow
            case 0x003:     // MOT Broadcast Web Site
            case 0x004:     // TPEG
            case 0x005:     // DGPS
            case 0x006:     // TMC
            case 0x007:     // EPG
            case 0x008:     // DAB Java
                break;

            case 0x44a:     // Journaline
                //           std::clog << "fib-processor:" << "Journaline\n") << std::endl;
                break;

            default:
                break;
        }
    }

    (void)SId;
    (void)SCIds;
    return lOffset / 8;
}

void FIBProcessor::FIG0Extension14 (uint8_t *d)
{
    int16_t length = getBits_5 (d, 3); // in Bytes
    int16_t used   = 2; // in Bytes

    while (used < length) {
        int16_t subChId = getBits_6 (d, used * 8);
        uint8_t fecScheme = getBits_2 (d, used * 8 + 6);
        used = used + 1;

        for (int i = 0; i < 64; i++) {
            if (subChannels[i].subChId == subChId) {
                subChannels[i].fecScheme = fecScheme;
            }
        }

    }
}

void FIBProcessor::FIG0Extension16 (uint8_t *d)
{
    int16_t length = getBits_5 (d, 3); // in bytes
    int16_t offset = 16;           // in bits
    Service *s;

    while (offset < length * 8) {
        uint16_t    SId = getBits (d, offset, 16);
        s   = findServiceId (SId);
        if (!s -> hasPNum) {
            uint8_t PNum = getBits (d, offset + 16, 16);
            s -> pNum       = PNum;
            s -> hasPNum    = true;
            //        std::clog << "fib-processor:" << "Program number info SId = %.8X, PNum = %d\n",
            //                                         SId, PNum) << std::endl;
        }
        offset += 72;
    }
}

void FIBProcessor::FIG0Extension17(uint8_t *d)
{
    int16_t length  = getBits_5 (d, 3);
    int16_t offset  = 16;
    Service *s;

    while (offset < length * 8) {
        uint16_t    SId = getBits (d, offset, 16);
        bool    L_flag  = getBits_1 (d, offset + 18);
        bool    CC_flag = getBits_1 (d, offset + 19);
        int16_t type;
        int16_t Language = 0x00;    // init with unknown language
        s = findServiceId (SId);
        if (L_flag) {       // language field present
            Language = getBits_8 (d, offset + 24);
            s->language = Language;
            offset += 8;
        }

        type = getBits_5 (d, offset + 27);
        s->programType = type;
        if (CC_flag) {          // cc flag
            offset += 40;
        }
        else {
            offset += 32;
        }
    }
}

void FIBProcessor::FIG0Extension18(uint8_t *d)
{
    int16_t  offset  = 16;       // bits
    uint16_t SId, AsuFlags;
    int16_t  Length  = getBits_5 (d, 3);

    while (offset / 8 < Length - 1 ) {
        int16_t NumClusters = getBits_5 (d, offset + 35);
        SId = getBits (d, offset, 16);
        AsuFlags = getBits (d, offset + 16, 16);
        //     std::clog << "fib-processor:" << "Announcement %d for SId %d with %d clusters\n",
        //                      AsuFlags, SId, NumClusters) << std::endl;
        offset += 40 + NumClusters * 8;
    }
    (void)SId;
    (void)AsuFlags;
}

void FIBProcessor::FIG0Extension19(uint8_t *d)
{
    int16_t  offset  = 16;       // bits
    int16_t  Length  = getBits_5 (d, 3);
    uint8_t  region_Id_Lower;

    while (offset / 8 < Length - 1) {
        uint8_t clusterId   = getBits_8 (d, offset);
        bool    new_flag    = getBits_1(d, offset + 24);
        bool    region_flag = getBits_1 (d, offset + 25);
        uint8_t subChId     = getBits_6 (d, offset + 26);

        uint16_t aswFlags = getBits (d, offset + 8, 16);
        //     std::clog << "fib-processor:" <<
        //            "%s %s Announcement %d for Cluster %2u on SubCh %2u ",
        //                ((new_flag==1)?"new":"old"),
        //                ((region_flag==1)?"regional":""),
        //                aswFlags, clusterId,subChId) << std::endl;
        if (region_flag) {
            region_Id_Lower = getBits_6 (d, offset + 34);
            offset += 40;
            //           fprintf(stderr,"for region %u",region_Id_Lower);
        }
        else {
            offset += 32;
        }

        //     fprintf(stderr,"\n");
        (void)clusterId;
        (void)new_flag;
        (void)subChId;
        (void)aswFlags;
    }
    (void)region_Id_Lower;
}

void FIBProcessor::FIG0Extension21(uint8_t *d)
{
    //  std::clog << "fib-processor:" << "Frequency information\n") << std::endl;
    (void)d;
}

void FIBProcessor::FIG0Extension22(uint8_t *d)
{
    int16_t Length  = getBits_5 (d, 3);
    int16_t offset  = 16;       // on bits
    int16_t used    = 2;

    while (used < Length) {
        used = HandleFIG0Extension22 (d, used);
    }
    (void)offset;
}

int16_t FIBProcessor::HandleFIG0Extension22(uint8_t *d, int16_t used)
{
    uint8_t MS;
    int16_t mainId;
    int16_t noSubfields;

    mainId  = getBits_7 (d, used * 8 + 1);
    (void)mainId;
    MS  = getBits_1 (d, used * 8);
    if (MS == 0) {      // fixed size
        int16_t latitudeCoarse = getBits (d, used * 8 + 8, 16);
        int16_t longitudeCoarse = getBits (d, used * 8 + 24, 16);
        //     std::clog << "fib-processor:" << "Id = %d, (%d %d)\n", mainId,
        //                                latitudeCoarse, longitudeCoarse) << std::endl;
        (void)latitudeCoarse;
        (void)longitudeCoarse;
        return used + 48 / 6;
    }
    //  MS == 1

    noSubfields = getBits_3 (d, used * 8 + 13);
    //  std::clog << "fib-processor:" << "Id = %d, subfields = %d\n", mainId, noSubfields) << std::endl;
    used += (16 + noSubfields * 48) / 8;

    return used;
}

//  FIG 1
//
void    FIBProcessor::process_FIG1 (uint8_t *d)
{
    uint8_t     charSet, extension;
    uint32_t    SId = 0;
    uint8_t     oe;
    int16_t     offset  = 0;
    Service    *service;
    ServiceComponent *component;
    int16_t     i;
    uint8_t     pd_flag;
    uint8_t     SCidS;
    uint8_t     XPAD_aid;
    //uint8_t       region_id;
    char        label [17];
    //
    //  from byte 1 we deduce:
    charSet     = getBits_4 (d, 8);
    oe      = getBits_1 (d, 8 + 4);
    extension   = getBits_3 (d, 8 + 5);
    label [16]  = 0x00;
    if (oe == 1) {
        return;
    }

    switch (extension) {
        case 0: // ensemble label
            {
                uint32_t EId = getBits(d, 16, 16);
                (void)EId;
            }
            offset  = 32;
            if ((charSet <= 16)) { // EBU Latin based repertoire
                for (i = 0; i < 16; i ++) {
                    label[i] = getBits_8 (d, offset);
                    offset += 8;
                }
                //           std::clog << "fib-processor:" << "Ensemblename: %16s\n", label) << std::endl;
                if (!oe) {
                    if (firstTime) {
                        ensembleLabel.flag = getBits(d, offset, 16);
                        ensembleLabel.raw_label = label;
                        ensembleLabel.setCharset(charSet);

                        myRadioInterface.onNewEnsembleName(
                                toUtf8StringUsingCharset(
                                    (const char *)label,
                                    (CharacterSet)charSet));
                    }
                    firstTime = false;
                    isSynced  = true;
                }
            }
            //        std::clog << "fib-processor:" <<
            //                 "charset %d is used for ensemblename\n", charSet) << std::endl;
            break;

        case 1: // 16 bit Identifier field for service label
            SId = getBits(d, 16, 16);
            offset  = 32;
            service = findServiceId(SId);
            if (service->serviceLabel.raw_label.empty() && charSet <= 16) {
                for (i = 0; i < 16; i++) {
                    label[i] = getBits_8(d, offset);
                    offset += 8;
                }
                service->serviceLabel.flag = getBits(d, offset, 16);
                service->serviceLabel.raw_label = label;
                service->serviceLabel.setCharset(charSet);

                // std::clog << "fib-processor:" << "FIG1/1: SId = %4x\t%s\n", SId, label) << std::endl;
                myRadioInterface.onServiceDetected(SId,
                        toUtf8StringUsingCharset(
                            (const char *)label, (CharacterSet) charSet));
            }
            break;

        case 3:
            // region label
            //        region_id = getBits_6 (d, 16 + 2);
            offset = 24;
            for (i = 0; i < 16; i ++) {
                label[i] = getBits_8 (d, offset + 8 * i);
            }

            //        std::clog << "fib-processor:" << "FIG1/3: RegionID = %2x\t%s\n", region_id, label) << std::endl;
            break;

        case 4:
            pd_flag = getBits(d, 16, 1);
            SCidS   = getBits(d, 20, 4);
            if (pd_flag) {  // 32 bit identifier field for service component label
                SId = getBits(d, 24, 32);
                offset  = 56;
            }
            else {  // 16 bit identifier field for service component label
                SId = getBits(d, 24, 16);
                offset  = 40;
            }

            for (i = 0; i < 16; i ++) {
                label[i] = getBits_8 (d, offset);
                offset += 8;
            }

            component = findComponent(SId, SCidS);
            if (component) {
                component->componentLabel.flag = getBits(d, offset, 16);
                component->componentLabel.setCharset(charSet);
                component->componentLabel.raw_label = label;
            }
            //        std::clog << "fib-processor:" << "FIG1/4: Sid = %8x\tp/d=%d\tSCidS=%1X\tflag=%8X\t%s\n",
            //                          SId, pd_flag, SCidS, flagfield, label) << std::endl;
            break;


        case 5: // 32 bit Identifier field for service label
            SId = getBits(d, 16, 32);
            offset  = 48;
            service = findServiceId(SId);
            if (service->serviceLabel.raw_label.empty() && charSet <= 16) {
                for (i = 0; i < 16; i ++) {
                    label[i] = getBits_8(d, offset);
                    offset += 8;
                }
                service->serviceLabel.flag = getBits(d, offset, 16);
                service->serviceLabel.raw_label = label;
                service->serviceLabel.setCharset(charSet);

#ifdef  MSC_DATA__
                string l = toUtf8StringUsingCharset(
                        (const char *)label, (CharacterSet)charSet);
                l += " (data)";
                myRadioInterface.onServiceDetected(SId, l);
#endif
            }
            break;

        case 6: // XPAD label
            pd_flag = getBits(d, 16, 1);
            SCidS   = getBits(d, 20, 4);
            if (pd_flag) {  // 32 bits identifier for XPAD label
                SId       = getBits(d, 24, 32);
                XPAD_aid  = getBits(d, 59, 5);
                offset    = 64;
            }
            else {  // 16 bit identifier for XPAD label
                SId       = getBits(d, 24, 16);
                XPAD_aid  = getBits(d, 43, 5);
                offset    = 48;
            }

            for (i = 0; i < 16; i ++) {
                label[i] = getBits_8 (d, offset + 8 * i);
            }

            //        std::clog << "fib-processor:" << "FIG1/6: SId = %8x\tp/d = %d\t SCidS = %1X\tXPAD_aid = %2u\t%s\n",
            //             SId, pd_flag, SCidS, XPAD_aid, label) << std::endl;
            break;

        default:
            //        std::clog << "fib-processor:" << "FIG1/%d: not handled now\n", extension) << std::endl;
            break;
    }
    (void)SCidS;
    (void)XPAD_aid;
}

//  locate - and create if needed - a reference to the entry
//  for the Service serviceId
Service *FIBProcessor::findServiceId(uint32_t serviceId)
{
    for (size_t i = 0; i < services.size(); i++) {
        if (services[i].serviceId == serviceId) {
            return &services[i];
        }
    }

    Service serv;
    serv.serviceId = serviceId;
    services.push_back(serv);
    return &services.back();
}

ServiceComponent *FIBProcessor::findComponent(uint32_t serviceId, int16_t SCIdS)
{
    auto comp = std::find_if(components.begin(), components.end(),
                [&](const ServiceComponent& sc) {
                    return sc.SId == serviceId && sc.componentNr == SCIdS;
                });

    if (comp == components.end()) {
        return nullptr;
    }
    else {
        return &(*comp);
    }
}

ServiceComponent *FIBProcessor::findPacketComponent(int16_t SCId)
{
    for (auto& component : components) {
        if (component.TMid != 03) {
            continue;
        }
        if (component.SCId == SCId) {
            return &component;
        }
    }
    return nullptr;
}

//  bind_audioService is the main processor for - what the name suggests -
//  connecting the description of audioservices to a SID
void FIBProcessor::bind_audioService(
        int8_t TMid,
        uint32_t SId,
        int16_t compnr,
        int16_t subChId,
        int16_t ps_flag,
        int16_t ASCTy)
{
    Service *s = findServiceId(SId);

    if (std::find_if(components.begin(), components.end(),
                [&](const ServiceComponent& sc) {
                    return sc.SId == s->serviceId && sc.componentNr == compnr;
                }) == components.end()) {
        ServiceComponent newcomp;
        newcomp.TMid         = TMid;
        newcomp.componentNr  = compnr;
        newcomp.SId          = SId;
        newcomp.subchannelId = subChId;
        newcomp.PS_flag      = ps_flag;
        newcomp.ASCTy        = ASCTy;
        components.push_back(newcomp);

        //  std::clog << "fib-processor:" << "service %8x (comp %d) is audio\n", SId, compnr) << std::endl;
    }
}

void FIBProcessor::bind_dataStreamService(
        int8_t TMid,
        uint32_t SId,
        int16_t compnr,
        int16_t subChId,
        int16_t ps_flag,
        int16_t DSCTy)
{
    Service *s = findServiceId (SId);
    if (std::find_if(components.begin(), components.end(),
                [&](const ServiceComponent& sc) {
                    return sc.SId == s->serviceId && sc.componentNr == compnr;
                }) == components.end()) {
        ServiceComponent newcomp;
        newcomp.TMid         = TMid;
        newcomp.SId          = SId;
        newcomp.subchannelId = subChId;
        newcomp.componentNr  = compnr;
        newcomp.PS_flag      = ps_flag;
        newcomp.DSCTy        = DSCTy;
        components.push_back(newcomp);

        //  std::clog << "fib-processor:" << "service %8x (comp %d) is packet\n", SId, compnr) << std::endl;
    }
}

//      bind_packetService is the main processor for - what the name suggests -
//      connecting the service component defining the service to the SId,
///     Note that the subchannel is assigned through a FIG0/3
void FIBProcessor::bind_packetService(
        int8_t TMid,
        uint32_t SId,
        int16_t compnr,
        int16_t SCId,
        int16_t ps_flag,
        int16_t CAflag)
{
    Service *s = findServiceId (SId);
    if (std::find_if(components.begin(), components.end(),
                [&](const ServiceComponent& sc) {
                    return sc.SId == s->serviceId && sc.componentNr == compnr;
                }) == components.end()) {
        ServiceComponent newcomp;
        newcomp.TMid        = TMid;
        newcomp.SId         = SId;
        newcomp.componentNr = compnr;
        newcomp.SCId        = SCId;
        newcomp.PS_flag     = ps_flag;
        newcomp.CAflag      = CAflag;
        components.push_back(newcomp);

        //  std::clog << "fib-processor:" << "service %8x (comp %d) is packet\n", SId, compnr) << std::endl;
    }
}

void FIBProcessor::clearEnsemble()
{
    std::lock_guard<std::mutex> lock(mutex);
    components.clear();
    subChannels.resize(64);
    services.clear();

    firstTime   = true;
    isSynced    = false;
}

std::vector<Service> FIBProcessor::getServiceList() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return services;
}

std::list<ServiceComponent> FIBProcessor::getComponents(const Service& s) const
{
    std::list<ServiceComponent> c;
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto& component : components) {
        if (component.SId == s.serviceId) {
            c.push_back(component);
        }
    }

    return c;
}

Subchannel FIBProcessor::getSubchannel(const ServiceComponent& sc) const
{
    std::lock_guard<std::mutex> lock(mutex);
    return subChannels.at(sc.subchannelId);
}

uint16_t FIBProcessor::getEnsembleId() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return ensembleId;
}

uint8_t FIBProcessor::getEnsembleEcc() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return ensembleEcc;
}

DabLabel FIBProcessor::getEnsembleLabel() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return ensembleLabel;
}

bool FIBProcessor::syncReached()
{
    return isSynced;
}

