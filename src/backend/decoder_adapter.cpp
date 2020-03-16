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

#include <iostream>
#include <vector>
#include "decoder_adapter.h"

DecoderAdapter::DecoderAdapter(ProgrammeHandlerInterface &mr, int16_t bitRate, AudioServiceComponentType &dabModus, const std::string &dumpFileName):
    bitRate(bitRate),
    myInterface(mr),
    padDecoder(this, true)
{
    if (dabModus == AudioServiceComponentType::DAB)
        decoder = std::make_unique<MP2Decoder>(this, false);
    else if (dabModus == AudioServiceComponentType::DABPlus)
        decoder = std::make_unique<SuperframeFilter>(this, true, false);
    else
        throw std::runtime_error("DecoderAdapter: Unkonwn service component");

    // Open a dump file (XPADxpert) if the user defined it
    if (!dumpFileName.empty()) {
        FILE *fd = fopen(dumpFileName.c_str(), "wb");
        // w for write, b for binary
        if (fd != nullptr) {
            dumpFile.reset(fd);
        }
    }

    // MOT, start of X-PAD data group, see EN 301 234
    padDecoder.SetMOTAppType(12);
}

void DecoderAdapter::addtoFrame(uint8_t *v)
{
    const size_t length = 24 * bitRate / 8;
    std::vector<uint8_t> data(length);

    // Convert 8 bits (stored in one uint8) into one uint8
    for (size_t i = 0; i < length; i ++) {
        data[i] = 0;
        for (int j = 0; j < 8; j ++) {
            data[i] <<= 1;
            data[i] |= v[8 * i + j] & 01;
        }
    }

    decoder->Feed(data.data(), length);

    if (dumpFile) {
        fwrite(data.data(), length, 1, dumpFile.get());
    }

    myInterface.onFrameErrors(frameErrorCounter);
    frameErrorCounter = 0;
}

void DecoderAdapter::FormatChange(const AUDIO_SERVICE_FORMAT& format)
{
    audioFormat = format.GetSummary();
}

void DecoderAdapter::StartAudio(int samplerate, int channels, bool float32)
{
    if (float32 == true)
        throw std::runtime_error("DecoderAdapter: Float32 audio samples are not supported");

    audioSamplerate = samplerate;
    audioChannels = channels;
}

void DecoderAdapter::PutAudio(const uint8_t *data, size_t len)
{
    // Then len is given in bytes. For stereo it is the double times of mono.
    // But we need two channels even if we have mono.
    // Mono: len = len / 2 * 2 We have len to devide by 2 and for two channels we have multiply by two
    // Stereo: len = len / 2 We just need to devide by 2 because it is stereo
    size_t bufferSize = audioChannels == 2 ? len/2 : len;
    std::vector<int16_t> audio(bufferSize);

    // Convert two uint8 into a int16 sample
    for(size_t i=0; i<len/2; ++i) {
        int16_t sample =  ((int16_t) data[i * 2 +1] << 8) | ((int16_t) data[i * 2]);

        if (audioChannels == 2) {
            audio[i] = sample;
        }
        else {
        	// upmix to stereo
            audio[i*2] = sample;
            audio[i*2+1] = sample;
        }
    }

    myInterface.onNewAudio(
        std::move(audio),
        audioSamplerate,
        audioFormat);
}

void DecoderAdapter::ProcessPAD(const uint8_t *xpad_data, size_t xpad_len, bool exact_xpad_len, const uint8_t *fpad_data)
{
    padDecoder.Process(xpad_data, xpad_len, exact_xpad_len, fpad_data);
}

void DecoderAdapter::AudioError(const std::string &hint)
{
    (void)hint;
    frameErrorCounter++;
}

void DecoderAdapter::AudioWarning(const std::string &hint)
{
    (void)hint;
    myInterface.onAacErrors(1);
}

void DecoderAdapter::FECInfo(int total_corr_count, bool uncorr_errors)
{
    myInterface.onRsErrors(uncorr_errors, total_corr_count);
}

void DecoderAdapter::PADChangeDynamicLabel(const DL_STATE &dl)
{
    if (dl.raw.empty()) {
        myInterface.onNewDynamicLabel("");
    }
    else {
        myInterface.onNewDynamicLabel(
                toUtf8StringUsingCharset(
                    dl.raw.data(),
                    (CharacterSet)dl.charset,
                    dl.raw.size()));
    }
}

void DecoderAdapter::PADChangeSlide(const MOT_FILE &slide)
{
    mot_file_t mot_file;

    mot_file.data = slide.data;
    mot_file.content_sub_type = slide.content_sub_type;
    mot_file.content_name = slide.content_name;
    mot_file.click_through_url = slide.click_through_url;
    mot_file.category = slide.category;
    mot_file.slide_id = slide.slide_id;
    mot_file.category_title = slide.category_title;

    myInterface.onMOT(mot_file);
}

void DecoderAdapter::PADLengthError(size_t announced_xpad_len, size_t xpad_len)
{
    myInterface.onPADLengthError(announced_xpad_len, xpad_len);
}
