/*
 * @class FileSink
 *
 * @author: Kacper Patro patro.kacper@gmail.com
 * @date May 21, 2015
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

#ifndef SRC_FILE_SINK_H_
#define SRC_FILE_SINK_H_

#include "abstract_sink.h"
#include "player.h"
#include <gst/gst.h>

/**
 * @namespace FileSinkCallbacks
 * @brief This namespace provides necessary for FileSink class callbacks (mostly GStreamer callbacks)
 */
namespace FileSinkCallbacks {

    /**
     * GStreamer callback called when unlinking sink from pipeline. Check GStreamer documentation for more
     */
    extern "C" GstPadProbeReturn UnlinkCallFileSink(GstPad *, GstPadProbeInfo *, gpointer);

}

/**
 * @class FileSink
 * @brief Class used to save raw audio files
 *
 * @author Kacper Patro patro.kacper@gmail.com
 * @copyright Public domain
 * @pre
 */
class FileSink: public AbstractSink {
    public:
        /**
         * Constructor of FileSink
         * @param[in] path Path to output file
         */
        FileSink(const char *path);
        virtual ~FileSink();

        void InitSink(void *player_data);
        const char *name() const;
        void Finish();
        bool linked() const;

        friend GstPadProbeReturn FileSinkCallbacks::UnlinkCallFileSink(GstPad *, GstPadProbeInfo *, gpointer);

    private:
        /**
         * @struct Data
         * @brief This struct contains specific for FileSink class elements
         */
        struct Data {
            FileSink *abstract_sink;    /**< Pointer to "this" sink element */

            GstElement *queue;  /**< Queue element for GStreamer */
            GstElement *sink;   /**< Sink element for GStreamer */
            GstPad *teepad; /**< TeePad element for GStreamer */

            gboolean removing;  /**< True, when sink is being removed from pipeline */
            bool linked;    /**< True, when sink is linked in pipeline */

            Player::Data *player_data;  /**< Pointer to core Player data */
        };

        Data *data_;    /**< Pointer to internal Data element */

        const char *path_;  /**< Path to output file */

};

#endif /* SRC_FILE_SINK_H_ */
