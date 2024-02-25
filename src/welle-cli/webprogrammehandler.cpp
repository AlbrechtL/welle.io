/*
 *    Copyright (C) 2020
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
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
#include "webprogrammehandler.h"
#include <iostream>
#include <algorithm>
#include <functional>

#include <lame/lame.h>

using namespace std;

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

class IEncoder 
{
    public:
    virtual bool process_interleaved(std::vector<int16_t>& audioData) = 0;
    virtual ~IEncoder() = default;
};

#ifdef HAVE_FLAC
#include <FLAC++/encoder.h>
class FlacEncoder : protected FLAC::Encoder::Stream, public IEncoder
{
    private:
    std::function<void(const std::vector<uint8_t>& headerData, const std::vector<uint8_t>& data)> handlerFunc;
    std::vector<uint8_t> flacHeader;
    bool streamHeaderInitialised = false;
    // The audio decoders always upconvert to stereo
    const int channels = 2;

    public :
    FlacEncoder(int sample_rate, std::function<void(const std::vector<uint8_t>& headerData, const std::vector<uint8_t>& data)> handler) : handlerFunc(handler)
    {
        set_streamable_subset(true);
        set_channels(2);
        set_sample_rate(sample_rate);
        set_compression_level(5);
        // Header created during this call
        init();
        // Header data finished
        streamHeaderInitialised = true;

    }

    bool process_interleaved(std::vector<int16_t>& audioData) override
    {
        std::vector<int32_t> pcm_32(audioData.size());

        // Convert 16bit samples to 32bit samples 
        for(long unsigned int i = 0; i < audioData.size(); i++)
        {
            pcm_32[i] = (int)audioData[i];
        }

        return FLAC::Encoder::Stream::process_interleaved(pcm_32.data(), pcm_32.size()/channels);
    }

    FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], size_t bytes, uint32_t samples, uint32_t current_frame) override
    {
        if (!streamHeaderInitialised)
        {
            flacHeader.insert(flacHeader.end(), buffer, buffer + bytes);
        }
        else
        {
            std::vector<uint8_t> vectdata(buffer, buffer + bytes);
            handlerFunc(flacHeader, vectdata);
        }

        return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }

    ~FlacEncoder()
    {
        finish();
    }
};

#endif

class LameEncoder : public IEncoder {
    std::function<void(const std::vector<uint8_t>& headerData, const std::vector<uint8_t>& data)> handlerFunc;
    lame_t lame;
    // The audio decoders always upconvert to stereo
    const int channels = 2;

    public:

    LameEncoder(int sample_rate, std::function<void(const std::vector<uint8_t>& headerData, const std::vector<uint8_t>& data)> handler) : handlerFunc(handler)
    {
        lame = lame_init();
        lame_set_in_samplerate(lame, sample_rate);
        lame_set_num_channels(lame, channels);
        lame_set_VBR(lame, vbr_default);
        lame_set_VBR_q(lame, 2);
        lame_init_params(lame);
    }

    LameEncoder(const LameEncoder& other) = delete;
    LameEncoder& operator=(const LameEncoder& other) = delete;
    LameEncoder(LameEncoder&& other) = default;
    LameEncoder& operator=(LameEncoder&& other) = default;

    bool process_interleaved(std::vector<int16_t>& audioData) override
    {
        vector<uint8_t> mp3buf(16384);

        int written = lame_encode_buffer_interleaved(lame,
                audioData.data(), audioData.size()/channels,
                mp3buf.data(), mp3buf.size());

        if (written < 0) {
            cerr << "Failed to encode mp3: " << written << endl;
        }
        else if (written > (ssize_t)mp3buf.size()) {
            cerr << "mp3 encoder wrote more than buffer size!" << endl;
        }
        else if (written > 0) {
            mp3buf.resize(written);
            handlerFunc(std::vector<uint8_t>(), mp3buf); 
        }

        return true;
    }

    ~LameEncoder() {
        lame_close(lame);
    }
};


ProgrammeSender::ProgrammeSender(Socket&& s) :
    s(move(s))
{
}

ProgrammeSender::ProgrammeSender(ProgrammeSender&& other) :
    s(move(other.s))
{
}

ProgrammeSender& ProgrammeSender::operator=(ProgrammeSender&& other)
{
    s = move(other.s);
    other.running = false;
    return *this;
}

bool ProgrammeSender::send_stream(const std::vector<uint8_t>& headerdata, const std::vector<uint8_t>& mp3Data)
{
    if (not s.valid()) {
        return false;
    }

    const int flags = MSG_NOSIGNAL;
    ssize_t ret = 0;

    if (!headerSent)
    {
        ret = s.send(headerdata.data(), headerdata.size(), flags);
        headerSent = true;
    }

    ret = s.send(mp3Data.data(), mp3Data.size(), flags);

    if (ret == -1) {
        s.close();
        std::unique_lock<std::mutex> lock(mutex);
        running = false;
        lock.unlock();
        cv.notify_all();
        return false;
    }

    return true;
}

void ProgrammeSender::wait_for_termination() const
{
    std::unique_lock<std::mutex> lock(mutex);
    while (running) {
        cv.wait_for(lock, chrono::seconds(2));
    }
}

void ProgrammeSender::cancel()
{
    s.close();
    running = false;
}

WebProgrammeHandler::WebProgrammeHandler(uint32_t serviceId, OutputCodec codecID) :
    serviceId(serviceId), codec(codecID)
{
    const auto now = chrono::system_clock::now();
    time_label = now;
    time_label_change = now;
    time_mot = now;
    time_mot_change = now;
}

WebProgrammeHandler::WebProgrammeHandler(WebProgrammeHandler&& other) :
    serviceId(other.serviceId),
    codec(other.codec),
    senders(move(other.senders))
{
    other.senders.clear();
    other.serviceId = 0;

    const auto now = chrono::system_clock::now();
    time_label = now;
    time_label_change = now;
    time_mot = now;
    time_mot_change = now;
}

WebProgrammeHandler::~WebProgrammeHandler()
{

}

void WebProgrammeHandler::registerSender(ProgrammeSender *sender)
{
    std::unique_lock<std::mutex> lock(senders_mutex);
    senders.push_back(sender);
}

void WebProgrammeHandler::removeSender(ProgrammeSender *sender)
{
    std::unique_lock<std::mutex> lock(senders_mutex);
    senders.remove(sender);
}

bool WebProgrammeHandler::needsToBeDecoded() const
{
    std::unique_lock<std::mutex> lock(senders_mutex);
    return not senders.empty();
}

void WebProgrammeHandler::cancelAll()
{
    std::unique_lock<std::mutex> lock(senders_mutex);
    for (auto& s : senders) {
        s->cancel();
    }
}

WebProgrammeHandler::dls_t WebProgrammeHandler::getDLS() const
{
    dls_t dls;

    std::unique_lock<std::mutex> lock(stats_mutex);
    if (last_label_valid) {
        dls.label = last_label;
        dls.time = time_label;
        dls.last_changed = time_label_change;
    }

    return dls;
}

WebProgrammeHandler::mot_t WebProgrammeHandler::getMOT() const
{
    mot_t mot;

    std::unique_lock<std::mutex> lock(stats_mutex);
    if (last_mot_valid) {
        mot.data = last_mot;
        mot.time = time_mot;
        mot.last_changed = time_mot_change;
        mot.subtype = last_subtype;
    }
    return mot;
}

WebProgrammeHandler::xpad_error_t WebProgrammeHandler::getXPADErrors() const
{
    std::unique_lock<std::mutex> lock(stats_mutex);
    xpad_error_t r(xpad_error);
    return r;
}

WebProgrammeHandler::audiolevels_t WebProgrammeHandler::getAudioLevels() const
{
    std::unique_lock<std::mutex> lock(stats_mutex);
    audiolevels_t r(audiolevels);
    return r;
}

WebProgrammeHandler::errorcounters_t WebProgrammeHandler::getErrorCounters() const
{
    std::unique_lock<std::mutex> lock(stats_mutex);
    errorcounters_t r(errorcounters);
    return r;
}

void WebProgrammeHandler::onFrameErrors(int frameErrors)
{
    std::unique_lock<std::mutex> lock(stats_mutex);
    errorcounters.num_frameErrors += frameErrors;
    errorcounters.time = chrono::system_clock::now();
}

void WebProgrammeHandler::onNewAudio(std::vector<int16_t>&& audioData,
                int sampleRate, const string& m)
{
    rate = sampleRate;
    mode = m;

    if (audioData.empty()) {
        return;
    }

    int last_audioLevel_L = 0;
    int last_audioLevel_R = 0;
    {
        int16_t max_L = 0;
        int16_t max_R = 0;
        for (size_t i = 0; i < audioData.size()-1; i+=2) {
            max_L = std::max(max_L, audioData[i]);
            max_R = std::max(max_R, audioData[i+1]);
        }
        last_audioLevel_L = max_L;
        last_audioLevel_R = max_R;
    }

    {
        std::unique_lock<std::mutex> lock(stats_mutex);
        audiolevels.time = chrono::system_clock::now();
        audiolevels.last_audioLevel_L = last_audioLevel_L;
        audiolevels.last_audioLevel_R = last_audioLevel_R;
    }

    if (encoder == nullptr)
    {
        switch (codec)
        {
        case OutputCodec::MP3 :
            encoder = make_unique<LameEncoder>(rate, [&](const vector<uint8_t>& headerData, const vector<uint8_t>& vectData){send_to_all_clients(headerData, vectData);});
            break;
        #ifdef HAVE_FLAC
        case OutputCodec::FLAC :
            encoder = make_unique<FlacEncoder>(rate, [&](const vector<uint8_t>& headerData, const vector<uint8_t>& vectData){send_to_all_clients(headerData, vectData);});
            break;
        #endif
        default:
                throw runtime_error("OutputCodec not handled, did you compile with flac support ?");
            break;
        }
    }

    encoder->process_interleaved(audioData);

}

void WebProgrammeHandler::send_to_all_clients(const std::vector<uint8_t>& headerData, const std::vector<uint8_t>& data)
{
    std::unique_lock<std::mutex> lock(senders_mutex);

    for (auto& s : senders) {
        bool success = s->send_stream(headerData, data);
        if (not success) {
            cerr << "Failed to send audio for " << serviceId << endl;
        }
    }
}

void WebProgrammeHandler::onRsErrors(bool uncorrectedErrors, int numCorrectedErrors)
{
    (void)numCorrectedErrors; // TODO calculate BER before Reed-Solomon
    std::unique_lock<std::mutex> lock(stats_mutex);
    errorcounters.num_rsErrors += (uncorrectedErrors ? 1 : 0);
    errorcounters.time = chrono::system_clock::now();
}

void WebProgrammeHandler::onAacErrors(int aacErrors)
{
    std::unique_lock<std::mutex> lock(stats_mutex);
    errorcounters.num_aacErrors += aacErrors;
    errorcounters.time = chrono::system_clock::now();
}

void WebProgrammeHandler::onNewDynamicLabel(const string& label)
{
    std::unique_lock<std::mutex> lock(stats_mutex);
    last_label_valid = true;
    const auto now = chrono::system_clock::now();
    time_label = now;
    if (last_label != label) {
        time_label_change = now;
    }
    last_label = label;
}

void WebProgrammeHandler::onMOT(const mot_file_t& mot_file)
{
    std::unique_lock<std::mutex> lock(stats_mutex);
    last_mot_valid = true;
    const auto now = chrono::system_clock::now();
    time_mot = now;
    if (last_mot != mot_file.data) {
        time_mot_change = now;
    }
    last_mot = mot_file.data;
    if (mot_file.content_sub_type == 0x01) {
        last_subtype = MOTType::JPEG;
    }
    else if (mot_file.content_sub_type == 0x03) {
        last_subtype = MOTType::PNG;
    }
    else {
        last_subtype = MOTType::Unknown;
    }
}

void WebProgrammeHandler::onPADLengthError(size_t announced_xpad_len, size_t xpad_len)
{
    std::unique_lock<std::mutex> lock(stats_mutex);
    xpad_error.has_error = true;
    xpad_error.time = chrono::system_clock::now();
    xpad_error.announced_xpad_len = announced_xpad_len;
    xpad_error.xpad_len = xpad_len;
}

