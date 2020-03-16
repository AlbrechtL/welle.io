/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
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

#ifndef __VIRTUAL_INPUT
#define __VIRTUAL_INPUT

#include <memory>
#include <fstream>
#include <iostream>

#include "dab-constants.h"
#include "radio-controller.h"
#include "ringbuffer.h"

enum class CDeviceID {
    UNKNOWN, NULLDEVICE, AIRSPY, RAWFILE, RTL_SDR, RTL_TCP, SOAPYSDR, ANDROID_RTL_SDR, LIMESDR};

class CVirtualInput : public InputInterface {
public:
    virtual ~CVirtualInput() {}
    virtual CDeviceID getID(void) = 0;

    void writeRecordBufferToFile(std::string &fileanme) {
        if(!recordBuffer)
            return;

        std::ofstream rawStream(fileanme, std::ios::binary);

        while (1) {
            uint8_t data_tmp[1024];
            const auto available = recordBuffer->GetRingBufferReadAvailable();

            if (available <= 0) {
                break;
            }

            const size_t data_tmpSize = std::min(sizeof(data_tmp), (size_t)available);
            recordBuffer->getDataFromBuffer(data_tmp, data_tmpSize);
            rawStream.write((char*)data_tmp, data_tmpSize);
        }

        rawStream.close();
    }

    void initRecordBuffer(uint32_t size) {
        // The ring buffer size has to be power of 2
        uint32_t bitCount = ceil(log2(size));
        uint32_t bufferSize = pow(2, bitCount);

        try {
            recordBuffer.reset(new RingBuffer<uint8_t>(bufferSize));
        }

        catch (const std::bad_alloc& e) {
                std::clog << "CVirtualInput: recordBuffer allocation failed (size " << bufferSize * sizeof(uint8_t) << " bytes) : " << e.what() << std::endl;
        }
    }

protected:
    void putIntoRecordBuffer(uint8_t &data, uint32_t size) {
        if(!recordBuffer)
            return;

        recordBuffer->putDataIntoBuffer(&data, static_cast<int>(size));
        //std::clog << "CVirtualInput: GetRingBufferReadAvailable() " << recordBuffer->GetRingBufferReadAvailable() << std::endl;
    }

private:
    std::unique_ptr<RingBuffer<uint8_t>> recordBuffer;
};

#endif
