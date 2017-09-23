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

#ifndef SRC_RING_SRC_H_
#define SRC_RING_SRC_H_

#include "abstract_src.h"
#include "blocking_ring_buffer.h"
#include "player.h"
#include <gst/gst.h>
#include <stdint.h>

/**
 * @brief This callback is called when reading from internal buffer
 * @param[in] Current buffer fullness is provided
 * @param[in,out] Provided user data
 */
typedef void (*ReadCallback)(size_t, void *);

/**
 * @namespace RingSrcCallbacks
 * @brief This namespace provides necessary for RingSrc class callbacks (mostly GStreamer callbacks)
 */
namespace RingSrcCallbacks {

    extern "C" {
        /**
         * GStreamer callback called when feed started. Check GStreamer documentation for more
         */
        gboolean ReadData(gpointer);

        /**
         * GStreamer callback called when need more data. Check GStreamer documentation for more
         */
        void StartFeed(GstElement *, guint, gpointer);

        /**
         * GStreamer callback called when feeding should be stopped. Check GStreamer documentation for more
         */
        void StopFeed(GstElement *, gpointer);

    }

}

/**
 * @class RingSrc
 * @brief Class used to read from buffer. Introduces internal ring buffer, threshold for audio time-stretching
 *
 * @author Kacper Patro patro.kacper@gmail.com
 * @copyright Public domain
 * @pre
 */
class RingSrc: public AbstractSrc {
    public:
        /**
         * Constructor of RingSrc
         * @param[in] threshold Relative to 0.5, indicates values for which audio time-stretching will take place
         * @param[in] buffer_length Length of internal blocking ring buffer
         */
        RingSrc(float threshold, size_t buffer_length);
        virtual ~RingSrc();

        void SetSrc(void *player_data);
        void LinkSrc();
        const char *name() const;
        void ResetSrc();

        /**
         * Decrements playback speed
         * @return Current playback speed
         */
        float DecrementRatio();

        /**
         * Increments playback speed
         * @return Current playback speed
         */
        float IncrementRatio();

        /**
         * Process audio time-stretching depending upon current internal buffer fullness
         */
        void ProcessThreshold();

        /**
         * Writes into internal ring buffer, could override oldest data when full
         * @param[in] source_buffer Pointer to source buffer
         * @param[in] how_many Number of elements to read from source buffer
         * @return Number of elements written into internal buffer
         */
        size_t Write(uint8_t *source_buffer, size_t how_many);

        /**
         * Checks last frame property
         * @return True when set, false otherwise
         */
        bool last_frame() const;

        /**
         * Sets last frame property. Needed to properly invoke cleaning procedures
         * @param[in] to
         */
        void set_last_frame(bool to);

        /**
         * Checks flush property
         * @return True when set, false otherwise
         */
        bool flush() const;

        /**
         * Sets flush property. Needed when flushing internal state
         * @param[in] to
         */
        void set_flush(bool to);

        /**
         * Gets internal ring buffer
         * @return Pointer to ring buffer
         */
        const BlockingRingBuffer *ring_buffer() const;

        /**
         * Registers read callback. cb_func will be called with cb_data passed when reading from internal buffer
         * @param[in] cb_func Function to be called. Must match ReadCallback signature
         * @param[in,out] cb_data Data which will be passed to cb_func when called
         */
        void RegisterReadCallback(ReadCallback cb_func, void *cb_data);

        friend gboolean RingSrcCallbacks::ReadData(gpointer);
        friend void RingSrcCallbacks::StartFeed(GstElement *, guint, gpointer);
        friend void RingSrcCallbacks::StopFeed(GstElement *, gpointer);

    private:
        /**
         * @struct Data
         * @brief This struct contains specific for RingSrc class elements
         */
        struct Data {
            RingSrc *abstract_src;  /**< Pointer to "this" src element */
            GstElement *src;    /**< Src element for GStreamer */
            guint source_id;    /**< SID, needed by GStreamer callbacks*/
            Player::Data *player_data;  /**< Pointer to core Player data */
        };

        Data *data_;    /**< Pointer to internal Data element */

        float threshold_;   /**< Threshold relative to 0.5, determines upper and lower bound for audio time-stretching  */
        float current_ratio_;   /**< Current speed ratio, 1.0 means common speed */
        bool flush_;    /** Reset internal state at next read */

        ReadCallback read_cb_;  /**< User provided callback function, called when reading from internal buffer */
        void *read_cb_data_;    /**< Pointer to user data that will be passed to read_cb_ function */

        const size_t buff_size_;    /**< Size of internal buffer */
        const size_t buff_chunk_size_;  /**< Size of buffer chunks */
        size_t upper_bound_;    /**< Upper time-stretching bound in number of items */
        size_t lower_bound_;    /**< Lower time-stretching bound in number of items */

        BlockingRingBuffer *ring_buffer_;   /**< Pointer to BlockingRingBuffer object */

        /**
         * Provides conversion from fraction number to number of items
         * @param[in] fraction Fraction number
         * @return Number of items
         */
        size_t ParseThreshold(float fraction);

};

#endif /* SRC_RING_SRC_H_ */
