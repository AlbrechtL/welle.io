/*
 * @class AudioDecoder
 * @brief Facade class for audio playing
 *
 * @author Jaroslaw Bulat kwant@agh.edu.pl (AudioDecoder)
 * @author Kacper Patro patro.kacper@gmail.com (AudioDecoder)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Kacper Patro
 * @par License
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "audio_decoder.h"

AudioDecoder::AudioDecoder(float threshold, size_t length, int type) {
    src_ = new RingSrc(threshold, length);
    sink_ = new NullSink();

    player_ = new Player(src_, type);
    player_->AddSink(sink_);
}

AudioDecoder::~AudioDecoder() {
    delete player_;
    delete sink_;
    delete src_;
}

void AudioDecoder::RemoveSink(AbstractSink *sink) {
    player_->RemoveSink(sink);
}

AbstractSink *AudioDecoder::AddSink(AbstractSink *sink) {
    return player_->AddSink(sink);
}

size_t AudioDecoder::Write(uint8_t *buffer, size_t length) {
    return src_->Write(buffer, length);
}

void AudioDecoder::LastFrame() {
    src_->set_last_frame(true);
}

void AudioDecoder::RegisterTagsMapCallback(TagsMapCallback cb_func, void *cb_data) {
    player_->RegisterTagsMapCallback(cb_func, cb_data);
}

void AudioDecoder::Process() {
    player_->Process();
}

int AudioDecoder::PlayerType() const {
    return player_->type();
}

void AudioDecoder::Flush() {
    src_->set_flush(true);
}

void AudioDecoder::RegisterReadCallback(ReadCallback cb_func, void *cb_data) {
    src_->RegisterReadCallback(cb_func, cb_data);
}
