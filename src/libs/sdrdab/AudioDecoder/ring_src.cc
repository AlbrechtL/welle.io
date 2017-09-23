/*
 * @class RingSrc
 *
 * @author: Kacper Patro patro.kacper@gmail.com
 * @date May 17, 2015
 *
 * @version 1.0 beta
 * @copyright Copyright (c) 2015 Kacper Patro
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
 *
 */

#include "ring_src.h"
#include <gst/app/app.h>

#define BOUND 0.001 //upper/lower bound of playback speed, relative to 1.0
#define RESOLUTION 0.000001 //resolution of pitch increments/decrements

#define APP_SRC_BUFF_PERCENT 100    //percent of appsrc buffer fullness to emit need-data
#define APP_SRC_BUFF_DIVIDER 250    //divider of ring buffer length to obtain appsrc buffer length
#define BUFF_CHUNK_DIVIDER 10000    //divider of ring buffer length to obtain single chunk length

RingSrc::RingSrc(float threshold, size_t buffer_length):
    data_(new Data),
    threshold_(threshold),
    current_ratio_(1.0),
    flush_(false),
    read_cb_(NULL),
    read_cb_data_(NULL),
    buff_size_(buffer_length),
    buff_chunk_size_(buffer_length/BUFF_CHUNK_DIVIDER),
    ring_buffer_(new BlockingRingBuffer(buffer_length/BUFF_CHUNK_DIVIDER, buffer_length)) {
        data_->abstract_src = this;
        data_->player_data = NULL;

        upper_bound_ = ParseThreshold(0.5+threshold_);
        lower_bound_ = ParseThreshold(0.5-threshold_);
    }

RingSrc::~RingSrc() {
    delete data_;
    delete ring_buffer_;
}

const char *RingSrc::name() const {
    return "app_src";
}

gboolean RingSrcCallbacks::ReadData(gpointer src_data_ptr) {
    RingSrc::Data *src_data = reinterpret_cast<RingSrc::Data *>(src_data_ptr);

    GstBuffer *buffer;
    GstMapInfo map;
    uint8_t *it;
    size_t size;
    GstFlowReturn ret;

    buffer = gst_buffer_new_allocate(NULL, src_data->abstract_src->buff_chunk_size_, NULL);
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    it = reinterpret_cast<uint8_t *>(map.data);

    size = src_data->abstract_src->ring_buffer_->ReadFrom(it, src_data->abstract_src->buff_chunk_size_);
    gst_buffer_set_size(buffer, size);

    gst_buffer_unmap(buffer, &map);

    if(!size || src_data->abstract_src->flush()) {
        gst_buffer_unref(buffer);
        gst_app_src_end_of_stream(reinterpret_cast<GstAppSrc *>(src_data->src));
        return FALSE;
    }

    ret = gst_app_src_push_buffer(reinterpret_cast<GstAppSrc *>(src_data->src), buffer);

    if(ret !=  GST_FLOW_OK) {
        return FALSE;
    }

    if(src_data->abstract_src->read_cb_ != NULL)
        src_data->abstract_src->read_cb_(src_data->abstract_src->ring_buffer()->DataStored(), src_data->abstract_src->read_cb_data_);
    else
        src_data->abstract_src->ProcessThreshold();

    return TRUE;
}


void RingSrcCallbacks::StartFeed(GstElement *pipeline, guint size, gpointer src_data_ptr) {
    RingSrc::Data *src_data = reinterpret_cast<RingSrc::Data *>(src_data_ptr);

    if(src_data->source_id == 0) {
        src_data->source_id = g_idle_add(reinterpret_cast<GSourceFunc>(ReadData), src_data_ptr);
    }
}

void RingSrc::LinkSrc() {
    g_assert(gst_element_link_many(
                data_->src,
                data_->player_data->iddemux,
                NULL)
            );
}

bool RingSrc::flush() const {
    return flush_;
}

void RingSrc::set_flush(bool to) {
    flush_ = to;
    if(to == true) {
        ring_buffer_->set_force_read(true);
        ring_buffer_->ForceSignal();
    }
}

void RingSrcCallbacks::StopFeed(GstElement *pipeline, gpointer src_data_ptr) {
    RingSrc::Data *src_data = reinterpret_cast<RingSrc::Data *>(src_data_ptr);

    if (src_data->source_id != 0) {
        g_source_remove(src_data->source_id);
        src_data->source_id = 0;
    }
}

void RingSrc::SetSrc(void *player_data) {
    data_->player_data = reinterpret_cast<Player::Data *>(player_data);

    data_->src = gst_element_factory_make("appsrc", "src");
    g_assert(data_->src);
    data_->source_id = 0;

    g_signal_connect(data_->src, "need-data", G_CALLBACK(RingSrcCallbacks::StartFeed), data_);
    g_signal_connect(data_->src, "enough-data", G_CALLBACK(RingSrcCallbacks::StopFeed), data_);

    g_object_set(data_->src, "max-bytes", buff_size_/APP_SRC_BUFF_DIVIDER, NULL);
    g_object_set(data_->src, "min-percent", APP_SRC_BUFF_PERCENT, NULL);

    gst_bin_add_many(GST_BIN(data_->player_data->pipeline),
            data_->src,
            NULL
            );
}

float RingSrc::DecrementRatio() {
    current_ratio_ -= RESOLUTION;
    if(current_ratio_<(1-BOUND))
        current_ratio_ = 1-BOUND;

    data_->player_data->player->set_playback_speed(current_ratio_);

    return current_ratio_;
}

float RingSrc::IncrementRatio() {
    current_ratio_ += RESOLUTION;
    if(current_ratio_>(1+BOUND))
        current_ratio_ = 1+BOUND;

    data_->player_data->player->set_playback_speed(current_ratio_);

    return current_ratio_;
}

size_t RingSrc::ParseThreshold(float fraction) {
    return buff_size_*fraction;
}

void RingSrc::ProcessThreshold() {
    if(data_->player_data->player->ready()) {
        //float ratio;

        if(ring_buffer_->DataStored()<lower_bound_) {
            DecrementRatio();
            //g_warning("current ratio: %f\n", ratio);
            return;
        }

        if(ring_buffer_->DataStored()>upper_bound_) {
            IncrementRatio();
            //g_warning("current ratio: %f\n", ratio);
            return;
        }
    }
}

size_t RingSrc::Write(uint8_t *buffer, size_t length) {
    return ring_buffer_->WriteInto(buffer, length);
}

void RingSrc::set_last_frame(bool to) {
    ring_buffer_->set_last_frame(to);
    if(to == true)
        ring_buffer_->ForceSignal();
}

const BlockingRingBuffer *RingSrc::ring_buffer() const {
    return ring_buffer_;
}

void RingSrc::ResetSrc() {
    ring_buffer_->Reset();
    data_->source_id = 0;
    set_flush(false);
}

bool RingSrc::last_frame() const {
    return ring_buffer_->last_frame();
}

void RingSrc::RegisterReadCallback(ReadCallback cb_func, void *cb_data) {
    read_cb_ = cb_func;
    read_cb_data_ = cb_data;
}
