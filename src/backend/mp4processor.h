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

#ifndef MP4PROCESSOR
#define MP4PROCESSOR
/*
 *  Handling superframes for DAB+ and delivering
 *  frames into the ffmpeg or faad decoding library
 */
//
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdint>
#include "dab-constants.h"
#include "dab-processor.h"
#include "pad_decoder.h"
#include "radio-controller.h"
#include "subchannel_sink.h"
#include "dabplus_decoder.h"

class Mp4Processor : public DabProcessor,  public SubchannelSinkObserver, public PADDecoderObserver
{
    public:
        Mp4Processor(ProgrammeHandlerInterface& phi,
                int16_t bitRate,
                const std::string& mscFileName);

        virtual void addtoFrame(uint8_t *v);

        // SubchannelSinkObserver impl
        virtual void FormatChange(const std::string& /*format*/);
        virtual void StartAudio(int /*samplerate*/, int /*channels*/, bool /*float32*/);
        virtual void PutAudio(const uint8_t* /*data*/, size_t /*len*/);
        virtual void ProcessPAD(const uint8_t* /*xpad_data*/, size_t /*xpad_len*/, bool /*exact_xpad_len*/, const uint8_t* /*fpad_data*/);

        // PADDecoderObserver impl
        virtual void PADChangeDynamicLabel(const DL_STATE& dl);
        virtual void PADChangeSlide(const MOT_FILE& slide);
        virtual void PADLengthError(size_t announced_xpad_len, size_t xpad_len);

    private:
        ProgrammeHandlerInterface& myInterface;

        struct FILEDeleter{ void operator()(FILE* fd){ if (fd) fclose(fd); }};
        std::unique_ptr<FILE, FILEDeleter> mscFile;

        PADDecoder padDecoder;
        std::unique_ptr<SuperframeFilter> mp4Decoder;

        int16_t bitRate;
        int audioSamplerate;
        int audioChannels;
        int audioSampleSize;
};

#endif


