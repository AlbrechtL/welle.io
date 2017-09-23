/*
 * @class PulseSink
 *
 * @author: Kacper Patro patro.kacper@gmail.com
 * @date May 13, 2015
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

#ifndef SRC_PULSE_SINK_H_
#define SRC_PULSE_SINK_H_

#include "abstract_sink.h"
#include "player.h"
#include <gst/gst.h>

/**
 * @namespace PulseSinkCallbacks
 * @brief This namespace provides necessary for PulseSink class callbacks (mostly GStreamer callbacks)
 */
namespace PulseSinkCallbacks {

    extern "C" {
        /**
         * GStreamer callback called when unlinking sink from pipeline. Check GStreamer documentation for more
         */
        GstPadProbeReturn UnlinkCallPulseSink(GstPad *, GstPadProbeInfo *, gpointer);

        /**
         * GStreamer callback called when queue is empty. Check GStreamer documentation for more
         */
        void QueueUnderrun(GstElement *, gpointer);

        /**
         * GStreamer callback called when queue has enough data. Check GStreamer documentation for more
         */
        void QueuePushing(GstElement *, gpointer);

    }

}

class PulseSink: public AbstractSink {
    public:
        PulseSink();
        virtual ~PulseSink();

        void InitSink(void *player_data);
        const char *name() const;
        void Finish();
        bool linked() const;

        friend GstPadProbeReturn PulseSinkCallbacks::UnlinkCallPulseSink(GstPad *, GstPadProbeInfo *, gpointer);

    private:
        /**
         * @struct Data
         * @brief This struct contains specific for PulseSink class elements
         */
        struct Data {
            PulseSink *abstract_sink;   /**< Pointer to "this" sink element */

            GstElement *queue;  /**< Queue element for GStreamer */
            GstElement *sink;   /**< Sink element for GStreamer */
            GstPad *teepad; /**< TeePad element for GStreamer */

            gboolean removing;  /**< True, when sink is being removed from pipeline */
            bool linked;    /**< True, when sink is linked in pipeline */

            Player::Data *player_data;  /**< Pointer to core Player data */
        };

        Data *data_;    /**< Pointer to internal Data element */

};

#endif /* SRC_PULSE_SINK_H_ */
