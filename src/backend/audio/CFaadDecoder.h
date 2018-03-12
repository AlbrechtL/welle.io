/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
 *    The ringbuffer here is a rewrite of the ringbuffer used in the PA code
 *    All rights remain with their owners
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
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <memory>
#include "neaacdec.h"
#include "ringbuffer.h"

class CFaadDecoder
{
public:
    CFaadDecoder(std::shared_ptr<RingBuffer<int16_t>> buffer);
    ~CFaadDecoder(void);
    CFaadDecoder(const CFaadDecoder& other) = delete;
    CFaadDecoder& operator=(const CFaadDecoder& other) = delete;

    int get_aac_channel_configuration(
            int16_t m_mpeg_surround_config,
            uint8_t aacChannelMode);

    int16_t MP42PCM(uint8_t dacRate, uint8_t sbrFlag,
            int16_t mpegSurround,
            uint8_t aacChannelMode,
            uint8_t buffer[],
            int16_t bufferLength,
            uint32_t *sampleRate = nullptr, bool *isParametricStereo = nullptr);

private:
    bool                     processorOK;
    bool                     aacInitialized;
    uint32_t                 aacCap;
    NeAACDecHandle           aacHandle;
    NeAACDecConfigurationPtr aacConf;
    NeAACDecFrameInfo        hInfo;
    std::shared_ptr<RingBuffer<int16_t>> audioBuffer;
};
