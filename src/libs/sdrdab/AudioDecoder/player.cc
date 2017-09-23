/*
 * @class Player
 *
 * @author: Kacper Patro patro.kacper@gmail.com
 * @date May 23, 2015
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

#include "player.h"
#include <sstream>

void PlayerCallbacks::SaveTags(const GstTagList *list, const gchar *tag, gpointer player_ptr) {
    Player *player = reinterpret_cast<Player *>(player_ptr);
    std::map<const std::string, std::string>::iterator t_it;
    std::map<const std::string, TagsMapParser>::iterator f_it;

    gint count = gst_tag_list_get_tag_size(list, tag);

    for(gint i = 0; i < count; i++) {
        if((t_it = player->tags_map_->find(gst_tag_get_nick(tag))) != player->tags_map_->end()) {   //if we are interested in this tag
            if((f_it = player->tags_map_parsers_->find(gst_tag_get_nick(tag))) != player->tags_map_parsers_->end()) //if this tag has parser function
                f_it->second(list, tag, i, &t_it->second);
        }
    }

    if(player->tags_map_cb_ != NULL) {
        player->tags_map_cb_(player->tags_map_, player->tags_map_cb_data_);
        /**
         * @todo Tag map should be wiped after callback?
         */
        //for(it = player->tags_map_->begin(); it != player->tags_map_->end(); it++)
        //it->second = "";
    }
}

gboolean PlayerCallbacks::BusCall(GstBus *bus, GstMessage *message, gpointer container_ptr) {
    Player::Data *player_data = reinterpret_cast<Player::Data *>(container_ptr);

    switch(GST_MESSAGE_TYPE(message)) {

        case GST_MESSAGE_ERROR: {
                                    gchar *debug;
                                    GError *err;

                                    gst_message_parse_error(message, &err, &debug);
                                    g_print("GStreamer error: %s\n", err->message);
                                    g_error_free(err);
                                    g_free(debug);

                                    g_main_loop_quit(player_data->loop);

                                    break;
                                }

        case GST_MESSAGE_WARNING: {
                                      gchar *debug;
                                      GError *err;

                                      gst_message_parse_warning(message, &err, &debug);
                                      g_print("GStreamer warning: %s\n", err->message);
                                      g_error_free(err);
                                      g_free(debug);

                                      break;
                                  }

        case GST_MESSAGE_ASYNC_DONE:
                                  //g_print ("GStreamer: %s prerolled, lock'n'load\n", GST_OBJECT_NAME(message->src));
                                  player_data->async_ready = true;

                                  break;

        case GST_MESSAGE_TAG: {
                                  GstTagList *tags = NULL;

                                  gst_message_parse_tag(message, &tags);
                                  gst_tag_list_foreach(tags, PlayerCallbacks::SaveTags, player_data->player);

                                  gst_tag_list_free(tags);

                                  break;
                              }

        case GST_MESSAGE_EOS:
                              g_main_loop_quit(player_data->loop);

                              break;

                              //case GST_MESSAGE_STATE_CHANGED:
                              //GstState old_state, new_state, pending_state;
                              //gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
                              //g_print("%s state changed from %s to %s\n",
                              //GST_MESSAGE_SRC_NAME(message), gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
                              //break;

        default:
                              break;

    }

    return TRUE;
}

Player::Player(AbstractSrc *src, int type):
    type_(type),
    tags_map_(new std::map<const std::string, std::string>),
    tags_map_parsers_(new std::map<const std::string, TagsMapParser>),
    abstract_src_(src),
    abstract_sinks_(new std::list<AbstractSink *>),
    tags_map_cb_(NULL),
    tags_map_cb_data_(NULL) {
        gst_init (NULL, NULL);

        data_.player = this;
        data_.async_ready = false;

        Init();
    }

Player::~Player() {
    gst_element_set_state(data_.pipeline, GST_STATE_NULL);

    std::list<AbstractSink *>::iterator it;
    it = abstract_sinks_->begin();

    while(it != abstract_sinks_->end()) {
        (*it)->Finish();
        it = abstract_sinks_->erase(it);
    }
    delete abstract_sinks_;

    gst_object_unref(data_.pipeline);
    delete tags_map_parsers_;
    delete tags_map_;
}

void Player::Process() {
    if(!abstract_sinks_->size()) {
        g_print("GStreamer: no sinks\n");
        return;
    }

    std::list<AbstractSink *>::iterator it;

    for(it = abstract_sinks_->begin(); it != abstract_sinks_->end(); it++) {
        if(!(*it)->linked())
            (*it)->InitSink(&data_);
    }

    gst_element_set_state(data_.pipeline, GST_STATE_PLAYING);
    data_.loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data_.loop);
    g_main_loop_unref(data_.loop);

    for(it = abstract_sinks_->begin(); it != abstract_sinks_->end(); it++)
        (*it)->Finish();

    gst_element_set_state(data_.pipeline, GST_STATE_READY);

    abstract_src_->ResetSrc();

    data_.async_ready = false;
}

void Player::ConstructObjects() {
    data_.pipeline = gst_pipeline_new("player");

    data_.iddemux = gst_element_factory_make("id3demux", "tag_demuxer");

    switch(type_) {

        case PLAYER_MPEG:
            data_.parser = gst_element_factory_make("mpegaudioparse", "parser");
            data_.decoder = gst_element_factory_make("mad", "decoder");
            break;

        case PLAYER_AAC:
        default:
            data_.parser = gst_element_factory_make("aacparse", "parser");
            data_.decoder = gst_element_factory_make("faad", "decoder");
            break;

    }

    data_.pitch = gst_element_factory_make("pitch", "pitch");
    data_.converter = gst_element_factory_make("audioconvert", "converter");

    data_.tee = gst_element_factory_make("tee", "tee");

    g_assert(data_.pipeline
            && data_.iddemux
            && data_.decoder
            && data_.parser
            && data_.pitch
            && data_.converter
            && data_.tee);  //check if all elements created
}

void Player::SetProperties() {
    GstBus *bus;

    abstract_src_->SetSrc(&data_);

    gst_bin_add_many(GST_BIN(data_.pipeline),
            data_.iddemux,
            data_.decoder,
            data_.parser,
            data_.pitch,
            data_.converter,
            data_.tee,
            NULL);  //add elements to bin

    bus = gst_element_get_bus(data_.pipeline);
    gst_bus_add_watch(bus, PlayerCallbacks::BusCall, &data_);

    SetTagsFilters();

    gst_object_unref(bus);
}

void Player::SetTagsFilters() {
    tags_map_->insert(std::make_pair(std::string(GST_TAG_TITLE), std::string("")));
    tags_map_->insert(std::make_pair(std::string(GST_TAG_ARTIST), std::string("")));
    tags_map_->insert(std::make_pair(std::string(GST_TAG_ALBUM), std::string("")));
    tags_map_->insert(std::make_pair(std::string(GST_TAG_GENRE), std::string("")));
    tags_map_->insert(std::make_pair(std::string(GST_TAG_DATE_TIME), std::string("")));

    tags_map_parsers_->insert(std::make_pair(std::string(GST_TAG_TITLE), PlayerCallbacks::ParseStringTag));
    tags_map_parsers_->insert(std::make_pair(std::string(GST_TAG_ARTIST), PlayerCallbacks::ParseStringTag));
    tags_map_parsers_->insert(std::make_pair(std::string(GST_TAG_ALBUM), PlayerCallbacks::ParseStringTag));
    tags_map_parsers_->insert(std::make_pair(std::string(GST_TAG_GENRE), PlayerCallbacks::ParseStringTag));
    tags_map_parsers_->insert(std::make_pair(std::string(GST_TAG_DATE_TIME), PlayerCallbacks::ParseDateTag));
}

AbstractSink *Player::AddSink(AbstractSink *sink) {
    std::list<AbstractSink *>::iterator it;
    it = abstract_sinks_->begin();
    while(it != abstract_sinks_->end()) {
        if(*it == sink)
            return NULL;
        it++;
    }

    sink->InitSink(&data_);
    abstract_sinks_->push_front(sink);

    return sink;
}

void Player::RemoveSink(AbstractSink *sink) {
    std::list<AbstractSink *>::iterator it;
    it = abstract_sinks_->begin();
    while(it != abstract_sinks_->end()) {
        if(*it == sink) {
            (*it)->Finish();
            abstract_sinks_->erase(it);

            if(!abstract_sinks_->size())
                gst_element_set_state(data_.pipeline, GST_STATE_READY);
            return;
        }
        it++;
    }
}

bool Player::ready() const {
    return data_.async_ready;
}

void Player::Init() {
    ConstructObjects();
    SetProperties();
    LinkElements();
}

void Player::LinkElements() {
    abstract_src_->LinkSrc();

    g_assert(gst_element_link_many(
                data_.iddemux,
                data_.parser,
                data_.decoder,
                data_.converter,
                data_.pitch,
                data_.tee,
                NULL)
            );  //link chain
}

const AbstractSrc *Player::abstract_src() const {
    return abstract_src_;
}

void Player::set_playback_speed(float ratio) {
    g_object_set(G_OBJECT(data_.pitch), "tempo", ratio, NULL);
}

void Player::RegisterTagsMapCallback(TagsMapCallback cb_func, void *cb_data) {
    tags_map_cb_ = cb_func;
    tags_map_cb_data_ = cb_data;
}

void PlayerCallbacks::ParseStringTag(const GstTagList *list, const gchar *tag, gint index, void *ptr_to_elem) {
    std::string *elem_data = reinterpret_cast<std::string *>(ptr_to_elem);
    gchar *string;

    gst_tag_list_get_string_index(list, tag, index, &string);

    std::string temp = std::string(string);
    *(elem_data) = temp;

    g_free(string);
}

void PlayerCallbacks::ParseDateTag(const GstTagList *list, const gchar *tag, gint index, void *ptr_to_elem) {
    std::string *elem_data = reinterpret_cast<std::string *>(ptr_to_elem);
    GstDateTime *time;

    gst_tag_list_get_date_time_index(list, tag, index, &time);
    gint year = gst_date_time_get_year(time);

    std::ostringstream t;
    t << year;
    std::string temp = t.str();
    *(elem_data) = temp;

    gst_date_time_unref(time);
}

int Player::type() const {
    return type_;
}
