/*
 * @class OggSink
 *
 * @author: Kacper Patro patro.kacper@gmail.com
 * @date May 29, 2015
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

#include "ogg_sink.h"

GstPadProbeReturn OggSinkCallbacks::UnlinkCallOggSink(GstPad *pad, GstPadProbeInfo *info, gpointer container_ptr) {
    OggSink::Data *data = reinterpret_cast<OggSink::Data *>(container_ptr);

    GstPad *sinkpad;

    if(!g_atomic_int_compare_and_exchange(&data->removing, FALSE, TRUE))
        return GST_PAD_PROBE_OK;

    sinkpad = gst_element_get_static_pad(data->queue, "sink");
    gst_pad_unlink(data->teepad, sinkpad);
    gst_object_unref(sinkpad);

    gst_bin_remove(GST_BIN(data->player_data->pipeline), data->queue);
    gst_bin_remove(GST_BIN(data->player_data->pipeline), data->encoder);
    gst_bin_remove(GST_BIN(data->player_data->pipeline), data->muxer);
    gst_bin_remove(GST_BIN(data->player_data->pipeline), data->sink);

    gst_element_set_state(data->sink, GST_STATE_NULL);
    gst_element_set_state(data->muxer, GST_STATE_NULL);
    gst_element_set_state(data->encoder, GST_STATE_NULL);
    gst_element_set_state(data->queue, GST_STATE_NULL);

    gst_object_unref(data->sink);
    gst_object_unref(data->muxer);
    gst_object_unref(data->encoder);
    gst_object_unref(data->queue);

    gst_element_release_request_pad(data->player_data->tee, data->teepad);
    gst_object_unref(data->teepad);

    data->linked = false;

    return GST_PAD_PROBE_REMOVE;
}

OggSink::OggSink(const char *path):
    data_(new Data),
    path_(path) {
        data_->abstract_sink = this;
        data_->linked = false;
        data_->player_data = NULL;
    }

OggSink::~OggSink() {
    delete data_;
}

void OggSink::InitSink(void *player_data) {
    if(linked())
        return;

    data_->player_data = reinterpret_cast<Player::Data *>(player_data);

    GstPad *sinkpad;
    GstPadTemplate *templ;

    templ = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(data_->player_data->tee), "src_%u");
    data_->teepad = gst_element_request_pad(data_->player_data->tee, templ, NULL, NULL);

    char buff[100];

    strcpy(buff, name());
    strcat(buff, "_queue");

    data_->queue = gst_element_factory_make("queue", buff);
    g_assert(data_->queue);

    strcpy(buff, name());
    strcat(buff, "_encoder");

    data_->encoder = gst_element_factory_make("vorbisenc", buff);
    g_assert(data_->encoder);

    strcpy(buff, name());
    strcat(buff, "_muxer");

    data_->muxer = gst_element_factory_make("oggmux", buff);
    g_assert(data_->muxer);

    strcpy(buff, name());
    strcat(buff, "_sink");

    data_->sink = gst_element_factory_make("filesink", buff);
    g_assert(data_->sink);

    g_object_set(data_->sink, "location", path_, NULL);
    g_object_set(data_->sink, "async", FALSE, NULL);

    data_->removing = FALSE;

    gst_object_ref(data_->queue);
    gst_object_ref(data_->encoder);
    gst_object_ref(data_->muxer);
    gst_object_ref(data_->sink);

    gst_bin_add_many(GST_BIN(data_->player_data->pipeline),
            data_->queue,
            data_->encoder,
            data_->muxer,
            data_->sink,
            NULL);

    g_assert(gst_element_link_many(
                data_->queue,
                data_->encoder,
                data_->muxer,
                data_->sink,
                NULL)
            );

    gst_element_sync_state_with_parent(data_->queue);
    gst_element_sync_state_with_parent(data_->encoder);
    gst_element_sync_state_with_parent(data_->muxer);
    gst_element_sync_state_with_parent(data_->sink);

    sinkpad = gst_element_get_static_pad(data_->queue, "sink");
    gst_pad_link(data_->teepad, sinkpad);
    gst_object_unref(sinkpad);

    data_->linked = true;
}

const char *OggSink::name() const {
    return "ogg_sink";
}

void OggSink::Finish() {
    if(!linked()) {
        return;
    }

    gst_pad_add_probe(data_->teepad, GST_PAD_PROBE_TYPE_IDLE, OggSinkCallbacks::UnlinkCallOggSink, data_, NULL);
}

bool OggSink::linked() const {
    return data_->linked;
}
