/*
 *    Copyright (C) 2018
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



#ifndef DECODER_DATA_ADAPTER_H
#define DECODER_DATA_ADAPTER_H

#include <memory>
#include <vector>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include "dab-data-processor.h"
#include "radio-controller.h"
#include <iostream>
#include "fib-processor.h"
#include "fic-handler.h"
#include <sqlite3.h>



// 555555555555555555555555555555555555555555555555555555555555
//  `DecoderDataAdapter::DecoderDataAdapter(ProgrammeHandlerInterface&, short, short, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)'
class DecoderDataAdapter: public DabDataProcessor
{
    public:
        DecoderDataAdapter(ProgrammeHandlerInterface& mr, int16_t bitDataRate, int16_t DSCTy, const std::string& dumpFileName);
        void  addtoDatenFrame	(uint8_t *data, int32_t fragmentSize, std::string dumpFileName , uint32_t DSCTy, uint32_t DGflag, uint32_t SId, uint32_t subChId,uint32_t appType);

        void handlePackets(uint8_t *dataL, uint16_t length);

        //	result handlers, wie in data-processor.h in qt-dab
    	void		handleTDCAsyncstream 	(uint8_t *data,size_t len){}
    	
    	 void		handlePacket		(uint8_t *data);
         
         void       write_hex      (uint8_t *data,uint16_t length);
         void write_dump_strings(uint8_t *temp_dump, uint16_t framebytes);
         void write_tpeg_strings(std::vector<uint8_t> temptpeg, uint16_t framebytes);
         void       show_ascii_hex_row  (uint8_t *data,uint16_t len,size_t pos);
         
        //
         
         int32_t handleFrame_type_0 (uint8_t *data, int32_t offset, int32_t length);
         int32_t handleFrame_type_1 (uint8_t *data, int32_t offset, int32_t length);
         int32_t handle_tpeg_tmc (std::vector<uint8_t> temptpeg, uint16_t framebytes);
         int32_t tpeg_tmc_sqlite (uint16_t location, uint16_t event) ;


         bool serviceComponentFrameheaderCRC (uint8_t *data, int16_t offset, int16_t maxL);
         
         void addOrReportDuplicate(std::vector<uint16_t> &array, std::unordered_map<uint16_t, int> &countMap, uint16_t value, uint16_t event);
         
         bool write_only_on_png(std::vector<std::string>& arraySTR, std::unordered_map<std::string, int>& countMapSTR, std::string dataSTR);
         
         void add_mscDatagroupTPEG(std::vector<uint8_t> m);

         void handle_TPEG_Message0 (uint8_t *data, int32_t offset, int32_t length);
         void handle_TPEG_Message1 (uint8_t *data, int32_t offset, int32_t length);

       // int sqlitecallback(void* data, int argc, char** argv, char** azColName){};

         void	add_mscDatagroup	(std::vector<uint8_t>);
         void	add_mscTPEG	(std::vector<uint8_t>){}
          
          
        uint16_t	bitDataRate;
    	ProgrammeHandlerInterface& myInterface;
    private:
        int frameErrorCounter = 0;

    	std::string	serviceName;
        std::string	subName;
        std::string	dumpFileName;
        std::vector<uint8_t>	series;
        std::string dumpDataFilename;
        bool		assembling;
        std::vector<uint8_t> AppVector;
        std::vector<uint8_t> FECVector;
	    uint16_t		packetAddress;
        std::vector<Subchannel> subChannelsData;
        struct FILEDeleter{ void operator()(FILE* fd){ if (fd) fclose(fd); }};
        std::unique_ptr<FILE, FILEDeleter> dumpFile;

        std::string audioFormat;
        
        channelDescriptor	channel;
        FIBProcessor* fibpr;
        FicHandler* fichdl;
        Service getService(uint32_t sId) const;

        
};
#endif // DECODER_DATA_ADAPTER_H

