/*      SWLJO43 Anpassungen 2025
 *    Copyright (C) 2020
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
 * 
 *  |                   CRC-Ensemble                   |  ENSEMBLE
 |__________________________________________________|
         |                 |                 |
         |                 |                 |
  _______V______    _______V______    _______V______
 | CRC-Service1 |  | CRC-Service2 |  | CRC-Service3 |  SERVICES
 |______________|  |______________|  |______________|
    |        |        |        | |______         |
    |        |        |        |        |        |
  __V__    __V__    __V__    __V__    __V__    __V__
 | SC1 |  | SC2 |  | SC3 |  | SC4 |  | SC5 |  | SC6 |  SERVICE
 |_____|  |_____|  |_____|  |_____|  |_____|  |_____|  COMPONENTS
    |        |   _____|        |        |    ____|
    |        |  |              |        |   |
  __V________V__V______________V________V___V_______   COMMON
 | SubCh1 | SubCh9 |  ...  | SubCh3 | SubCh60 | ... |  INTERLEAVED
 |________|________|_______|________|_________|_____|  FRAME
 * 
 * 
 *  fib and fig processor
 */
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cstring>

#include "fib-processor.h"
#include "charsets.h"
#include "MathHelper.h"
#include "dab-constants.h"
#include <vector>
#include "channels.h"

FIBProcessor::FIBProcessor(RadioControllerInterface& mr) :
    myRadioInterface(mr)
{
    clearEnsemble();
}

//  FIB's are segments of 256 bits. When here, we already
//  passed the crc and we start unpacking into FIGs
//  This is merely a dispatcher
void FIBProcessor::processFIB(uint8_t *p, uint16_t fib)
{
    int8_t  processedBytes  = 0;
    uint8_t *d = p;
    uint8_t FIGtype = 0;
    uint8_t FIGlength = 0;

    std::lock_guard<std::mutex> lock(mutex);

    (void)fib;

    while (processedBytes  < 30) {
        FIGtype = getBits_3 (d, 0); // FIG header von links 3Bits ab Position null
        FIGlength = getBits_5(d, 3);
        if(FIGlength < 1) {
		    fprintf(stderr, "FICDecoder: received empty FIG 0\n");
		    return;
	    }
        if ((FIGtype == 0x07) && (FIGlength == 0x3F))
            return;

       // fprintf (stderr, "FIG: %4d %4d %4d\n,", FIGtype, processedBytes, fib);
        switch (FIGtype) {
            case 0:
                process_FIG0(d); // MCI and part of the SI 
                break;

            case 1:
               process_FIG1(d); // Labels, etc. (part of the SI) 
                break;

            case 2:
                process_FIG2(d); // Labels, etc. (part of the SI) 
                break;

            case 3:
                //process_FIG3(d); // Labels, etc. (part of the SI) 
                break;

            case 4: //  FIC service information
               // fprintf(stderr,"FIG0/%d %d", FIGtype, fib);
                //process_FIG4(d); // Reserved
                break;

            case 5: //  (asynchronous insertion)  32 bytes per frame (FIGs and CRC)
                //fprintf(stderr,"FIG0/%d %d", FIGtype, fib);
                process_FIG5(d); // Reserved
                break;

            case 6:
                //process_FIG6(d); // Conditional Access (CA) 
                break;

            case 7:  // In-house data
                if (processedBytes == 31) return; // wenn processedByte = 31 , dann Endmakierung
                process_FIG7(d); // Reserved (except for Length 31)
                break;

            default:
                fprintf(stderr,"FIG%d present? \n",FIGtype);
                break;
        }

        processedBytes += FIGlength +1 ; // zum nächsten Block mit aktueller Länge 
        d = p + processedBytes*8;
//fprintf(stderr," B%d L%d d%i\n",processedBytes, FIGlength, fib);
        //ORG:
    //    processedBytes += getBits_5 (d, 3) + 1; // FIG länge von links 5Bits ab Position 3 
    //    d = p + processedBytes * 8;
     /*   ORG:
        Length: this 5-bit field shall represent the length in bytes of the FIG data field and is expressed as an unsigned 
binary number (MSb first) in the range 1 to 29. Values 0, 30 and 31 shall be reserved for future use of the 
FIG data field except for 31 ("11111") when used with FIG type 7 ("111") which is used for the end marker. 
        */
    }
    
}

//
//  Handle ensemble is all through FIG7
//
void FIBProcessor::process_FIG7 (uint8_t *d)
{
    int16_t used        = 2; // offset in bytes
    int16_t Length      = getBits_5(d, 3);
    uint8_t CN_bit      = getBits_1(d, 8 + 0);
    uint8_t OE_bit      = getBits_1(d, 8 + 1);
    uint8_t PD_bit      = getBits_1(d, 8 + 2);
    uint8_t extension   = getBits_5 (d, 8 + 3);
 
   // if (extension>0) fprintf(stderr,"FIG7:%d ", extension);

    switch (extension) {
        case 0:
            //fprintf(stderr,"FIG7/0:%d ", Length);
            //FIG7Extension0 (d);
         break;
        case 1: 
        //FIG7Extension1 (d);
        
         break;
        case 2: 
        //FIG7Extension2 (d);

         break;
        case 3:
         //FIG7Extension3 (d);
        break;
        case 4:
            //fprintf(stderr,"FIG7/4:%d ", Length);
            //FIG7Extension4 (d);
          break;
        case 5: 
        //FIG7Extension5 (d);

         break;
        case 6:
            //fprintf(stderr,"FIG7/6:%d ", Length);
            //FIG7Extension6 (d);                
          break;         
        case 7:
            //fprintf(stderr,"FIG7/7:%d ", Length);
            //FIG7Extension7 (d);                
          break;          
        case 8:
        // FIG7Extension8 (d);

          break;
        case 9: 
        //FIG7Extension9 (d);

         break;
        case 10:
       //  FIG7Extension10 (d);
        break;
        case 11:
            //fprintf(stderr,"FIG7/11:%d ", Length);
            //FIG7Extension11 (d);                
          break;        
        case 12:
            //fprintf(stderr,"FIG7/12:%d ", Length);
            //FIG7Extension12 (d);
          break;
        case 13:
        // FIG7Extension13 (d);

          break;
        case 14:
        // FIG7Extension14 (d);

          break;
        case 15:
            //fprintf(stderr,"FIG7/15:%d ", Length);
            //FIG7Extension15 (d);                
          break;          
        case 16:
            //fprintf(stderr,"FIG7/16:%d ", Length);
            //FIG7Extension16 (d);                
          break;          
        case 17:
        // FIG7Extension17 (d);

          break;
        case 18:
        // FIG7Extension18 (d);

          break;
        case 19:
        // FIG7Extension19 (d);
          break;
        case 20:
            //fprintf(stderr,"FIG7/20:%d ", Length);
            //FIG7Extension20 (d);                
          break;          
        case 21:
        // fprintf(stderr,"FIG7/21:%d ", Length);
        // FIG7Extension21 (d);
          break;
        case 22:
        // FIG7Extension22 (d);
        break;
        case 23:
            //fprintf(stderr,"FIG7/23:%d ", Length);
            //FIG7Extension23 (d);                
          break;        
        case 24:
            //fprintf(stderr,"FIG7/24:%d ", Length);
            //FIG7Extension24 (d);    
            break;    
        case 25:
            //fprintf(stderr,"FIG7/25:%d ", Length);
            //FIG7Extension25 (d);                
          break;
        case 26:
            //fprintf(stderr,"FIG7/26:%d ", Length);
            //FIG7Extension26 (d);                
          break;          
        case 27:
            //fprintf(stderr,"FIG7/27:%d ", Length);
            //FIG7Extension27 (d);                
          break;          
        case 28:
            //fprintf(stderr,"FIG7/28:%d ", Length);
            //FIG7Extension28 (d);                
          break;          
        case 29:
            //fprintf(stderr,"FIG7/29:%d ", Length);
            //FIG7Extension29 (d);                
          break;          
        case 30:
            //fprintf(stderr,"FIG7/30:%d ", Length);
            //FIG7Extension30 (d);                
          break;          
        case 31:
            //fprintf(stderr,"FIG7/31:%d ", Length);
            //FIG7Extension31 (d);                
          break;          
        default:
                 fprintf(stderr,"passed by FIG7/%d: %d",extension, Length);
                 
                 //  std::cerr << "fib-processor" << "FIG7/%d passed by\n" << extension << std::endl;
            break;
    }
}

//
//  Handle ensemble is all through FIG5
//
void FIBProcessor::process_FIG5 (uint8_t *d)
{
    uint8_t extension   = getBits_5 (d, 8 + 3);
    //uint8_t   CN  = getBits_1 (d, 8 + 0);
 
    switch (extension) {
        ///case 0: FIG5Extension0 (d); break;
        case 1: 
        FIG5Extension1 (d);
        std::clog << "fib-processor:" << "FIG5/%d passed by\n" << extension << std::endl;
         break;
        //case 2: FIG5Extension2 (d); break;
        //case 3: FIG5Extension3 (d); break;
        //case 5: FIG5Extension5 (d); break;
       // case 8: FIG5Extension8 (d); break;
       // case 9: FIG5Extension9 (d); break;
       // case 10: FIG5Extension10 (d); break;
       // case 14: FIG5Extension14 (d); break;
       // case 13: FIG5Extension13 (d); break;
       // case 17: FIG5Extension17 (d); break;
       // case 18: FIG5Extension18 (d); break;
        //case 19: FIG5Extension19 (d); break;
        //case 21: FIG5Extension21 (d); break;
        //case 22: FIG5Extension22 (d); break;
        default:
                   fprintf(stderr,"fib-processor:/%d passed by\n",extension);
            break;
    }
}

void FIBProcessor::FIG5Extension1 (uint8_t *d)
//  FIG 5/1 DAB-TMC messages
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);
    uint8_t PD_bit  = getBits_1 (d, 8 + 2); // 

    while (used < Length) {
        used = HandleFIG5Extension1 (d, used, PD_bit); //0=37bits, 1=16bits
    }
}


// 4.1 DAB-TMC service component in the FIDC,  ETSI TS 102 368
// The fifth service (identified by the service label "TPEG") consists of only a primary service component carrying Traffic 
// and Travel Information (TTI) via Transport Protocol Expert Group (TPEG). 
int16_t FIBProcessor::HandleFIG5Extension1(
        uint8_t *d,
        int16_t used,
        uint8_t pdBit)
{
    int16_t  lOffset = used * 8;
    uint32_t SId = getBits(d, lOffset, pdBit == 1 ? 16 : 37);
    lOffset += (pdBit == 1 ? 16 : 37);
     fprintf(stderr," FIG5/1 TMC=%x ",SId);
    /* wie DAB-TMC
37-bit TMC Message: this 37-bit field shall contain one of the following: 
.... 
• TMC User message: a message as defined in ISO 14819-1 [4], comprising parameters such as the Location 
code and Event code. 
• TMC tuning information: information as defined in ISO 14819-1 [4], comprising information that a TMC 
product needs to change from one transmitter to another if the signal becomes weak. 
• Encryption Administration group: information as defined in ISO 14819-6 [6] comprising details of the 
encryption parameters such as the Service Identifier (SID), the ENCryption Identifier (ENCID) and the 
Location Table Number Before Encryption (LTNBE). 
• Future information: future TMC applications conveyed in RDS-ODA groups requiring 37 bits mapping 
(e.g. 11A, 13A, etc.). 
The mapping of 37-bit TMC messages is described in clause 5.1 of the present document. 
    */

   /* wie RDS-TMC
   16-bit TMC Message: this 16-bit field shall contain TMC system information as defined in ISO 14819-1 [4]. 
The mapping of 16-bit TMC messages is described in clause 5.2 of the present document. 
ETSI 
11 
Draft ETSI TS 102 368 V1.1.1 (2005-01) 
Figure 3 shows in a conceptual block diagram how a TMC encoder transmits TMC messages comprising TMC user and 
system messages to a TMC decoder via the RDS and DAB channel. The TMC decoder located in the receiver extracts 
the TMC messages from the RDS and DAB channel. The upper DAB-TMC path is the subject of the present document. 
The lower part is described in ISO 14819-1 [4]. 
   
   */

    uint8_t extensionFlag   = getBits_1(d, lOffset);
    uint16_t SCIds   = getBits_4(d, lOffset + 4);
    lOffset += 4;


// std::clog << "T51";

/*
    uint8_t lsFlag  = getBits_1(d, lOffset);
    if (lsFlag == 1) {
        int16_t SCid = getBits(d, lOffset + 4, 12);
        lOffset += 16;
        //           if (findPacketComponent ((SCIds << 4) | SCid) != NULL) {
        //              std::clog << "fib-processor:" << "packet component bestaat !!\n") << std::endl;
        //           }
    }
    else {
        int16_t SubChId = getBits_6(d, lOffset + 4);
        lOffset += 8;
    }

    if (extensionFlag) {
        lOffset += 8;   // skip Rfa
    }
*/
      (void)SId;
    (void)SCIds;
    return lOffset / 8;
}

//
//  Handle ensemble is all through FIG0
//
void FIBProcessor::process_FIG0 (uint8_t *d)
{
    /*
     1bit   1bit   1bit  5 bits
    b7      b6     b5    b4 b0
    C/N     OE     P/D   Extension
  Figure 7: Structure of the FIG type 0 data field
    */
    uint8_t extension   = getBits_5 (d, 8 + 3); // 5bits ab position 11 von links

    switch (extension) {
        case 0: FIG0Extension0 (d); break; // ensemble information (6.4.1)
        case 1: FIG0Extension1 (d); break; // sub-channel organization (6.2.1)
        case 2: FIG0Extension2 (d); break; // service organization (6.3.1)
        case 3: FIG0Extension3 (d); break; // service component in packet mode (6.3.2)
        case 4: FIG0Extension4 (d); break; // service component with CA (6.3.3)
        case 5: FIG0Extension5 (d); break;  // service component language (8.1.2)
        case 6: FIG0Extension6 (d); break; // service linking information (8.1.15)
        case 7: FIG0Extension7 (d); break; // configuration information (6.4.2)
        case 8: FIG0Extension8 (d); break; // service component global definition (6.3.5)
        case 9: FIG0Extension9 (d); break; // country, LTO & international table (8.1.3.2)
        case 10: FIG0Extension10 (d); break; // date and time (8.1.3.1)
        case 11: FIG0Extension11 (d); break; // obsolete
        case 13: FIG0Extension13 (d); break; // user application information (6.3.6)
        case 14: FIG0Extension14 (d); break; // FEC subchannel organization (6.2.2)
        
        case 17: FIG0Extension17 (d); break; // Program type (8.1.5)
        case 18: FIG0Extension18 (d); break; // announcement support (8.1.6.1)
        case 19: FIG0Extension19 (d); break; // announcement switching (8.1.6.2)
        case 20: FIG0Extension20 (d); break; // service component information (8.1.4)
        case 21: FIG0Extension21 (d); break; // frequency information (8.1.8)
        case 22: FIG0Extension22 (d); break; // obsolete
        case 24: FIG0Extension24 (d); break; // OE services (8.1.10)
        //case 25: FIG0Extension25 (d); break; // OE announcement support (8.1.6.3)
        //case 26: FIG0Extension26 (d); break; // OE announcement switching (8.1.6.4)
        default:
            //        std::clog << "fib-processor:" << "FIG0/%d passed by\n", extension) << std::endl;
            break;
    }
}


//  FIG0/0 indicated a change in channel organization
//  we are not equipped for that, so we just return
//  control to the init
void FIBProcessor::FIG0Extension0 (uint8_t *d)
//  FIG 0/0 6.4.1 Ensemble information MCI Rfu Rfu Rfu 
{
    uint8_t     changeflag;
    uint16_t    highpart, lowpart;
    int16_t     occurrenceChange;
    uint8_t		alarmFlag;
    /*
          CN  0: current configuration; 
          CN  1: next configuration. 
    */
    uint8_t CN  = getBits_1 (d, 8 + 0); 
    (void)CN;

       
    uint8_t   PD  = getBits_1 (d, 8 + 2); 
        // 0: 16-bit SId, used for programme services;
        // 1: 32-bit SId, used for data services.
    uint16_t eId  = getBits(d, 16, 16);  // 5.2.2.1 Type 0 field

    (void)eId;
    if (ensembleId != eId) {
        ensembleId = eId;
        myRadioInterface.onNewEnsemble(ensembleId);
    }
/*
Change flags: this 2-bit field shall be used to indicate whether there is to be a change to the multiplex configuration, as 
follows: 
*/
    changeflag      = getBits_2 (d, 16 + 16);
    alarmFlag		= getBits_1 (d, 16 + 16 + 2);
    (void)alarmFlag;
    highpart        = getBits_5 (d, 16 + 19) % 20;  //CIF Count
    (void)highpart;
    lowpart         = getBits_8 (d, 16 + 24) % 250;  //CIF Count
    (void)lowpart;
    occurrenceChange    = getBits_8 (d, 16 + 32);
    (void)occurrenceChange;

	//cifCount. store (highpart * 250 + lowpart);
	//if (getBits (d, 34, 1))         // only alarm, just ignore
	 //  return;

    // In transmission mode I, because four ETI frames make one transmission frame, we will
    // see lowpart == 0 only every twelve seconds, and not 6 as expected by the 250 overflow value.
    if (lowpart == 0) {
        timeLastFCT0Frame = std::chrono::system_clock::now();
    }


/*
0 = 0 0: no change, no occurrence change field present; 
1 = 0 1: next sub-channel organization only signalled (legacy support only); 
2 = 1 0: next service organization only signalled (legacy support only); 
3 = 1 1: complete next MCI (sub-channel organization and service organization) signalled. 
*/
    if (changeflag == 0)
        return;
//     else if (changeflag == 1) {
//         fprintf (stderr, "fib-processor: Changes in sub channel organization\n");
//        fprintf (stderr, "fib-processor: cifcount = %d\n", highpart * 250 + lowpart);
//         fprintf (stderr, "fib-processor: Change happening in %d CIFs\n", occurrenceChange );
//      }
//      else if (changeflag == 3) {
//         fprintf (stderr, "fib-processor: Changes in subchannel and service organization\n" );
//         fprintf (stderr, "fib-processor: cifcount = %d\n", highpart * 250 + lowpart) ;
//         fprintf (stderr, "fib-processor: change happening in %d CIFs\n", occurrenceChange) ;
//      }
    fprintf (stderr, "fib-processor: changes in config not supported, choose again" );
	if (alarmFlag)
	   fprintf (stderr, "serious problem\n");
}
//  FIG0 extension 1 creates a mapping between the
//  sub channel identifications and the positions in the
//  relevant CIF.
void FIBProcessor::FIG0Extension1 (uint8_t *d) // Subchannel for stream mode MSC
//  FIG 0/1 6.2.1 Sub-channel organization MCI MCI Rfu Rfu 
{
    int16_t used    = 2;        // offset in bytes = Sub-channel k?
    int16_t Length  = getBits_5 (d, 3); // 5.2.2.1 MCI and SI: FIG type 0 data field aus FIG header
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);  // 5.2.2.1 aus FIG data field, 0=16bit programme, 1=32bit data
    //uint8_t CN      = getBits_1 (d, 8 + 0);  // 5.2.2.1 aus FIG data field, 0=current, 1=next

    while (used < Length - 1)
        used = HandleFIG0Extension1 (d, used, PD_bit);
}

//  defining the channels
int16_t FIBProcessor::HandleFIG0Extension1(
        uint8_t *d,
        int16_t offset,
        uint8_t pd)
{
    int16_t bitOffset = offset * 8;  // used * 8bits = Sub-channel k?
    int16_t subChId   = getBits_6 (d, bitOffset); // Sub-channel k?
    int16_t startAdr  = getBits(d, bitOffset + 6, 10); // Sub-channel k?
    int16_t subChanSize = 0;
    
    subChannels[subChId].programmeNotData = pd;  // PD_bit,  // 5.2.2.1 aus FIG data field, 0=16bit programme, 1=32bit data
    subChannels[subChId].subChId = subChId;
    subChannels[subChId].inUse = true;
    subChannels[subChId].startAddr = startAdr;


    if (getBits_1 (d, bitOffset + 16) == 0) {   // UEP, short form
        int16_t tableIx = getBits_6 (d, bitOffset + 18); // aus size and protection
        auto& ps = subChannels[subChId].protectionSettings;
        ps.uepTableIndex = tableIx;
        ps.shortForm = true;
        ps.uepLevel = ProtLevel[tableIx][1];
        // bitrate?

        subChannels[subChId].length = ProtLevel[tableIx][0];
        bitOffset += 24;
    }
    else {  // EEP, long form
        auto& ps = subChannels[subChId].protectionSettings;
        ps.shortForm  = false;
        int16_t option = getBits_3(d, bitOffset + 17); // aus size and protection
        if (option == 0) {
            ps.eepProfile = EEPProtectionProfile::EEP_A;
        }
        else if (option == 1) {
            ps.eepProfile = EEPProtectionProfile::EEP_B;
        }

        if ((option == 0) or (option == 1))   // EEP-A protection = 0
             {                            // EEP-B protection = 1
            int16_t protLevel = getBits_2(d, bitOffset + 20); // Protection level
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

            subChanSize = getBits(d, bitOffset + 22, 10);
            subChannels[subChId].length = subChanSize;
        }
        else {
            std::clog << "Warning, FIG0/1 for " << subChId <<
                " has invalid protection option " << option << std::endl;
        }
        subChanSize = getBits(d, bitOffset + 22, 10);
        subChannels[subChId].length = subChanSize;
        bitOffset += 32;
    }
    if (pd == 1)
        fprintf (stderr, "--xx--xx--xx--xx--> FIG0/1 programmeNotData %d, subChId %x, subChanSize %x\n", pd, subChId, subChanSize);
    return bitOffset / 8;   // we return bytes
}

// Service and service components information in stream mode
void FIBProcessor::FIG0Extension2 (uint8_t *d)
//  FIG 0/2 6.3.1 Service organization MCI MCI Rfu P/D 
//	bind channels to SIds
//  d = Type 0 field for extension 2, bestehend aus Service v + Service k + Service t
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3); // 5.2.2.1 MCI and SI: FIG type 0 data field aus FIG header
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);  // 5.2.2.1 aus FIG data field, 0=16bit programme, 1=32bit data
    uint8_t CN      = getBits_1 (d, 8 + 0);  // 5.2.2.1 aus FIG data field, 0=current, 1=next

    while (used < Length) {
        used = HandleFIG0Extension2(d, used, CN, PD_bit);
    }
}

//  Note Offset is in bytes
//  With FIG0/2 we bind the channels to Service Ids
// 6.1 Format of FIG 0/2 (Service Organization) for DAB-TMC Seite 13
int16_t FIBProcessor::HandleFIG0Extension2(
        uint8_t *d,
        int16_t offset,
        uint8_t cn,
        uint8_t pd)
{
    (void)cn;
    int16_t     lOffset = 8 * offset;
    int16_t     i;
    uint8_t     ecc; // If the TMC service is a component of a programme service, then the ECC shall be carried in FIG 0/9 (Country, LTO and International table)
    uint8_t     cId;
    uint32_t    SId;
    int16_t     numberofComponents;
    //uint8_t     OE  = getBits_1 (d, 8 + 1);  // 5.2.2.1 Other Ensemble (OE), 0=this, 1=other  (or FM or AM or DRM service)

    if (pd == 1) {      // long Sid data
        ecc = getBits_8(d, lOffset);   (void)ecc; // If the TMC service is a component of a programme service, then the ECC shall be carried in FIG 0/9 (Country, LTO and International table)
        cId = getBits_4(d, lOffset + 1); // war "+8" 6.3.1 Basic service and service component definition 
        SId = getBits(d, lOffset, 32);  // The 32 bit SId identifies the DAB service carrying the TMC service, it is a data service
        lOffset += 32;
        //fprintf (stderr, "FIG0/2 ecc %4x, SId %4x, cId %4x\n", ecc, SId, cId);
    }
    else { // short programme
        cId = getBits_4(d, lOffset);   (void)cId;
		SId = getBits(d, lOffset + 4, 12);								  
        SId = getBits(d, lOffset, 16);
        lOffset += 16;
    }

    // Keep track how often we see a service using a saturating counter.
    // Every time a service is signalled, we increment the counter.
    // If the counter is >= 2, we consider the service. Every second, we
    // decrement all counters by one.
    // This avoids that misdecoded services appear and stay in the list.
    using namespace std::chrono;
    const auto now = steady_clock::now();
    if (timeLastServiceDecrement + seconds(1) < now) {

        auto it = serviceRepeatCount.begin();
        while (it != serviceRepeatCount.end()) {
            if (it->second > 0) {
                it->second--;
                ++it;
            }
            else if (it->second == 0) {
                dropService(it->second);
                it = serviceRepeatCount.erase(it);
            }
            else {
                ++it;
            }
        }

        timeLastServiceDecrement = now;

#if 0
        std::stringstream ss;
        ss << "Counters: ";
        for (auto& c : serviceRepeatCount) {
            ss << " " << c.first << ":" << (int)c.second;
        }
        std::cerr << ss.str() << std::endl;
#endif
    }

    if (serviceRepeatCount[SId] < 4) {  // 4
        serviceRepeatCount[SId]++;
    }

    if ((findServiceId(SId) == nullptr) and (serviceRepeatCount[SId] >= 1)) {  // ist SID = ServiceID noch nicht vorhanden
        services.emplace_back(SId);
        myRadioInterface.onServiceDetected(SId);  // gebe trigger heraus, das ein Service gefunden wurde
    }
        // NOTE: To determine the SCIdS (service component id within service)
        // Refer to clause 6.3.7.1 of EN 300 401
        // It states that we should correlate the service id and the subchannel id 
        // This is done by getting the SCIdS/subchannel_id pairing from fig 0/8
        // Get all the components
    numberofComponents = getBits_4(d, lOffset + 4); // Number of service components
    lOffset += 8; // springe bis zum Anfang von Service description k des Services k von FIG0/2


    

    for (i = 0; i < numberofComponents; i ++) {
        uint8_t TMid    = getBits_2 (d, lOffset);  // TMId (Transport Mechanism Identifier): this 2-bit field shall indicate the transport mechanism used
//fprintf (stderr, "FIG0/2 MSC numberofComponents %d, TMid %d\n", numberofComponents,TMid);

        if (TMid == 00)  {  // Audio TMId=00  (MSC stream audio)
            uint8_t ASCTy   = getBits_6 (d, lOffset + 2);  // DAB or DAB+
            uint8_t SubChId = getBits_6 (d, lOffset + 8);
            uint8_t PS_flag = getBits_1 (d, lOffset + 14);
            bindAudioService(TMid, SId, i, SubChId, PS_flag, ASCTy);
        }
        else if (TMid == 3) { // MSC packet data  TMId=11  (MSC packet data)
            int16_t SCId    = getBits (d, lOffset + 2, 12);
            uint8_t PS_flag = getBits_1 (d, lOffset + 14);
            uint8_t CA_flag = getBits_1 (d, lOffset + 15);
            uint8_t DSCTy   = getBits_6 (d, lOffset + 2);
            bindPacketService(TMid, SId, i, SCId, PS_flag, CA_flag);
         //   fprintf (stderr, "FIG0/2 MSC DSCTy : %d\n", DSCTy);
        }        
        else if (TMid == 1) { // MSC stream data TMId=01  (MSC stream data)
            uint8_t DSCTy   = getBits_6 (d, lOffset + 2);
            uint8_t SubChId = getBits_6 (d, lOffset + 8);
            uint8_t PS_flag = getBits_1 (d, lOffset + 14);
            bindDataStreamService(TMid, SId, i, SubChId, PS_flag, DSCTy);
            //fprintf (stderr, "FIG0/2 MSC DSCTy : %d\n", DSCTy);
        }
        else if (TMid == 2) { // FIDC stream data TMId=02  (FIDC stream data)
            uint8_t DSCTy   = getBits_6 (d, lOffset + 2);
            uint8_t SubChId = getBits_6 (d, lOffset + 8);
            uint8_t PS_flag = getBits_1 (d, lOffset + 14);
            bindFIDCDataStreamService(TMid, SId, i, SubChId, PS_flag, DSCTy);
            //fprintf (stderr, "FIG0/2 FIDC DSCTy : %d\n", DSCTy);
        }

        else {
            // reserved
            fprintf (stderr, "FIG0/2 FIDC TMid : %d\n", TMid);
        }
        lOffset += 16;
    }
    return lOffset / 8;     // in Bytes
}


//      The Extension 3 of FIG type 0 (FIG 0/3) gives
//      additional information about the service component
//      description in packet mode.
//      manual: page 55
// In FIG 0/3, the DSCTy shall be set to TDC (see ETSI TS 101 756 [2])
void FIBProcessor::FIG0Extension3 (uint8_t *d)
//  FIG 0/3 6.3.2 Service component in packet mode MCI MCI Rfu Rfu 
{
    int16_t used    = 2;
    int16_t Length  = getBits_5 (d, 3);

    while (used < Length) {
        used = HandleFIG0Extension3 (d, used);
    }
}

//      DSCTy   DataService Component Type
int16_t FIBProcessor::HandleFIG0Extension3(uint8_t *d, int16_t used)
{
    //uint8_t CN_bit  = getBits_1 (d, 8 + 0);
    //uint8_t OE_bit  = getBits_1 (d, 8 + 1);
    int16_t compnr;
    int16_t used8 = used * 8;
    int16_t SCId            = getBits (d, used8, 12);  // SCId (Service Component Identifier): see clause 6.3.1.
    int16_t CAOrgflag       = getBits_1 (d, used8 + 15); // CAOrg flag: this 1-bit flag shall indicate whether the Conditional Access Organization (CAOrg) field is present, or not
    int16_t DGflag          = getBits_1 (d, used8 + 16);  //  DG flag: this 1-bit flag shall indicate whether data groups are used to transport the service component
    int16_t DSCTy           = getBits_6 (d, used8 + 18); // DSCTy (Data Service Component Type): see clause 6.3.1.
    int16_t SubChId         = getBits_6 (d, used8 + 24); // SubChId (Sub-channel Identifier): see clause 6.3.1. 
    int16_t packetAddress   = getBits (d, used8 + 30, 10);  //  Packet address: this 10-bit field shall define the address of the packet in which the service component is carried. 
    uint16_t CAOrg          = getBits (d, used8 + 40, 16);  // CAOrg (Conditional Access Organization): this 16-bit field shall contain information about the applied Conditional Access Systems and mode (see ETSI TS 102 367 [4]).
    //uint8_t PD_bit          = getBits_1 (d, used8 + 2);

    std::string serviceName;

     if (CAOrgflag == 1) {
	   CAOrg = getBits (d, used8 + 40, 16);
	   used += 16 / 8; 
        }
    used += 40 / 8;
	(void)CAOrg;
 //   if (DSCTy == 1) fprintf(stderr,"## DSCTy Traffic Message CHannel (TMC) gefunden\n");
    if (DSCTy == 2) fprintf(stderr,"## DSCTy Emergency Warning System (EWS) gefunden\n");
    if (DSCTy == 59) fprintf(stderr,"## DSCTy ip data gefunden\n");
    if (DSCTy == 44) fprintf(stderr,"## DSCTy journaline data gefunden\n");
 //   if (DSCTy == 5) fprintf(stderr,"## DSCTy Transparent Data Channel   (TDC) gefunden\n");
 //   if (DSCTy == 60) fprintf(stderr,"## DSCTy Multimedia Object Transfer (MOT) gefunden\n");

//      if the  Data Service Component Type == 0, we do not deal
//      with it
    // if (DSCTy == 0) return used;
    ServiceComponent *packetComp = findPacketComponent(SCId);
    if (packetComp) {      
         packetComp->subchannelId    = SubChId;
         packetComp->SCId            = SCId;
         packetComp->DSCTy           = DSCTy; // 5 = Transparent Data Channel (TDC), see ETSI TS 101 759 [5] , 60 = Multimedia Object Transfer (MOT), see ETSI EN 301 234 [2] 
         packetComp->DGflag          = DGflag;
         packetComp->packetAddress   = packetAddress;
         packetComp->inUse           = true;
         packetComp->TMid            = 3;
         }


   return used;
}


// Service components information in stream mode with conditional access
void FIBProcessor::FIG0Extension4(uint8_t *d)
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);

    while (used < Length) {
        used = HandleFIG0Extension4 (d, used);
    }
}

// ETSI 300 401 6.3.3 Service component with Conditional Access in stream mode
int16_t FIBProcessor::HandleFIG0Extension4(uint8_t* d, int16_t offset)
{
    int16_t     lOffset = 8 * offset;
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);  // 5.2.2.1 aus FIG data field, 0=16bit programme, 1=32bit data
    uint8_t CN      = getBits_1 (d, 8 + 0);  // 5.2.2.1 aus FIG data field, 0=current, 1=next
    uint8_t     cId;
    uint32_t    SId;

    if (PD_bit == 1) {      // long Sid data
        cId = getBits_4(d, lOffset + 4); // war "+8" 6.3.1 Basic service and service component definition 
        SId = getBits(d, lOffset, 32);  // The 32 bit SId identifies the DAB service carrying the TMC service, it is a data service
        lOffset += 32;
        //
    }
    else { // short programme
        cId = getBits_4(d, lOffset);   (void)cId;
        SId = getBits(d, lOffset, 16);
        lOffset += 16;
    }

    fprintf (stderr, "FIG0/4 SId %4x, cId %4x\n", SId, cId);
    /*
    const int N = (int)d.size();
    const int nb_component_bytes = 3;
    if ((N % nb_component_bytes) != 0) {
        LOG_ERROR("fig 0/4 Field must be a multiple of {} bytes", 
            nb_component_bytes);
        return;
    }

    const int nb_components = N / nb_component_bytes;
    for (int i = 0; i < nb_components; i++) {
        auto* b = &buf[i*nb_component_bytes];
        const uint8_t rfa           = (b[0] & 0b10000000) >> 7;
        const uint8_t rfu           = (b[0] & 0b01000000) >> 6;
        const uint8_t subchannel_id = (b[0] & 0b00111111) >> 0;
        const uint16_t CAOrg =
                (static_cast<uint16_t>(b[1] & 0b11111111) << 8) |
                                     ((b[2] & 0b11111111) >> 0);
        LOG_MESSAGE("fig 0/4 i={}/{} rfa={} rfu={} subchannel_id={} CAOrg={}",
            i, nb_components,
            rfa, rfu, subchannel_id, CAOrg);
        
        m_handler->OnServiceComponent_2_StreamConditionalAccess(
            subchannel_id, CAOrg);
    }
    */
   return 0;
}





void FIBProcessor::FIG0Extension5 (uint8_t *d)
//  FIG 0/5 8.1.2 Service component language SI Rfu Rfu Rfu 
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);

    while (used < Length) {
        used = HandleFIG0Extension5 (d, used);
    }
}

// Service component language 
int16_t FIBProcessor::HandleFIG0Extension5(uint8_t* d, int16_t offset)
{
    int16_t loffset = offset * 8;
    uint8_t lsFlag  = getBits_1 (d, loffset);
    int16_t subChId, SCId, language;

    if (lsFlag == 0) {  // short form
        if (getBits_1 (d, loffset + 1) == 0) { // Rfu
            subChId = getBits_6 (d, loffset + 2);
            language = getBits_8 (d, loffset + 8);
            subChannels[subChId].language = language;
//            fprintf (stderr, "------------ FIG0/5 short language : %4x %4x %4x\n", subChId, SCId, language);
        }
        loffset += 16;
    }
    else {          // long form
        SCId = getBits (d, loffset + 4, 12);
        language    = getBits_8 (d, loffset + 16);
        loffset += 24;
        ServiceComponent *packetComp = findPacketComponent(SCId);
        if (packetComp == nullptr)		// no serviceComponent yet nullptr
            return loffset / 8;
     
        uint32_t serviceIndex = findServiceIdPOS( packetComp->SId);
        if (serviceIndex == (uint32_t)-1) // kein serviceIndex bei dieser SId
	        return loffset / 8;
  
        //fprintf(stderr, "------------> FIG0/5 long language %x\n",language );
        packetComp->language = language;
                

        
    }
    (void)SCId;
    //fprintf (stderr, "FIG0/5 language : %4x %4x %4x\n", subChId, SCId, language);
    return loffset / 8;
}

//  FIG 0/6 8.1.15 Service linking information SI SIV Rfu P/D 

void FIBProcessor::FIG0Extension6 (uint8_t *d)
{
  //  fprintf (stderr, "FIG0/6 : %4x \n", d);
 //   int16_t used    = 2;        // offset in bytes
 //   int16_t Length  = getBits_5 (d, 3);
 //   fprintf (stderr, "FIG0/6 : %4x %4x %4x\n", used, Length, d);
  //  while (used < Length) {
  //      used = HandleFIG0Extension5 (d, used);
  //  }
}

/*
void FIBProcessor::FIG0Extension6(
    const FIG_Header_Type_0 header, 
    tcb::span<const uint8_t> buf)
{
    const int N = (int)buf.size();
    const int nb_header_bytes = 2;

    int curr_byte = 0;
    while (curr_byte < N) {
        const int nb_remain_bytes = N-curr_byte;

        // minimum of 16 bits = 2 bytes
        if (nb_remain_bytes < nb_header_bytes) {
            LOG_ERROR("fig 0/6 Insufficient length for header ({}/{})",
                nb_header_bytes, nb_remain_bytes);
            return;
        }

        auto* b = &buf[curr_byte];

        const uint8_t id_list_flag =     (b[0] & 0b10000000) >> 7;
        const uint8_t is_active_link =   (b[0] & 0b01000000) >> 6;
        const uint8_t is_hard_link =     (b[0] & 0b00100000) >> 5;
        const uint8_t is_international = (b[0] & 0b00010000) >> 4;
        const uint16_t linkage_set_number = 
                   (static_cast<uint16_t>(b[0] & 0b00001111) << 8) |
                                        ((b[1] & 0b11111111) >> 0);

        // short data field without id list
        if (!id_list_flag) {
            LOG_MESSAGE("fig 0/6 pd={} ld={} LA={} S/H={} ILS={} LSN={}",
                header.pd,
                id_list_flag, is_active_link, is_hard_link, is_international, linkage_set_number);
            
            m_handler->OnServiceLinkage_1_LSN_Only(
                is_active_link, is_hard_link, is_international, 
                linkage_set_number);
            
            curr_byte += nb_header_bytes;
            continue;
        }

        // id list is present
        // it must contain at least a list header byte
        const int nb_list_header_bytes = 1;
        const int nb_total_header_bytes = nb_header_bytes + nb_list_header_bytes;

        if (nb_remain_bytes < nb_total_header_bytes) {
            LOG_ERROR("fig 0/6 Insufficient length for long header ({}/{})",
                nb_total_header_bytes, nb_remain_bytes);
            return;
        }

        const uint8_t rfu0   = (b[2] & 0b10000000) >> 7;
        const uint8_t IdLQ   = (b[2] & 0b01100000) >> 5;
        const uint8_t Rfa0   = (b[2] & 0b00010000) >> 4;
        const uint8_t nb_ids = (b[2] & 0b00001111) >> 0;

        const int nb_list_remain = nb_remain_bytes-nb_total_header_bytes;
        if (nb_list_remain <= 0) {
            LOG_ERROR("fig 0/6 Insufficient length for any list buffer");
            return;
        }

        // 3 possible arrangements for id list
        auto* list_buf = &b[3];

        // Arrangement 1: List of 16bit IDs
        if (!header.pd && !is_international) {
            const int nb_id_bytes = 2;
            const int nb_list_bytes = nb_id_bytes*nb_ids;
            if (nb_list_bytes > nb_list_remain) {
                LOG_ERROR("fig 0/6 Insufficient length for type 1 id list ({}/{})",
                    nb_list_bytes, nb_list_remain);
                return;
            }

            for (int i = 0; i < nb_ids; i++)  {
                auto* entry_buf = &list_buf[i*nb_id_bytes];

                // Interpret id according to value of IdLQ (id list qualifier) 
                switch (IdLQ) {
                case 0b00: // DAB service id - 16bit
                    {
                        ServiceIdentifier sid;
                        sid.ProcessShortForm({entry_buf, (size_t)2});
                        LOG_MESSAGE("fig 0/6 pd={} ld={} LA={} S/H={} ILS={} LSN={} rfu0={} IdLQ={} Rfa0={} type=1 i={}/{} country_id={} service_ref={} ecc={}",
                            header.pd,
                            id_list_flag, is_active_link, is_hard_link, is_international, linkage_set_number,
                            rfu0, IdLQ, Rfa0, 
                            i, nb_ids,
                            sid.country_id, sid.service_reference, sid.ecc);

                        m_handler->OnServiceLinkage_1_ServiceID(
                            is_active_link, is_hard_link, is_international,
                            linkage_set_number, 
                            sid.country_id, sid.service_reference, sid.ecc);
                    }
                    break;
                case 0b01: // RDS-PI code 
                    {
                        const uint16_t rds_pi_code = 
                            (static_cast<uint16_t>(entry_buf[0]) << 8) |
                                                  (entry_buf[1]  << 0);
                        LOG_MESSAGE("fig 0/6 pd={} ld={} LA={} S/H={} ILS={} LSN={} rfu0={} IdLQ={} Rfa0={} type=1 i={}/{} RDS_PI={:04X}",
                            header.pd,
                            id_list_flag, is_active_link, is_hard_link, is_international, linkage_set_number,
                            rfu0, IdLQ, Rfa0, 
                            i, nb_ids,
                            rds_pi_code);
                        
                        m_handler->OnServiceLinkage_1_RDS_PI_ID(
                            is_active_link, is_hard_link, is_international,
                            linkage_set_number, rds_pi_code);
                    }
                    break;
                case 0b11: // DRM 24bit-service identifier
                    {
                        const uint32_t drm_id = 
                            (static_cast<uint32_t>(entry_buf[0]) << 8) |
                                                  (entry_buf[1]  << 0);
                        LOG_MESSAGE("fig 0/6 pd={} ld={} LA={} S/H={} ILS={} LSN={} rfu0={} IdLQ={} Rfa0={} type=1 i={}/{} DRM_id={}",
                            header.pd,
                            id_list_flag, is_active_link, is_hard_link, is_international, linkage_set_number,
                            rfu0, IdLQ, Rfa0, 
                            i, nb_ids,
                            drm_id);
                        
                        m_handler->OnServiceLinkage_1_DRM_ID(
                            is_active_link, is_hard_link, is_international,
                            linkage_set_number, drm_id);
                    }
                    break;
                default:
                    // Reserved for future use
                    break;
                }
                
            }

            curr_byte += (nb_total_header_bytes + nb_list_bytes);
            continue;
        } 

        // Arrangement 2: List of pairs of (8bit ECC and 16bit ID)
        if (!header.pd && is_international) {
            const int nb_entry_bytes = 3;
            const int nb_list_bytes = nb_entry_bytes*nb_ids;
            if (nb_list_bytes > nb_list_remain) {
                LOG_ERROR("fig 0/6 Insufficient length for type 2 id list ({}/{})",
                    nb_list_bytes, nb_list_remain);
                return;
            }

            for (int i = 0; i < nb_ids; i++)  {
                auto* entry_buf = &list_buf[i*nb_entry_bytes];
                const uint8_t ecc = entry_buf[0];

                // Interpret id according to value of IdLQ (id list qualifier) 
                switch (IdLQ) {
                case 0b00: // DAB service id - 16bit with ecc provided separately
                    {
                        ServiceIdentifier sid;
                        sid.ProcessShortForm({&entry_buf[1], (size_t)2});
                        sid.ecc = ecc;
                        LOG_MESSAGE("fig 0/6 pd={} ld={} LA={} S/H={} ILS={} LSN={} rfu0={} IdLQ={} Rfa0={} type=2 i={}/{} country_id={} service_ref={} ecc={}",
                            header.pd,
                            id_list_flag, is_active_link, is_hard_link, is_international, linkage_set_number,
                            rfu0, IdLQ, Rfa0, 
                            i, nb_ids,
                            sid.country_id, sid.service_reference, sid.ecc);
                        
                        m_handler->OnServiceLinkage_1_ServiceID(
                            is_active_link, is_hard_link, is_international,
                            linkage_set_number,
                            sid.country_id, sid.service_reference, sid.ecc);
                    }
                    break;
                case 0b01: // RDS-PI code with ecc
                    {
                        const uint16_t rds_pi_code = 
                            (static_cast<uint16_t>(entry_buf[1]) << 8) |
                                                  (entry_buf[2]  << 0);
                        LOG_MESSAGE("fig 0/6 pd={} ld={} LA={} S/H={} ILS={} LSN={} rfu0={} IdLQ={} Rfa0={} type=2 i={}/{} RDS_PI={:04X} ecc={}",
                            header.pd,
                            id_list_flag, is_active_link, is_hard_link, is_international, linkage_set_number,
                            rfu0, IdLQ, Rfa0, 
                            i, nb_ids,
                            rds_pi_code, ecc);
                        
                        m_handler->OnServiceLinkage_1_RDS_PI_ID(
                            is_active_link, is_hard_link, is_international,
                            linkage_set_number, 
                            rds_pi_code, ecc);
                    }
                    break;
                case 0b11: // DRM 24bit service identifier with ecc as MSB
                    {
                        const uint32_t drm_id = 
                            (static_cast<uint32_t>(ecc)          << 16) |
                            (static_cast<uint32_t>(entry_buf[1]) << 8) |
                                                  (entry_buf[2]  << 0);
                        LOG_MESSAGE("fig 0/6 pd={} ld={} LA={} S/H={} ILS={} LSN={} rfu0={} IdLQ={} Rfa0={} type=2 i={}/{} DRM_id={}",
                            header.pd,
                            id_list_flag, is_active_link, is_hard_link, is_international, linkage_set_number,
                            rfu0, IdLQ, Rfa0, 
                            i, nb_ids,
                            drm_id);
                        
                        m_handler->OnServiceLinkage_1_DRM_ID(
                            is_active_link, is_hard_link, is_international,
                            linkage_set_number, drm_id);
                    }
                    break;
                default:
                    // Reserved for future use
                    break;
                }
            }

            curr_byte += (nb_total_header_bytes + nb_list_bytes);
            continue;
        } 

        // Arrangement 3: List of 32bit IDs
        {
            const int nb_entry_bytes = 4;
            const int nb_list_bytes = nb_entry_bytes*nb_ids;
            if (nb_list_bytes > nb_list_remain) {
                LOG_ERROR("fig 0/6 Insufficient length for type 3 id list ({}/{})",
                    nb_list_bytes, nb_list_remain);
                return;
            }
            for (int i = 0; i < nb_ids; i++)  {
                auto* entry_buf = &list_buf[i*nb_entry_bytes];

                // Interpret id according to value of IdLQ (id list qualifier) 
                switch (IdLQ) {
                case 0b00: // DAB service id - 32bit 
                    {
                        ServiceIdentifier sid;
                        sid.ProcessLongForm({entry_buf, (size_t)4});
                        LOG_MESSAGE("fig 0/6 pd={} ld={} LA={} S/H={} ILS={} LSN={} rfu0={} IdLQ={} Rfa0={} type=3 i={}/{} country_id={} service_ref={} ecc={}",
                            header.pd,
                            id_list_flag, is_active_link, is_hard_link, is_international, linkage_set_number,
                            rfu0, IdLQ, Rfa0, 
                            i, nb_ids,
                            sid.country_id, sid.service_reference, sid.ecc);
                        
                        m_handler->OnServiceLinkage_1_ServiceID(
                            is_active_link, is_hard_link, is_international,
                            linkage_set_number, 
                            sid.country_id, sid.service_reference, sid.ecc);
                    }
                    break;
                case 0b01: // RDS-PI code 
                    {
                        // const uint32_t id = 
                        //     (static_cast<uint32_t>(entry_buf[0]) << 24) |
                        //     (static_cast<uint32_t>(entry_buf[1]) << 16) |
                        //     (static_cast<uint32_t>(entry_buf[2]) << 8 ) |
                        //                           (entry_buf[3]  << 0 );
                        // TODO: Figure out how the RDSPI code is interpreted in the 32bit field
                        const uint16_t rds_pi_code = 
                              (static_cast<uint16_t>(entry_buf[2]) << 8) |
                                                    (entry_buf[3]  << 0);
                        LOG_MESSAGE("fig 0/6 pd={} ld={} LA={} S/H={} ILS={} LSN={} rfu0={} IdLQ={} Rfa0={} type=3 i={}/{} RDS_PI={:08X}",
                            header.pd,
                            id_list_flag, is_active_link, is_hard_link, is_international, linkage_set_number,
                            rfu0, IdLQ, Rfa0, 
                            i, nb_ids,
                            rds_pi_code);
                        
                        m_handler->OnServiceLinkage_1_RDS_PI_ID(
                            is_active_link, is_hard_link, is_international,
                            linkage_set_number, rds_pi_code);
                    }
                    break;
                case 0b11: // DRM 24bit service identifier
                    {
                        // DRM service id
                        const uint32_t drm_id = 
                            (static_cast<uint32_t>(entry_buf[0]) << 24) |
                            (static_cast<uint32_t>(entry_buf[1]) << 16) |
                            (static_cast<uint32_t>(entry_buf[2]) << 8 ) |
                                                  (entry_buf[3]  << 0 );
                        LOG_MESSAGE("fig 0/6 pd={} ld={} LA={} S/H={} ILS={} LSN={} rfu0={} IdLQ={} Rfa0={} type=3 i={}/{} DRM_id={}",
                            header.pd,
                            id_list_flag, is_active_link, is_hard_link, is_international, linkage_set_number,
                            rfu0, IdLQ, Rfa0, 
                            i, nb_ids,
                            drm_id);
                        
                        m_handler->OnServiceLinkage_1_DRM_ID(
                            is_active_link, is_hard_link, is_international,
                            linkage_set_number, drm_id);
                    }
                    break;
                default:
                    // Reserved for future use
                    break;
                }
            }

            curr_byte += (nb_total_header_bytes + nb_list_bytes);
            continue;
        }
    }
}

*/



//  FIG 0/7 6.4.2 Configuration information MCI MCI Rfu Rfu
//
void FIBProcessor::FIG0Extension7(uint8_t *d)
{
    int16_t used = 2; // offset in bytes
    int16_t Length = getBits_5(d, 3);
    uint8_t CN_bit = getBits_1(d, 8 + 0);
    uint8_t OE_bit = getBits_1(d, 8 + 1);
    uint8_t PD_bit = getBits_1(d, 8 + 2);

    uint serviceCount = getBits_6(d, used * 8);  //  Services: this 6-bit field, coded as an unsigned binary number, contains the total number of services in the configuration. 
    uint counter = getBits(d, used * 8 + 6, 10); //  Count: this modulo-1 024 binary counter increments by one for every multiplex reconfiguration. 

	(void)Length;
	(void)CN_bit; (void)OE_bit; (void)PD_bit;
    //fprintf (stderr, "FIG0/7 services, counter: %d %d\n", serviceCount, counter);
	if (CN_bit == 0)	// only current configuration for now
	   // nrServices (serviceCount);
	(void)counter;
}



void FIBProcessor::FIG0Extension8 (uint8_t *d)
//  FIG 0/8 6.3.5 Service component global definition MCI MCI Rfu P/D 
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);

    while (used < Length) {
        used = HandleFIG0Extension8 (d, used, PD_bit);
    }
}


/*
FIG 0/8: Service Component Global Definition. This FIG provides the appropriate cross-referencing
information to link the service component identifiers that are valid globally to those that are valid only within a
DAB ensemble. FIG 0/8 indicates where the DAB-TMC service component is located in the FIDC (Fast
Information Data Channel) of the DAB Ensemble. The short form (L/S = 0) shall be used. The MSC/FIC flag
shall be set to "1" to indicate that the TMC component is carried in the FIC and that the subsequent 6-bits field
contains the FIDCId (Fast Information Data Channel Identifier) which identifies the TMC service component
carried in the FIDC 
*/
// 6.3.5 Service component global definition 
int16_t FIBProcessor::HandleFIG0Extension8(
        uint8_t *d,
        int16_t used,
        uint8_t pdBit)
{
    int16_t 	lOffset	= used * 8;
    uint32_t	SId	= getLBits (d, lOffset, pdBit == 1 ? 32 : 16); // SId wie z.B. "d8e1"=Audio, "e0d020f0"=data
    uint8_t		lsFlag;
    uint16_t	SCIds;  //  SCIdS provides the link to the User Application information signalled in FIG 0/13
    uint16_t		SCid;
    uint16_t		MSCflag;
   uint16_t		SubChId;
    uint8_t		extensionFlag;
    uint32_t    compnr = 0;
	lOffset += (pdBit == 1 ? 32 : 16);
    extensionFlag   = getBits_1 (d, lOffset);
    SCIds   = getBits_4 (d, lOffset + 4);
    lOffset += 8; // war 8
    lsFlag  = getBits_1 (d, lOffset);
    
    if (lsFlag==1) {  // long form DATA
            SCid = getBits (d, lOffset + 4, 12);
            lOffset += 16;
          //  ServiceComponent *packetComp = findPacketComponent(SCid);
            Service *s = findServiceId(SId); // SId wie z.B. "d8e1"=Audio, "e0d020f0"=data
            int servicecompSID = findPacketComponentSIDPOS(SId);
            if (servicecompSID == -1)
                return (lOffset += 8)/8;
          std::string Name = s->serviceLabel.fig1_label.c_str();
           
        
           }
	else {      // short form
        MSCflag	= getBits_1 (d, lOffset + 1); // 1 indicate that the TMC component is carried in the FIC
	    SubChId	= getBits_6 (d, lOffset + 2);
        //fprintf(stderr, "FIG0/8 short SId=%x, SCid=%x, SCIds=%x, SubChId=%x, MSCflag=%x\n",SId,SCid,SCIds,SubChId,MSCflag);
        int compIndex;
        lOffset += 8;
        if (!MSCflag)
            {
                if (subChannels[SubChId].inUse)
                {
                    ServiceComponent *packetComp =  findComponent(SId,SubChId);
                        if (packetComp == nullptr)	{	// no serviceComponent yet nullptr
                            return (lOffset += 8)/8;
                            fprintf(stderr, "FIG0/8 short form=%x\n",packetComp);
                        }
                if (findPacketComponent ((SCIds << 4) | SubChId) != NULL)
                    {  
                    packetComp->SCIds    = SCIds;
                    subChannels [SubChId]. inUse = true;
                    packetComp->inUse    = true;
                    
                     fprintf(stderr, "FIG0/8 MSCflag %x, SId %x, SCIds %x, SubChId %x, = %x \n",MSCflag,SId, SCIds, SubChId, ((SCIds << 4) | SubChId));  
                }
                }
                
            }
    }
	if (extensionFlag)
	    lOffset += 8;	// skip Rfa
	(void)SId;
	(void)SCIds;
	(void)SCid;
	(void)SubChId;
	(void)MSCflag;
    return lOffset / 8;
}


//  FIG0/9 and FIG0/10 are copied from the work of
//  Michael Hoehn
//  If the TMC service is a component of a programme service, then the ECC shall be carried in FIG 0/9+
/*
FIG 0/9: Country, LTO, and International Table. This FIG is required because it defines the local time
offset and the ECC (Extended Country Code). The Ensemble ECC (which makes the Ensemble Id unique
worldwide) shall be as defined in TS 101 756 [2]. The Service ECC shall be coded in the same way as the
Ensemble ECC.
*/

void FIBProcessor::FIG0Extension9(uint8_t *d)
//  FIG 0/9 8.1.3.2 Country, LTO & International table SI Rfu Rfu Rfu 
{
    int16_t offset  = 16;
    uint8_t ecc;

//	6 indicates the number of hours
	int	signbit = getBits_1 (d, offset + 2);
//	dateTime [6] = (signbit == 1)? -1 * getBits_4 (d, offset + 3): getBits_4 (d, offset + 3);
//	7 indicates a possible remaining half our
//	dateTime [7] = (getBits_1 (d, offset + 7) == 1) ? 30 : 0;

 //   if (signbit == 1) dateTime [7] = -dateTime [7];

	ecc	     = getBits (d, offset + 8, 8);
	if (!ensembleLabel.ecc_Present) {
	   ensembleLabel.ecc_byte = ecc;
	   ensembleLabel.ecc_Present = true;
	}
    

    dateTime.hourOffset = (getBits_1 (d, offset + 2) == 1) ? -1 * getBits_4 (d, offset + 3): getBits_4 (d, offset + 3);
    dateTime.minuteOffset = (getBits_1 (d, offset + 7) == 1) ? 30 : 0;
    timeOffsetReceived = true;
    ensembleEcc = getBits(d, offset + 8, 8); // FIG 0/2, if the TMC service is a component of a programme service, then the ECC shall be carried in FIG 0/9 (Country, LTO and International table)
    
  // fprintf(stderr, "FIG0/9 : ECC=%x, MinOffset=%x, HourOffset=%x\n", ensembleEcc, dateTime.minuteOffset, dateTime.hourOffset);
}


//static
//QString monthTable [] = {
//"jan", "feb", "mar", "apr", "may", "jun",
//"jul", "aug", "sep", "oct", "nov", "dec"};

int	monthLength [] {
31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
//
//	Time in 10 is given in UTC, for other time zones
//	we add (or subtract) a number of Hours (half hours)
void	adjustTime (int32_t *dateTime) {
//	first adjust the half hour  in the amount of minutes
	dateTime [4] += (dateTime [7] == 1) ? 30 : 0;
	if (dateTime [4] >= 60) {
	   dateTime [4] -= 60; dateTime [3] ++;
	}

	if (dateTime [4] < 0) {
	   dateTime [4] += 60; dateTime [3] --;
	}

	dateTime [3] += dateTime [6];
	if ((0 <= dateTime [3]) && (dateTime [3] <= 23))
	   return;

	if (dateTime [3] > 23) {
	   dateTime [3] -= 24; dateTime [2] ++;
	}

	if (dateTime [3] < 0) {
	   dateTime [3] += 24; dateTime [2] --;
	}

	if (dateTime [2] > monthLength [dateTime [1] - 1]) {
	   dateTime [2] = 1; dateTime [1] ++;
	   if (dateTime [1] > 12) {
	      dateTime [1] = 1;
	      dateTime [0] ++;
	   }
	}

	if (dateTime [2] < 0) {
	   if (dateTime [1] > 1) {
	      dateTime [2] = monthLength [dateTime [1] - 1 - 1];
	      dateTime [1] --;
	   }
	   else {
	      dateTime [2] = monthLength [11];
	      dateTime [1] = 12; dateTime [0] --;
	   }
	}
}

double FIBProcessor::dateToJulianDate(int year, int month, int day) {
    if (month <= 2) {
        year--;
        month += 12;
    }
    int A =int(year / 100);
    int B = 2 - A + int(A / 4);
    int JD = int(365.25 * (year + 4716)) + int(30.6001 * (month + 1)) + day + B - 1524.5;
    return JD;
}




/*
FIG 0/10: Data and Time. This FIG is required in order to signal a location-independent timing reference in
UTC format. Together with the Local Time Offset, provided in FIG 0/9, an Ensemble/Service related time is
specified. UTC allows time interval calculations to be made independent of time zones and summer-time
discontinuities. The date is encoded in Modified Julian Date (MJD) format
*/

/*
FIG 0/8 bietet Informationen zur eindeutigen Identifizierung von Servicekomponenten weltweit (in Kombination mit SId und ECC) und
zur Verknüpfung der Servicekomponente mit dem Servicekomponentenlabel, dem X-PAD-Benutzeranwendungslabel und den Benutzeranwendungsinformationen.
*/
void FIBProcessor::FIG0Extension10(uint8_t *fig)
//  FIG 0/10 8.1.3.1 Date & time SI Rfu Rfu Rfu 
{
    int16_t     offset = 16;
    int32_t     mjd = getBits(fig, offset + 1, 17);
    Global_theDay = mjd; // epg
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
     Global_year = Y;
     Global_month = M;
     Global_day   = D;
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


/*
FIG 0/11: Region Definition (see clause 3.6.15 of TR 101 496-2 [3]). This FIG can enhance a DAB-TMC
service as it allows defining geographical regions. In the receiver the Region Identification (and the TII
database) can be used for filtering TMC messages. It is recommended to provide in FIG 0/11 geographical
co-ordinates (GATy = 0001) to enable a receiver to map the regions provided in TMC onto DAB regions
which can then be used for filtering.
*/
void FIBProcessor::FIG0Extension11 (uint8_t *d)
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);

    while (used < Length) {
        //used = HandleFIG0Extension11 (d, used, PD_bit);
    }
}


//   Format of FIG 0/13 (User Application information) for DAB-TMC
void FIBProcessor::FIG0Extension13 (uint8_t *d)
//  FIG 0/13 6.3.6 User Application information MCI MCI Rfu P/D
{
    int16_t used    = 2;        // offset in bytes
    int16_t Length  = getBits_5 (d, 3);
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);

    while (used < Length) {
        used = HandleFIG0Extension13 (d, used, PD_bit);
    }
}


//6.2 Format of FIG 0/13 (User Application information) for DAB-TMC
// In FIG 0/13, the UATy shall be set to TPEG (see ETSI TS 101 756 [2]). 
int16_t FIBProcessor::HandleFIG0Extension13(
        uint8_t *d,
        int16_t used,
        uint8_t pdBit)
{
    
    int16_t  lOffset = used * 8;
    /*
    SId (Service Identifier): this 16-bit or 32-bit field shall identify the service (see clause 6.3.1) and the length of the SId 
shall be signalled by the P/D flag (see clause 5.2.2.1).
    */
    uint32_t SId = getBits(d, lOffset, pdBit == 1 ? 32 : 16);
    /*
    SCIdS (Service Component Identifier within the Service): this 4-bit field shall identify the service component within 
the service. The combination of the SId and the SCIdS provides a globally valid identifier for a service component. 
    */
    uint16_t SCIds;
    /*
    Number of user applications: this 4-bit field, expressed as an unsigned binary number, shall indicate the number of 
user applications (in the range 1 to 6) contained in the subsequent list. 
    */
    int16_t  NoApplications;
    int16_t  i;
//  The combination of the SId and the SCIdS provides a globally valid identifier for the service component.
//  FIG 0/8 (Service component global definition) indicates where the TMC service component is located in
//  the FIDC (Fast Information Data Channel)

    lOffset     += pdBit == 1 ? 32 : 16;
    SCIds       = getBits_4 (d, lOffset);
    NoApplications = getBits_4 (d, lOffset + 4);
    lOffset += 8;
    int16_t appType =0;
    int16_t length  =0;

//    std::clog << "fib-processor: HandleFIG0Extension13 NoApplications " << NoApplications << " ";
// Go through all user apps in user app information block
    for (i = 0; i < NoApplications; i++) {
 
        appType = getBits (d, lOffset, 11);
        length  = getBits_5 (d, lOffset + 11);
      
        lOffset += (11 + 5 + 8 * length);
//fprintf(stderr, "  FIG0/13 TMid=%d, bind packet service=(%8x), length=%d, appType=%x, lOffset=%x\n", packetComp->TMid,SId, length, appType,lOffset);
        switch (appType) {
            case 0x000:     // reserved for future use
                break;
            case 0x001:     // Dynamic labels (X-PAD only)
                std::clog << "X-PAD \n";
                break;

            case 0x002:     // MOT slideshow
                //std::clog << "MOT slideshow \n"; // oft aufgerufen
              //  fprintf(stderr, "  FIG0/13 MOT NoApplications %d, bind packet service (%8x), length %d, appType %x dataName=%c\n",NoApplications ,SId, length, appType, dataName);
                break;
            case 0x003:     // MOT Broadcast Web Site
                //std::clog << "MOT Broadcast Web Site \n"; 
                
                break;
            case 0x004:     // TPEG
               //fprintf(stderr, "FIG0/13 TPEG SId %x for SCIds %x with %x NoApplications \n",SId , SCIds, NoApplications);  // sehr oft
                break;  
            case 0x005:     // DGPS
                std::clog << "DGPS \n"; 
                break;
            case 0x006:     // TMC
                std::clog << "TMC \n"; 
                break;
            case 0x007:     // EPG SPI
               {
            //   std::vector<epgElement> res = find_epgData(SId);
            //   fprintf(stderr, "FIG0/13  EPG SId %x for SCIds %x with %x NoApplications, res=%x \n",SId , SCIds, NoApplications, res);
                break;
               }
            case 0x008:     // DAB Java
                std::clog << "DAB Java \n"; 
                break;
            case 0x009:     // DMB
                std::clog << "DMB \n"; 
                break;
            case 0x00a:     // IPDC services
                std::clog << "IPDC services \n";
                break;
            case 0x00b:     // Voice applications
                std::clog << "Voice applications \n"; 
                break;
            case 0x00c:     // Middleware
                std::clog << "Middleware \n"; 
                break;
            case 0x00d:     // Filecasting
                std::clog << "Filecasting \n";
                break;
            case 0x44a:     // Journaline
                {
               //std::clog << "Journaline \n"; 
                
                
                }
                break;
            case 0x5dc:     // DR Deutschland
             //   std::clog << "DR Deutschland \n"; 
                break;
            default:
                fprintf (stderr,"FIG 0/13 unknown apptype=%x\n",appType);
                break;
        }
        
    }

//    std::clog << std::endl;
    //fprintf (stderr, "FIG0/13 services : %d\n", appType);
    (void)SId;
    (void)SCIds;
    return lOffset / 8;
}

void FIBProcessor::FIG0Extension14 (uint8_t *d)
//  FIG 0/14 6.2.2 FEC sub-channel organization MCI MCI Rfu Rfu 
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
// FIG 0/17 - Programme Type
void FIBProcessor::FIG0Extension17(uint8_t *d)
//  FIG 0/17 8.1.5 Programme Type (PTy) SI Rfu Rfu Rfu 
{
    int16_t length  = getBits_5 (d, 3);
    int16_t offset  = 16;
    Service *s;

    while (offset < length * 8) {
        uint32_t SId = getBits (d, offset, 16);
        bool    L_flag  = getBits_1 (d, offset + 18);
        bool    CC_flag = getBits_1 (d, offset + 19);
        int16_t type;
        int16_t Language = 0x00;    // init with unknown language
        s = findServiceId(SId);     // 
        uint32_t  serviceIndex = findServiceIdPOS(SId);
        if (L_flag) {       // language field present
            Language = getBits_8 (d, offset + 24);
            offset += 8;
        }    
        type	= getBits_5 (d, offset + 27);



        if (CC_flag) {          // cc flag
            offset += 40;
        }
        else {
            offset += 32;
        if (s) {
            s->language = Language;  // ensemble -> services [serviceIndex]. language	= Language;
            s->programType = type;       // ensemble -> services [serviceIndex]. programType	= type;
            services[serviceIndex].language = Language;
            services[serviceIndex].programType = type;
            
            //fprintf (stderr,"FIG 0/17 type=%x, language=%x\n",type, Language);
        }
        }
    }
}

void FIBProcessor::FIG0Extension18(uint8_t *d)
//  FIG 0/18 8.1.6.1 Announcements support SI Rfu Rfu Rfu 
{
    int16_t  used       = 2;       // byte
    int16_t  Length     = getBits_5 (d, 3);
    uint8_t	CN_bit		= getBits_1 (d, 8 + 0);
    uint8_t	OE_bit		= getBits_1 (d, 8 + 1);
    uint8_t	PD_bit		= getBits_1 (d, 8 + 2);
    int16_t	offset		= used * 8;
    uint32_t SId;
    uint8_t AsuFlags = 0;
    int16_t NumClusters = 0;
    int16_t  announcement_ClusterID = 0;
    Service *s;
    
    while (offset / 8 < Length - 1 ) { // durchsuche alle Services
        SId = getBits (d, offset, 16);   
        s = findServiceId(SId);     // serviceIndex	= find_service (SId);   
        int32_t  serviceIndex = findServiceIdPOS(SId);
        offset += 16;
        AsuFlags = getBits (d, offset, 16);
        (void)AsuFlags; 
        offset += 16;
        uint8_t Rfa		= getBits (d, offset,  5);
        (void)Rfa;
        NumClusters = getBits (d, offset + 5,3);
        offset += 8;

	   for (int i = 0; i < NumClusters; i ++) {
	      if (getBits (d, offset + 8 * i, 8) == 0)
	         continue;
          if ((serviceIndex !=-1)  && (s))
           {    // hier weiter
                 announcement_ClusterID = getBits (d, offset + 8 * i, 8);
               setCluster(announcement_ClusterID, serviceIndex, AsuFlags);

                //   std::string announcementText = announcements(AsuFlags);
                //    fprintf(stderr, "FIG0/18 Announcement %s for SId %x with ClusterID=%x\n",announcementText.c_str(), SId,  announcement_ClusterID);
          }

                         
	   }
        if (s) {
            s->announcement_AsuFlags = AsuFlags;  // ensemble -> services [serviceIndex]. announcement_AsuFlags	= AsuFlags;
            s-> announcement_ClusterID =  announcement_ClusterID;       // ensemble -> services [serviceIndex]. ClusterID	= ClusterID;
        
        }
        offset +=  NumClusters * 8;
    }
    (void)SId;
    (void)AsuFlags;

}

static inline 
int	bits (uint s) {
uint32_t startBit = 01;
	for (int i = 0; i < 15; i ++) {
	   if ((s & startBit) != 0)
	      return i;
	   startBit <<= 1;
	}
	return 0;
}


void FIBProcessor::FIG0Extension19(uint8_t *d)
//  FIG 0/19 8.1.6.2 Announcement switching SI Rfu Rfu Rfu 
{
    int16_t  offset     = 16;       // bits
    int16_t  Length     = getBits_5 (d, 3); // in bytes
    uint8_t	CN_bit		= getBits_1 (d, 8 + 0);
    uint8_t	OE_bit		= getBits_1 (d, 8 + 1);
    uint8_t	PD_bit		= getBits_1 (d, 8 + 2);
    uint8_t  region_Id_Lower;
    std::string name = "";

    (void)OE_bit; (void)PD_bit;

    while (offset / 8 < Length - 1) {
        uint8_t clusterId   = getBits_8 (d, offset);
        uint16_t aswFlags   = getBits (d, offset + 8, 16);
        bool    new_flag    = getBits_1(d, offset + 24);
        bool    region_flag = getBits_1 (d, offset + 25);
        uint8_t subChId     = getBits_6 (d, offset + 26);
       

       // fprintf(stderr, "FIG0/19 %s %s Announcement %d for Cluster %2u on SubCh %2u region \n", ((new_flag==1)?"new":"old"), ((region_flag==1)?"regional":""), aswFlags, clusterId,subChId);
        
        if (region_flag==1) {
            region_Id_Lower = getBits_6 (d, offset + 34);
            offset += 40;
            fprintf(stderr, "FIG0/19 %s %s Announcement %d for Cluster %2u on SubCh %2u lower \n", ((new_flag==1)?"new":"old"), ((region_flag==1)?"regional":""), aswFlags, clusterId,subChId);
        }
        else {
            offset += 32;
            
        }
//	   if (!syncReached ())
//	      return;
        Cluster *myCluster = getCluster (clusterId);

	   if (myCluster == nullptr) {	// should not happen
	      fprintf (stderr, "cluster fout\n");
	      continue;
	   }
        
        int announcementId = bits (aswFlags);
	   if ((myCluster -> flags & aswFlags) != 0) {
	      myCluster -> announcing ++;
	      if (myCluster -> announcing >= 5) {
	         for (uint16_t i = 0; i < myCluster -> services. size (); i ++) {
                uint16_t aaa = myCluster -> services [i];
                std::string bbb = services[aaa].serviceLabel.fig1_label.c_str();
                name = bbb; 
                bbb = announcements(announcementId);           
                //fprintf(stderr," Announcement i=%x  ID=%x Name=%s, %s\n",i,aaa, bbb,name);
	               // fprintf(stderr, "**** emit ****FIG0/19 %s %s Announcement %d for Cluster %2u on SubCh %2u lower \n", ((new_flag==1)?"new":"old"), ((region_flag==1)?"regional":""), aswFlags, clusterId,subChId);
	               myRadioInterface.onNewAnnoucement (name, subChId, bbb);  // emit start_announcement (name, subChId, announcementId);
	         }
	      }
	   }
	   else {	// end of announcement
	      if (myCluster -> announcing > 0) {
	         myCluster -> announcing = 0;
	         for (uint16_t i = 0; i < myCluster -> services. size (); i ++) {
                uint16_t aaa = myCluster -> services [i];
                std::string bbb = services[aaa].serviceLabel.fig1_label.c_str();
                name = bbb; 
                bbb = announcements(announcementId);
              //  fprintf(stderr," Announcement i=%x  ID=%x Name=%s, %s\n",i,aaa, bbb,name);
	          //  fprintf(stderr, "**** stop ****FIG0/19 %s %s Announcement %d for Cluster %2u on SubCh %2u lower \n", ((new_flag==1)?"new":"old"), ((region_flag==1)?"regional":""), aswFlags, clusterId,subChId);
                myRadioInterface.onStopAnnoucement (name, subChId, bbb); //emit stop_announcement (name, subChId);
	         }
	      }
	   }



        //     fprintf(stderr,"\n");
        (void)clusterId;
        (void)new_flag;
        (void)subChId;
        (void)aswFlags;
    }
    (void)region_Id_Lower;
}

//  FIG 0/20 8.1.4 Service component information SI Rfu Rfu P/D
void FIBProcessor::FIG0Extension20(uint8_t *d)
{
    int16_t	used		  = 2;		// offset in bytes
    int16_t offset        = 8;       // on bits
    int16_t	Length		  = getBits_5 (d, 3);  // 5 bits ab Position 3 von links
    uint8_t	CN_bit		  = getBits_1 (d, offset + 0); // 1 bit ab Position 8 von links 
    uint8_t	OE_bit		  = getBits_1 (d, offset + 1);
    uint8_t	PD_bit		  = getBits_1 (d, offset + 2);
    uint8_t Rfu           = getBits_1 (d, offset + 4);
    uint8_t extension     = getBits_5 (d, offset + 11);
    offset                = offset +  8;
    uint32_t SId          = getBits   (d, offset, PD_bit == 1 ? 16 : 32);
    offset                = offset +  (PD_bit == 1 ? 16 : 32);
    int16_t SCId          = getBits_4 (d, offset );
    uint8_t change_flag   = getBits_2 (d, offset + 4);
    uint8_t PT_flag       = getBits_1 (d, offset + 6);
    uint8_t SC_flag       = getBits_1 (d, offset + 7);
    uint8_t CA_flag       = getBits_1 (d, offset + 8);
    uint8_t AD_flag       = getBits_1 (d, offset + 9);
    uint8_t SCTy          = getBits_6 (d, offset + 10);
    uint8_t Date          = getBits_5 (d, offset + 16);
    uint8_t Hour          = getBits_5 (d, offset + 21);
    uint8_t Minute        = getBits_6 (d, offset + 26);
    uint8_t Second        = getBits_6 (d, offset + 32);
    uint8_t SId_flag      = getBits_1 (d, offset + 38);
    uint8_t EId_flag      = getBits_1 (d, offset + 39);
    uint32_t Transfer_SId = getBits   (d, offset + 40, PD_bit == 1 ? 16 : 32);
    uint32_t Transfer_EId = getBits   (d, offset + 40 + (PD_bit == 1 ? 16 : 32), 16);


    fprintf(stderr, "FIG0/20 %d %d ",Minute, Second  );
    
    
}

/*
FIG 0/21: Frequency Information (FI). This FIG is required to allow mobile receivers, leaving the coverage
area of a DAB transmitter or single frequency network, to re-tune to an alternative frequency (service
following). The alternative frequency may apply to an identical DAB Ensemble, an Other Ensemble (carrying
the equivalent TMC service component) or the frequency of an equivalent RDS-TMC service. Service
following relies not only on FI, but also on OE services (FIG 0/24), Service Linking (FIG 0/6) and Region
definition (FIG 0/11). The network related information in the TMC Tuning Information shall be carried in the
FIC using the Service Following scheme described in clause 3.6.23 in TR 101 496-2 [3].
*/

void FIBProcessor::FIG0Extension21(uint8_t *d)
//  FIG 0/21 8.1.8 Frequency information (FI) SI SIV OE Rfu 
{
    int16_t	used		= 2;		// offset in bytes
    int16_t	Length		= getBits_5 (d, 3);  // 5 bits ab Position 3 von links
    uint8_t	CN_bit		= getBits_1 (d, 8 + 0); // 1 bit ab Position 8 von links 
    uint8_t	OE_bit		= getBits_1 (d, 8 + 1);  // Wenn OE-flag dann FM with RDS
    uint8_t	PD_bit		= getBits_1 (d, 8 + 2);

	while (used < Length) 
	   used = HandleFIG0Extension21 (d, CN_bit, OE_bit, PD_bit, used);
       //fprintf(stderr,"FIG0/21 Frequency information\n");
   
}

int16_t	FIBProcessor::HandleFIG0Extension21 (uint8_t	*d,
	                                   uint8_t	CN_bit,
	                                   uint8_t	OE_bit,
	                                   uint8_t	PD_bit,
	                                   int16_t	offset) {
int16_t	l_offset	= offset * 8;
int16_t	nb_freq_list_bytes	= getBits_5 (d, l_offset + 11);  // Länge der fi Liste, skip the Rfa=11bits
int16_t		upperLimit	= l_offset + 16 + nb_freq_list_bytes * 8;
int16_t		base		= l_offset + 16;


	(void)CN_bit; (void)OE_bit; (void)PD_bit;
    

	while (base < upperLimit) {
        // Header ETSI 300 401 8.1.8 Frequency Information
	    uint16_t idField	= getBits (d, base, 16);
	    uint8_t  RandM	= getBits_4 (d, base + 16);
	    uint8_t  continuity_flag	= getBits_1 (d, base + 20);
	    uint8_t  length	= getBits_3 (d, base + 21);
        std::string DABText     = " "; 
        uint32_t  DABFrequency	= 0;
        std::string sendername ="";

       int32_t serviceIndex	= findServiceIdPOS (idField);
        //fprintf(stderr,"FIG0/21-0 serviceIndex= %d idField=%x base=%x OE_bit=%x PD_bit=%x CN_bit=%x \n",serviceIndex,idField,base,OE_bit,PD_bit, CN_bit );

        // n x 8 bits = Freq. list
        //fprintf(stderr,"FIG0/21 Frequency information liste %i %i %i %i %i\n",idField, RandM,continuity_flag, length, nb_freq_list_bytes);
    switch (RandM)
        {
            case 0b0000:      // DAB ensemble, Id field = EId (see clause 6.4); 
               {
                const bool is_time_compensated = continuity_flag;
                DABText = " ";         
                uint16_t	controlfield	= getBits_5 (d, base + 24);  //  this 5-bit field shall be used to qualify the immediately following Freq
                uint16_t DABFrequency_key	= getBits (d, base + 29, 19);
                DABFrequency	= DABFrequency_key * 16;  // The centre carrier frequency of the other ensemble is given by (in this expression, the decimal equivalent of freq a is used):, 0 Hz + (Freq a × 16 kHz), 

                switch (controlfield)
                {
                case 0b00000:
                    DABText = " nearby DAB-TX DATA";
                    break;
                case 0b00010:
                    DABText = " nearby DAB-TX AUDIO";
                    break;
                case 0b00001:
                    DABText = " not nearby DAB-TX DATA";
                    break;
                case 0b00011:
                     DABText = " not nearby DAB-TX AUDIO";
                     break;
                default:
                    break;
                }

                //fprintf(stderr,"FIG0/21-0 DAB-Frequenz= %u kHz, %s, (%x)\n",DABFrequency,DABText.c_str(),continuity_flag);
                int i;       
                    
                    //fprintf(stderr,"FIG0/21-0 Zaehler= %i\n",DABFrequenzDatas.size);
    	            for (i = 0; i < 64; i ++) {
                        if ((DABFrequenzDatas.DAB_Freq[i] == DABFrequency) && (DABFrequenzDatas.DAB_idField[i] == idField)){
                        break;
                        } else {
                        if (DABFrequenzDatas.DAB_Freq[i] == 0) {
                            DABFrequenzDatas.DAB_Text  [i] = DABText.c_str();
	                        DABFrequenzDatas.DAB_Freq [i] = DABFrequency;
                            DABFrequenzDatas.DAB_idField [i] = idField;
                            DABFrequenzDatas.size         = i;
                            const auto chan = channels.getChannelForFrequency(DABFrequency*1000); // khz in hz
                            //fprintf(stderr,"channel=%s",chan.c_str());
                            myRadioInterface.onNewDABFREQ(sendername,DABFrequency,idField, DABText, chan);
                            break;
                            }            
                        }
                    } 
                  
            }

            break;
        
            case 0b1000:  // FM with RDS, ID: RDS PI-code (see IEC 62106 [10]) for FM radio
                {
                    const bool is_time_compensated = continuity_flag;
                    const uint16_t rds_pi_code = idField;                


                    uint16_t fmFrequency_key	= getBits (d, base + 24, 8);
	                uint32_t  fmFrequency	= 87500 + fmFrequency_key * 100;
	                
                    for (int i = 0; i < nb_freq_list_bytes; i++) {
           	        //sendername = services[serviceIndex].serviceLabel.fig1_label.c_str();
                        if ((DABFrequenzDatas.FM_Freq[i] == fmFrequency) && (DABFrequenzDatas.FM_rds_pi_code[i] == rds_pi_code)){
                            break;
                        } else {
                        if (DABFrequenzDatas.FM_Freq[i] == 0) {
                            DABFrequenzDatas.FM_Freq [i] = fmFrequency;
                            DABFrequenzDatas.FM_rds_pi_code [i] = rds_pi_code;
                          DABFrequenzDatas.size         = i;
                           //sendername = services[serviceIndex].serviceLabel.fig1_label.c_str();
                            myRadioInterface.OnFrequencyInformation_1_RDS_PI( rds_pi_code, sendername, fmFrequency, is_time_compensated);
                            break;
                            }            
                        }
                         // fprintf(stderr,"FIG0/21 PI=%x, Sender=%s, FM=%u, alt=%u\n",rds_pi_code,sendername,  fmFrequency,is_time_compensated );
                   
                    }
                            
                    
                }
                break;
           case 0b0110:  // ID: DRM Service Identifier (two least significant bytes) ETSI ES 201 980 [8]
                {
                    fprintf(stderr,"FIG0/21-8 DRM= %u, PI=%x\n",nb_freq_list_bytes, rand);
   /*                 
                    // 
                    const bool is_time_compensated = continuity_flag;

                    const uint8_t nb_entry_bytes = 3;
                    if ((nb_freq_list_bytes % nb_entry_bytes) != 0) {
                        LOG_ERROR("fig 0/21 Frequency list RM={} doesn't have a list length that is a multiple ({}{})",
                            RM, nb_freq_list_bytes, nb_entry_bytes);
                        return;
                    }
                    const int nb_entries = nb_freq_list_bytes / nb_entry_bytes;
                    for (int i = 0; i < nb_entries; i++) {
                        auto* b = &freq_list_buf[i*nb_entry_bytes];
                        const uint8_t drm_id_msb =    (b[0] & 0b11111111) >> 0;

                        const uint8_t is_multiplier = (b[1] & 0b10000000) >> 7;
                        const uint16_t freq = 
                                (static_cast<uint16_t>(b[1] & 0b01111111) << 8) | 
                                                     ((b[2] & 0b11111111) << 0);
                        const uint32_t drm_id = (static_cast<uint32_t>(drm_id_msb) << 16) | id;
                        // F' = k*F
                        // k = 1kHz or 10kHz depending on the multiplier flag
                        const uint32_t multiplier = is_multiplier ? 10000u : 1000u;
                        const uint32_t alt_freq = multiplier*freq;

                        LOG_MESSAGE("fig 0/21 i={}-{}-{}/{} Rfa0={} RM={} time_compensated={} DRM_id={} freq={}",
                            curr_block, curr_fi_list, i, nb_entries, 
                            Rfa0, RM, is_time_compensated, 
                            drm_id, (float)(alt_freq)*1e-6f);
                        m_handler->OnFrequencyInformation_1_DRM(
                            drm_id, alt_freq, is_time_compensated);
                    }
                    */
                }
                break;
            case 0b1110:    // ID: AMSS Service Identifier (most significant byte) ETSI TS 102 386
                {
                    fprintf(stderr,"FIG0/21-8 AMSS= %u, PI=%x",nb_freq_list_bytes, rand);
/*                     
                    // 
                    const bool is_time_compensated = continuity_flag;

                    const uint8_t nb_entry_bytes = 3;
                    if ((nb_freq_list_bytes % nb_entry_bytes) != 0) {
                        LOG_ERROR("fig 0/21 Frequency list RM={} doesn't have a list length that is a multiple ({}{})",
                            RM, nb_freq_list_bytes, nb_entry_bytes);
                        return;
                    }
                    const int nb_entries = nb_freq_list_bytes / nb_entry_bytes;
                    for (int i = 0; i < nb_entries; i++) {
                        auto* b = &freq_list_buf[i*nb_entry_bytes];
                        const uint8_t amss_id_msb = (b[0] & 0b11111111) >> 0;
                        const uint16_t freq = 
                              (static_cast<uint16_t>(b[1] & 0b11111111) << 8) | 
                                                   ((b[2] & 0b11111111) << 0);
                        const uint32_t amss_id = (static_cast<uint32_t>(amss_id_msb) << 16) | id;

                        // F' = F*1kHz 
                        const uint32_t alt_freq = freq*1000u;

                        LOG_MESSAGE("fig 0/21 i={}-{}-{}/{} Rfa0={} RM={} time_compensated={} AMSS_id={} freq={}",
                            curr_block, curr_fi_list, i, nb_entries, 
                            Rfa0, RM, is_time_compensated, 
                            amss_id, (float)(alt_freq)*1e-6f);
                        m_handler->OnFrequencyInformation_1_AMSS(
                            amss_id, alt_freq, is_time_compensated);
                    }
                    */
                }
                break;
            default:
                fprintf(stderr,"fig 0/21 Unknown RM value (%i)", RandM);
                
            }   
	   
	    base += 24 + length * 8;
	}
	         
	return upperLimit / 8;
}



/*
FIG 0/22: TII database (see clause 3.6.21 of TR 101 496-2 [3]) DAB - TMC can be more effectively
implemented if the position of the receiving vehicle and its driving direction were known from the Transmitter
Identification Information (TII). This would allow DAB-TMC decoders to automatically filter TMC messages
or to select messages, which are relevant within the locality. The TII provides the cross-reference between the
transmitter identifiers and the geographic locations of transmitters (expressed in grid co-ordinates)
*/
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
       fprintf(stderr, "FIB0/22: Id = %d, (%d %d)\n", mainId, latitudeCoarse, longitudeCoarse);
        (void)latitudeCoarse;
        (void)longitudeCoarse;
        return used + 48 / 6;
    }
    //  MS == 1

    noSubfields = getBits_3 (d, used * 8 + 13);
     fprintf(stderr, "fib-processor: Id = %d, subfields = %d\n", mainId, noSubfields);
    used += (16 + noSubfields * 48) / 8;

    return used;
}

/*
FIG 0/24: Other Ensembles services (OE services). This FIG provides a link between a DAB-TMC Service
component and other Ensembles carrying the same TMC Service component. Together with the Frequency
Information in FIG 0/21 it allows service following for the same DAB-TMC Service component on other
ensembles.
*/

void FIBProcessor::FIG0Extension24(uint8_t *d)
// FIG 0/24 8.1.10 OE services SI SIV OE P/D 
{
    int16_t Length  = getBits_5 (d, 3);
    int16_t offset  = 16;       // on bits
    uint8_t PD_bit  = getBits_1 (d, 8 + 2);
    uint8_t CN      = getBits_1 (d, 8 + 0);
    uint16_t eId  = getBits(d, 16, 16);
    int16_t used    = 2;
    uint32_t SId = getBits(d, offset, PD_bit == 1 ? 16 : 32);
    int16_t SCId            = getBits (d, used * 8, 12);
    int16_t CAOrgflag       = getBits_1 (d, used * 8 + 15);
    int16_t DGflag          = getBits_1 (d, used * 8 + 16);
    int16_t DSCTy           = getBits_6 (d, used * 8 + 18);
    int16_t SubChId         = getBits_6 (d, used * 8 + 24);
    int16_t packetAddress   = getBits (d, used * 8 + 30, 10);
    uint16_t CAOrg          = getBits (d, used * 8 + 40, 16);
    uint32_t   ecc = getBits_8(d, offset);   (void)ecc; // If the TMC service is a component of a programme service, then the ECC shall be carried in FIG 0/9 (Country, LTO and International table)
        //cId = getBits_4(d, lOffset + 1); // original
    uint16_t    cId = getBits_4(d, offset + 4);   // aus tq-dab
    //uint32_t    SId = getBits(d, offset, 32);  // The 32 bit SId identifies the DAB service carrying the TMC service, it is a data service
    
    //fprintf (stderr, "FIG0/24 ecc=%x, SId=%x, DSCTy=%d, packetAddress=%x, SCId=%x, SubChId=%x, DGflag=%x, laenge=%i\n",ecc,SId, DSCTy,packetAddress,SCId,SubChId,DGflag, Length);


//    while (used < Length) {
        //used = HandleFIG0Extension24 (d, used);
         //   if (DSCTy == 1) fprintf(stderr,"## DSCTy Traffic Message CHannel (TMC) gefunden\n");
          ///      if (DSCTy == 2) fprintf(stderr,"## DSCTy Emergency Warning System (EWS) gefunden\n");
         //         if (DSCTy == 59) fprintf(stderr,"## DSCTy ip data gefunden\n");
          //         if (DSCTy == 44) fprintf(stderr,"## DSCTy journaline data gefunden\n");
 //   if (DSCTy == 5) fprintf(stderr,"## DSCTy Transparent Data Channel   (TDC) gefunden\n");
 //   if (DSCTy == 60) fprintf(stderr,"## DSCTy Multimedia Object Transfer (MOT) gefunden\n");
 //   }
     (void)offset;
   
}

//  5.2.2.2 Labels: FIG type 1 data field 
void FIBProcessor::process_FIG1(uint8_t *d)
{
    uint32_t    SId = 0;
    int16_t     offset = 0;
    int16_t     zaehler = 0;
    Service    *service;
    ServiceComponent *component;
    uint8_t     pd_flag;
    uint8_t     SCidS;
    char        label[17];
    int		serviceIndex;
    std::string dataName;
    
    // FIG 1 first byte
    const uint8_t charSet = getBits_4(d, 8); // Charset: this 4-bit field shall identify a character set 
    const uint8_t Rfu = getBits_1(d, 8 + 4);  // Rfu: this 1-bit flag shall be reserved for future use.
    const uint8_t extension = getBits_3(d, 8 + 5);  // Extension: this 3-bit field, expressed as an unsigned binary number
    const uint8_t oe = getBits_1(d, 8 + 4);
    label[16]  = 0x00;
    (void)Rfu;
	(void)extension;
       if (oe == 1) {
        return;
    }
   // if (Rfu == 1) { // The Rfu bit is set to zero for the currently specified extension field and FIG type 1 field.
   //     return;
   // }





    switch (extension) {
        case 0: // ensemble label
            {
                const uint32_t EId = getBits(d, 16, 16);
                offset = 32;
                if ((charSet <= 16)) { // EBU Latin based repertoire             
                for (int i = 0; i < 16; i ++) {
                    label[i] = getBits_8 (d, offset+8*i);
                    
               }
                dataName = label;
                // std::clog << "fib-processor:" << "Ensemblename: " << label << std::endl;
              //  if (!oe and EId == ensembleId) {
                    ensembleLabel.fig1_flag = getBits(d, offset, 16);
                    ensembleLabel.fig1_label = label;
                    ensembleLabel.setCharset(charSet);
                    myRadioInterface.onSetEnsembleLabel(ensembleLabel);
             //  }
                }
                break;
            }

        case 1: // 16 bit Identifier field for service label
          {  
                       SId = getBits(d, 16, 16);
            offset  = 32;
            service = findServiceId(SId);
            if (service) {
                for (int i = 0; i < 16; i++) {
                    label[i] = getBits_8(d, offset);
                    offset += 8;
                }
                service->serviceLabel.fig1_flag = getBits(d, offset, 16);
                service->serviceLabel.fig1_label = label;
                service->serviceLabel.setCharset(charSet);
                // std::clog << "fib-processor:" << "FIG1/1: SId = %4x\t%s\n", SId, label) << std::endl;
            }
            break; 
          }
        case 3: // Region label
            {
                uint8_t region_id = getBits_6 (d, 16 + 2);
                offset = 24;
                for (int i = 0; i < 16; i ++) {
                    label[i] = getBits_8 (d, offset + 8 * i);
                }
                fprintf(stderr,"for region %u",region_id);
            
            }
             break;
                   

  // FIG 1/4: Service Component Label. To ensure consistency between the TMC service and the information
  // signalled in the DAB FIC, the Service Component Label carried in FIG 1/4 shall be identical to the TMC
  // Service Provider Name (SPN)

         case 4: // Component label
         {
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

            for (int i = 0; i < 16; i ++) {
                label[i] = getBits_8 (d, offset);
                offset += 8;
            }
            dataName = label;

            component = findComponent(SId, SCidS);
            if (component) {
                component->componentLabel.fig1_flag = getBits(d, offset, 16);
                component->componentLabel.setCharset(charSet);
                component->componentLabel.fig1_label = label;
            }

/*
            int16_t compIndex = findServiceComponent (SId, SCidS);
        	if (compIndex > 0) {
	            if (findServiceIdPOSStr (dataName) == -1) {
	                if (components [compIndex]. TMid == 0) {
	                    int subChId = components [compIndex]. subchannelId;
	                    if (subChId >= 0) {
	                      //  createService (dataName, dataName,  SId, SCidS);
	                        //  add_to_ensemble (dataName, SId, subChId);
	                    }
	                }
	            }
	        }
  */       

               
           //     fprintf(stderr,"%s %s",label,dataName);
        //   createService (dataName, dataName,  SId, SCidS);
           
           
          }  //        std::clog << "fib-processor:" << "FIG1/4: Sid = %8x\tp/d=%d\tSCidS=%1X\tflag=%8X\t%s\n",
            //                          SId, pd_flag, SCidS, flagfield, label) << std::endl;
            break;
            

        case 5: // 32 bit Identifier field for service label
        {
            SId = getBits(d, 16, 32);
            SCidS   = getBits(d, 20, 4);
            offset  = 48;
            service = findServiceId(SId);
            if (service) {
                for (int i = 0; i < 16; i ++) {
                    label[i] = getBits_8(d, offset);
                    offset += 8;
                }
                service->serviceLabel.fig1_flag = getBits(d, offset, 16);
                service->serviceLabel.fig1_label = label;
                service->serviceLabel.setCharset(charSet);
            //    fprintf(stderr,"%s %s",label,dataName);
               createService (dataName, dataName,  SId, 0);  // TPEG EPG
              myRadioInterface.onServiceDetected(SId);

            }
        }
            break;
                
        
        case 6: // XPAD label
        {
            uint8_t XPAD_aid;
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

            for (int i = 0; i < 16; i ++) {
                label[i] = getBits_8 (d, offset + 8 * i);
            }
            dataName = label;
             fprintf(stderr, "fib-processor: FIG1/6: SId = %8x\tp/d = %d\t SCidS = %1X\tXPAD_aid = %2u\t%s\n", SId, pd_flag, SCidS, XPAD_aid, dataName.c_str());
        }
            break;
        

        default:
             fprintf(stderr, "fib-processor: FIG1/%d: not handled now\n", extension);
            break;
    }
}

static void handle_ext_label_data_field(const uint8_t *f, uint8_t len_bytes,
        bool toggle_flag, uint8_t segment_index, uint8_t rfu,
        DabLabel& label)
{
    if (label.toggle_flag != toggle_flag) {
        label.segments.clear();
        label.extended_label_charset = CharacterSet::Undefined;
        label.toggle_flag = toggle_flag;
    }

    size_t len_character_field = len_bytes;

    if (segment_index == 0) {
        // Only if it's the first segment
        const uint8_t encoding_flag = (f[0] & 0x80) >> 7;
        const uint8_t segment_count = (f[0] & 0x70) >> 4;
        label.segment_count = segment_count + 1;

        if (encoding_flag) {
            label.extended_label_charset = CharacterSet::UnicodeUcs2;
        }
        else {
            label.extended_label_charset = CharacterSet::UnicodeUtf8;
        }

        if (rfu == 0) {
            // const uint8_t rfa = (f[0] & 0x0F);
            // const uint16_t char_flag = f[1] * 256 + f[2];

            if (len_bytes <= 3) {
                throw std::runtime_error("FIG2 label length too short");
            }

            f += 3;
            len_character_field -= 3;
        }
        else {
            // ETSI TS 103 176 draft V2.2.1 (2018-08) gives a new meaning to rfu
            // TODO const uint8_t text_control = (f[0] & 0x0F);

            if (len_bytes <= 1) {
                throw std::runtime_error("FIG2 label length too short");
            }

            f += 1;
            len_character_field -= 1;
        }

        label.fig2_rfu = rfu;
    }

    std::vector<uint8_t> labelbytes(f, f + len_character_field);
    label.segments[segment_index] = labelbytes;
}

// 5.2.2.3 Extended labels: FIG type 2 data field, UTF-8 or UCS2 Labels
void FIBProcessor::process_FIG2(uint8_t *d)
{
    // In order to reuse code with etisnoop, convert
    // the bit-vector into a byte-vector
   
    std::vector<uint8_t> fig_bytes;
  for (size_t i = 0; i < 30; i++) {
        fig_bytes.push_back(getBits_8(d, 8*i));
    }

    uint8_t *f = fig_bytes.data();

    const uint8_t figlen = f[0] & 0x1F; // 5bits, int16_t Length  = getBits_5 (d, 3);
    f++;

/*
Toggle flag: this bit shall be maintained in the same state for all segments of the same label. When a label is changed, 
this bit shall be inverted with respect to its previous state. When a label is repeated then this bit shall remain unchanged. 
*/
    const uint8_t toggle_flag = (f[0] & 0x80) >> 7; // 5.2.2.3 1bit
/*
Segment index: this 3-bit field, expressed as an unsigned binary number in the range 0 to 1, shall define the index of 
the segment field carried in the FIG type 2 field. 
*/
    const uint8_t segment_index = (f[0] & 0x70) >> 4; // 3bits
    /*
    Rfu: this 1-bit flag shall be reserved for future use. The Rfu bit shall be set to zero for the currently specified extension 
field and FIG type 2 field. 
    */
    const uint16_t rfu = (f[0] & 0x08) >> 3; // 1bit
    /*
    Extension: this 3-bit field, expressed as an unsigned binary number, shall identify one of 8 interpretations of the 
FIG type 2 field (see clause 8.1). Those extensions, which are not defined, are reserved for future use. 
    */
    const uint16_t ext = f[0] & 0x07;        // 3bits
    uint32_t SId = 0;
    uint8_t XPAD_aid;
    uint16_t offset = 0;
    uint16_t pd_flag = 0;
    uint16_t SCidS = 0;
    char        label[17];

   // fprintf(stderr, "fib-processor: FIG2/%d: SId = %8x\tp/d = %d\t SCidS = %1X\tXPAD_aid = %2u\t%s\n",ext, SId, pd_flag, SCidS, XPAD_aid, label);

    size_t identifier_len;
    switch (ext) {
        case 0: // Ensemble label 8.1.13 wie FIG1/0
            identifier_len = 2;
            break;
        case 1: // Programme service label 8.1.14.1 wie FIG1/1 
            identifier_len = 2;
            break;
        case 4: // Service component label 8.1.14.3  wie FIG1/4
            {
                uint8_t pd = (f[1] & 0x80) >> 7;
                identifier_len = (pd == 0) ? 3 : 5;
                break;
            }
        case 5: // Data service label 8.1.14.2 wie FIG1/5
            identifier_len = 4;
            break;
        case 6: // X-PAD User Application label 8.1.14.4 wie FIG1/6
            identifier_len = 4;
            break;
        default:
            return; // Unsupported
    }

    const size_t header_length = 1; // FIG data field header

    const uint8_t *figdata = f + header_length + identifier_len;
    const size_t data_len_bytes = figlen - header_length - identifier_len;

    // ext is followed by Identifier field of Type 2 field,
    // whose length depends on ext
    switch (ext) {
        case 0: // Ensemble label
            {   // ETSI EN 300 401 8.1.13
                uint16_t eid = f[1] * 256 + f[2];
                if (figlen <= header_length + identifier_len) {
                    std::clog << "FIG2/0 length error " << (int)figlen << std::endl;
                }
                else if (eid == ensembleId) {
                    handle_ext_label_data_field(figdata, data_len_bytes, toggle_flag, segment_index, rfu, ensembleLabel);
                }
                 break;
            }
           

        case 1: // Programme service label
            {   // ETSI EN 300 401 8.1.14.1
                uint32_t sid = f[1] * 256 + f[2];
                if (figlen <= header_length + identifier_len) {
                    std::clog << "FIG2/1 length error " << (int)figlen << std::endl;
                }
                else {
                    auto *service = findServiceId(sid);
                    if (service) {
                        handle_ext_label_data_field(figdata, data_len_bytes, toggle_flag, segment_index, rfu, service->serviceLabel);
                        //std::clog << "FIG2/1 figdata " << (int)figlen << std::endl;
                    }
                }
                break;
            }
            

        case 4: // Service component label
            {   // ETSI EN 300 401 8.1.14.3
                uint32_t SId;
                uint8_t pd    = (f[1] & 0x80) >> 7;
                uint8_t SCIdS =  f[1] & 0x0F;
                if (pd == 0) {
                    SId = f[2] * 256 + \
                          f[3];
                }
                else {
                    SId = ((uint32_t)f[2] << 24) |
                          ((uint32_t)f[3] << 16) |
                          ((uint32_t)f[4] << 8) |
                          ((uint32_t)f[5]);
                }
                if (figlen <= header_length + identifier_len) {
                    std::clog << "FIG2/4 length error " << (int)figlen << std::endl;
                }
                else {
                    auto *component = findComponent(SId, SCIdS);
                    if (component) {
                        handle_ext_label_data_field(figdata, data_len_bytes, toggle_flag, segment_index, rfu, component->componentLabel);
                        std::clog << "FIG2/1 figdata " << (int)figlen << std::endl;
                    }
                }
                break;
            }
            

        case 5: // Data service label
            {   // ETSI EN 300 401 8.1.14.2
                const uint32_t sid =
                    ((uint32_t)f[1] << 24) |
                    ((uint32_t)f[2] << 16) |
                    ((uint32_t)f[3] << 8) |
                    ((uint32_t)f[4]);

                if (figlen <= header_length + identifier_len) {
                    std::clog << "FIG2/5 length error " << (int)figlen << std::endl;
                }
                else {
                    auto *service = findServiceId(sid);
                    if (service) {
                        handle_ext_label_data_field(figdata, data_len_bytes, toggle_flag, segment_index, rfu, service->serviceLabel);
                        std::clog << "FIG2/5 figdata " << (int)figlen << std::endl;
                    }
                }
               break; 
            };
            

        case 6: // XPAD label aus FIG1/6 kopiert
          {
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

            for (int i = 0; i < 16; i ++) {
                label[i] = getBits_8 (d, offset + 8 * i);
            }
            std::clog << "FIG2/6 figdata " << (int)figlen << std::endl;
            break;
          }
        fprintf(stderr, "fib-processor: FIG2/6: SId = %8x\tp/d = %d\t SCidS = %1X\tXPAD_aid = %2u\t%s\n", SId, pd_flag, SCidS, XPAD_aid, label);
            
            
    }
    
}

// locate a reference to the entry for the Service serviceId
Service *FIBProcessor::findServiceId(uint32_t serviceId)
{
    for (size_t i = 0; i < services.size(); i++) {
        if (services[i].serviceId == serviceId) {
            return &services[i];
        }
    }

    return nullptr;
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

//  bindAudioService is the main processor for - what the name suggests -
//  connecting the description of audioservices to a SID
void FIBProcessor::bindAudioService(
        int8_t TMid,
        uint32_t SId,
        int16_t compnr,
        int16_t subChId,
        int16_t ps_flag,
        int16_t ASCTy)
{
    Service *s = findServiceId(SId);
    if (!s) return;

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

void FIBProcessor::bindDataStreamService(
        int8_t TMid,
        uint32_t SId,
        int16_t compnr,
        int16_t subChId,
        int16_t ps_flag,
        int16_t DSCTy)
{
    Service *s = findServiceId(SId);
    if (!s) return;

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

//      bindPacketService is the main processor for - what the name suggests -
//      connecting the service component defining the service to the SId,
///     Note that the subchannel is assigned through a FIG0/3
void FIBProcessor::bindPacketService(
        int8_t TMid,
        uint32_t SId,
        int16_t compnr,
        int16_t SCId,
        int16_t ps_flag,
        int16_t CAflag)
{
    Service *s = findServiceId(SId);
    if (!s) return;

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

//  bindDataStreamService is the main processor for - what the name suggests -
//  connecting the description of dataservices to a SID
void FIBProcessor::bindFIDCDataStreamService(
        int8_t TMid,
        uint32_t SId,
        int16_t compnr,
        int16_t subChId,
        int16_t ps_flag,
        int16_t DSCTy)
{
    Service *s = findServiceId(SId); // SId wie z.B. "d8e1"=Audio, "e0d020f0"=data
    if (!s) return;                     // SId ist nicht vorhanden, dann nicht einrichten

    if (std::find_if(components.begin(), components.end(), [&](const ServiceComponent& sc) {
                    return sc.SId == s->serviceId && sc.componentNr == compnr;
                }) == components.end()) {
        ServiceComponent newcomp;
        newcomp.inUse       = true;
        newcomp.TMid         = TMid;
        newcomp.SId          = SId;
        newcomp. SCIds		= 0;
        newcomp.subchannelId = subChId;
        newcomp.componentNr  = compnr;
        newcomp.PS_flag      = ps_flag;
        newcomp.DSCTy        = DSCTy;
        components.push_back(newcomp);  // packe Data an ServiceComponentliste
      fprintf (stderr, "  TMid %d, bind FIDC service (%8x), comp %d, DSCTy %d\n",TMid, SId, compnr,DSCTy);
    }
}

void FIBProcessor::dropService(uint32_t SId)
{
    std::stringstream ss;
    ss << "Dropping service " << SId;

    services.erase(std::remove_if(services.begin(), services.end(),
                [&](const Service& s) {
                    return s.serviceId == SId;
                }
                ), services.end());

    components.erase(std::remove_if(components.begin(), components.end(),
                [&](const ServiceComponent& c) {
                    const bool drop = c.SId == SId;
                    if (drop) {
                        ss << ", comp " << c.componentNr;
                    }
                    return drop;
                }
                ), components.end());

    // Check for orphaned subchannels
    for (auto& sub : subChannels) {
        if (sub.subChId == -1) {
            continue;
        }

        auto c_it = std::find_if(components.begin(), components.end(),
                [&](const ServiceComponent& c) {
                    return c.subchannelId == sub.subChId;
                });

        const bool drop = c_it == components.end();
        if (drop) {
            ss << ", subch " << sub.subChId;
            sub.subChId = -1;
        }
    }

    std::clog << ss.str() << std::endl;
}

void FIBProcessor::clearEnsemble()
{
    std::lock_guard<std::mutex> lock(mutex);
    components.clear();
    subChannels.resize(64);
    services.clear();
    serviceRepeatCount.clear();
    timeLastServiceDecrement = std::chrono::steady_clock::now();
    timeLastFCT0Frame = std::chrono::system_clock::now();
}

std::vector<Service> FIBProcessor::getServiceList() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return services;
}

Service FIBProcessor::getService(uint32_t sId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    auto srv = std::find_if(services.begin(), services.end(),
                [&](const Service& s) {
                    return s.serviceId == sId;
                });

    if (srv != services.end()) {
        return *srv;
    }
    else {
        return Service(0);
    }
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

std::chrono::system_clock::time_point FIBProcessor::getTimeLastFCT0Frame() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return timeLastFCT0Frame;
}

// ##################### Annoucement
std::string	FIBProcessor::announcements (uint8_t a) {
	switch (a) {
	   case 0:
	   default:
	      return std::string ("Alarm");

	   case 1:
	      return std::string ("Road Traffic Flash");

	   case 2:
	      return std::string ("Traffic Flash");

	   case 4:
	      return std::string ("Warning/Service");
	
	   case 8:
	      return std::string ("News Flash");

	   case 16:
	      return std::string ("Area Weather flash");

	   case 32:
	      return std::string ("Event announcement");

	   case 64:
	      return std::string ("Special Event");

	   case 128:
	      return std::string ("Programme Information");
	}
}


Cluster	*FIBProcessor::getCluster ( int16_t clusterId) {

    for (int i = 0; i < 64; i ++) {
        
        if ((ClusterConfigs.clusterTable [i]. inUse == true) && (ClusterConfigs.clusterTable [i]. clusterId == clusterId))
        {
	       //fprintf(stderr,"++++++++++++++++++++ clusterId=%x, i=%x\n",clusterId,i);
            return &(ClusterConfigs.clusterTable [i]);
        }
}

	for (int i = 0; i < 64; i ++) {
	   if (ClusterConfigs.clusterTable [i]. inUse == false) {
        //fprintf(stderr,"****************** clusterId=%x, i=%x   true\n",clusterId,i);
	      ClusterConfigs.clusterTable [i]. inUse = true;
	      ClusterConfigs.clusterTable [i]. clusterId = clusterId;
	      return &(ClusterConfigs.clusterTable [i]);
	   }
	}
	return &(ClusterConfigs.clusterTable [0]);	// cannot happen
}

// setCluster(announcement_ClusterID, serviceIndex, AsuFlags);
void	FIBProcessor::setCluster (int16_t clusterId, int16_t serviceIndex, uint16_t asuFlags) {

//	if (!syncReached ())
//	   return;
Cluster *myCluster = getCluster (clusterId);
	if (myCluster == nullptr) {
    //fprintf(stderr,"++++++++++++++++++++ clusterId=%x, nullptr=%x\n",clusterId,myCluster);
	   return;
    }
	if (myCluster -> flags != asuFlags) {
	   myCluster -> flags = asuFlags;
	}
    uint16_t aa = myCluster -> services. size ();
	for (uint16_t i = 0; i < aa; i ++) {
	   if (myCluster -> services [i] == serviceIndex) {
        //fprintf(stderr,"********return********** clusterId=%x, nullptr=%x asuFlags=%x serviceIndex=%x size=%x\n",clusterId,myCluster,asuFlags, serviceIndex, aa);
	      return;
       }
    }
	myCluster -> services. push_back (serviceIndex);
      //fprintf(stderr,"********push_back********** clusterId=%x, nullptr=%x asuFlags=%x serviceIndex=%x size=%x\n",clusterId,myCluster,asuFlags, serviceIndex, aa);
}


void	FIBProcessor::createService (const std::string &name, const std::string &shortName, uint32_t SId, int SCIds) {
	for (size_t i = 0; i < services.size(); i ++) {
	   if (services [i]. inUse == true) {
	      continue;
	   }
    
	//  fprintf(stderr,"\ncreateService Data*********%s %x******** \n",name.c_str(),i);
	   services [i]. inUse		= true;
	   services [i]. hasName	= true;
	//   services [i]. serviceLabel.fig1_label = name.c_str();
	   services [i]. shortName	= shortName.c_str();
	   services [i]. SId		= SId;
	   services [i]. SCIds		= SCIds;

	   services [i]. nrComps	= 0;
	   return;
	}   
}

// locate a reference to the entry for the Service serviceId
uint32_t FIBProcessor::findServiceIdPOS(uint32_t serviceId)
{
    for (size_t i = 0; i < services.size(); i++) {
        if (services[i].serviceId == serviceId) {     // if (ensemble -> services [i]. SId == SId)
	            return i;                    // return i;
        }
    }

    return -1;  // gesuchte SId nicht gefunden
}

bool	match (std::string s1, std::string s2) {
	if ((s1. length () != 16) || (s2. length () != 16)) 
	   fprintf (stderr, "%s %d %s %d\n", s1.data(), s1.length(), s2.data(), s2.length());
	return s1 == s2;
}

uint16_t	FIBProcessor::findServiceIdPOSStr(const std::string &s) {
for (size_t i = 0; i < services.size(); i++) {
    	  // if (!services [i]. inUse)
	      //    continue;
	   if (services[i].serviceLabel.fig1_label_utf8() == s)
           return i;
	}
	return -1;
}

uint32_t	FIBProcessor::findServiceIdSIdStr(const std::string &s) {
for (size_t i = 0; i < services.size(); i++) {
	   if (services[i].serviceLabel.fig1_label_utf8() == s)
           // services[i].shortName=s;
          //  return  services[i].SId;
	      return services[i].serviceId;
	}
	return -1;
}

//	find serviceComponent using the SId 
int	FIBProcessor::findPacketComponentSIDPOS( uint32_t SId) 
{
       int xx = 0;
    for (const auto& component : components) {
	   if (components[xx].TMid != 03) continue;
	   if (components[xx]. SId == SId) return xx;
       xx =xx+1;
    }
    return -1;
  }
