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

#ifndef SRC_PLAYER_H_
#define SRC_PLAYER_H_

#include "abstract_sink.h"
#include "abstract_src.h"
#include <gst/gst.h>
#include <list>
#include <map>
#include <string>
#include <stdint.h>
#include <string.h>

/**
 * @brief This callback is called after tag has been parsed
 * @param[in] Pointer to tags map is provided
 * @param[in,out] Provided user data
 */
typedef void (*TagsMapCallback)(const std::map<const std::string, std::string> *, void *);

/**
 * @brief This callback is called when parsing tag
 * @param[in] Pointer to GstTagList element
 * @param[in] Name of tag, usually provided by GStreamer
 * @param[in] Index of tag in GstTagList, usually provided by GStreamer
 * @param[out] ptr_to_elem Pointer into where parsed tag should be saved
 */
typedef void (*TagsMapParser)(const GstTagList *, const gchar *, gint, void *);

/**
 * @brief Indicates Player type
 */
enum {
    PLAYER_MPEG,    /**< MPEG - DAB */
    PLAYER_AAC  /**< AAC - DAB+ */
};

/**
 * @namespace PlayerCallbacks
 * @brief This namespace provides necessary for PulseSink class callbacks (mostly GStreamer callbacks)
 */
namespace PlayerCallbacks {

    /**
     * Parses string tag
     * @param[in] list Pointer to GstTagList element
     * @param[in] tag Name of tag, usually provided by GStreamer
     * @param[in] index Index of tag in GstTagList, usually provided by GStreamer
     * @param[out] ptr_to_elem Pointer into where parsed tag should be saved
     */
    void ParseStringTag(const GstTagList *list, const gchar *tag, gint index, void *ptr_to_elem);

    /**
     * Parses date tag
     * @param[in] list Pointer to GstTagList element
     * @param[in] tag Name of tag, usually provided by GStreamer
     * @param[in] index Index of tag in GstTagList, usually provided by GStreamer
     * @param[out] ptr_to_elem Pointer into where parsed tag should be saved
     */
    void ParseDateTag(const GstTagList *list, const gchar *tag, gint index, void *ptr_to_elem);

    extern "C" {

        /**
         * GStreamer callback called when got tags in pipeline. Check GStreamer documentation for more
         */
        void SaveTags(const GstTagList *, const gchar *, gpointer);

        /**
         * GStreamer callback called when bus event caught. Check GStreamer documentation for more
         */
        gboolean BusCall(GstBus *, GstMessage *, gpointer);

    }

}

/**
 * @class Player
 * @brief Class used to manage GStreaner pipeline
 *
 * @author Kacper Patro patro.kacper@gmail.com
 * @copyright Public domain
 * @pre
 */
class Player {
    public:
        /**
         * Constructor of Player
         * @param[in] src Pointer to AbstractSrc object
         * @param[in] type Player type: mpeg or aac
         */
        Player(AbstractSrc *src, int type);
        virtual ~Player();

        /**
         * Starts processing
         */
        void Process();

        /**
         * Gets source
         * @return Current AbstractSrc object
         */
        const AbstractSrc *abstract_src() const;

        /**
         * Removes sink
         * @param[in] sink Pointer to sink object
         */
        void RemoveSink(AbstractSink *sink);

        /**
         * Sets playback speed
         * @param[in] ratio Speed ratio
         */
        void set_playback_speed(float ratio);

        /**
         * Adds new sink
         * @param[in,out] sink Pointer to sink object
         * @return Added sink, NULL if already present
         */
        AbstractSink *AddSink(AbstractSink *sink);

        /**
         * Internal ready state
         * @return True if Player ready to change playback speed
         */
        bool ready() const;

        /**
         * Registers tags map callback. cb_func will be called with cb_data passed when tags received
         * @param[in] cb_func Function to be called. Must match TagsMapCallback signature
         * @param[in,out] cb_data Data which will be passed to cb_func when called
         */
        void RegisterTagsMapCallback(TagsMapCallback cb_func, void *cb_data);

        /**
         * Gets Player current type
         * @return Player type
         */
        int type() const;

        friend void PlayerCallbacks::SaveTags(const GstTagList *, const gchar *, gpointer);
        friend gboolean PlayerCallbacks::BusCall(GstBus *, GstMessage *, gpointer);

        /**
         * @struct Data
         * @brief This struct contains specific for Player class elements
         */
        struct Data {
            Player *player; /**< Pointer to "this" Player element */

            GstElement *pipeline;   /**< Pipeline element for GStreamer */

            GstElement *iddemux;    /**< ID3 demuxer element for GStreamer */
            GstElement *parser; /**< MPEG/AAC parser element for GStreamer */
            GstElement *decoder;    /**< MAD/FAAD decoder element for GStreamer */
            GstElement *pitch;  /**< SoundTouch pitch element for GStreamer */
            GstElement *converter;  /**< Converter element for GStreamer */
            GstElement *tee;    /**< Tee element for GStreamer */

            GMainLoop *loop;    /**< Loop element for GStreamer */

            bool async_ready;   /**< Ready flag */
        };

    private:
        int type_;  /**< Player type, PLAYER_MPEG or PLAYER_AAC */
        Data data_; /**< Internal Data element */

        std::map<const std::string, std::string> *tags_map_;    /**< Tags map: tag name - tag value */
        std::map<const std::string, TagsMapParser> *tags_map_parsers_;  /**< Tags parsers map: tag name - TagsMapParser function */

        AbstractSrc *abstract_src_; /**< Pointer to source of this Player */
        std::list<AbstractSink *> *abstract_sinks_; /**< Pointer to list of pointers to currently present sinks */

        TagsMapCallback tags_map_cb_;   /**< User provided callback function, called after tag has been parsed */
        void *tags_map_cb_data_;    /**< Pointer to user data that will be passed to tags_map_cb_ function */

        /**
         * Sets tags filters: which tag should be saved and which parser function should be called
         */
        void SetTagsFilters();

        /**
         * Construct necessary objects (GStreamer mainly)
         */
        void ConstructObjects();

        /**
         * Sets properties of constructed objects (GStreamer mainly)
         */
        void SetProperties();

        /**
         * Link all elements (GStreamer mainly)
         */
        void LinkElements();

        /**
         * Initializes internal state
         */
        void Init();

};

#endif /* SRC_PLAYER_H_ */
