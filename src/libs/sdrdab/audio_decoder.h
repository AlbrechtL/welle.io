/**
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

#ifndef AUDIO_DECODER_H_
#define AUDIO_DECODER_H_

#include "AudioDecoder/abstract_sink.h"
#include "AudioDecoder/player.h"
#include "AudioDecoder/null_sink.h"
#include "AudioDecoder/ring_src.h"


class AudioDecoder {
    public:
        /**
         * Constructor of AudioDecoder
         * @param[in] threshold Relative to 0.5, indicates values for which audio time-stretching will take place
         * @param[in] length Length of internal blocking ring buffer
         * @param[in] type Player type: mpeg or aac
         */
        AudioDecoder(float threshold, size_t length, int type = PLAYER_AAC);
        virtual ~AudioDecoder();

        /**
         * Removes sink
         * @param[in] sink Pointer to sink object
         */
        void RemoveSink(AbstractSink *sink);

        /**
         * Adds new sink
         * @param[in,out] sink Pointer to sink object
         * @return Added sink, NULL if already present
         */
        AbstractSink *AddSink(AbstractSink *sink);

        /**
         * Writes into internal ring buffer, could override oldest data when full
         * @param[in] buffer Pointer to source buffer
         * @param[in] length Length of source buffer to read from
         * @return Number of elements written into internal buffer
         */
        size_t Write(uint8_t *buffer, size_t length);

        /**
         * Should be called when current portion of data is last one
         */
        void LastFrame();

        /**
         * Registers tags map callback. cb_func will be called with cb_data passed when tags received
         * @param[in] cb_func Function to be called. Must match TagsMapCallback signature
         * @param[in,out] cb_data Data which will be passed to cb_func when called
         */
        void RegisterTagsMapCallback(TagsMapCallback cb_func, void *cb_data);

        /**
         * Registers read callback. cb_func will be called with cb_data passed when reading from internal buffer
         * @param[in] cb_func Function to be called. Must match ReadCallback signature
         * @param[in,out] cb_data Data which will be passed to cb_func when called
         */
        void RegisterReadCallback(ReadCallback cb_func, void *cb_data);

        /**
         * Starts audio processing
         */
        void Process();

        /**
         * Gets Player current type
         * @return Player type
         */
        int PlayerType() const;

        /**
         * Flushes Player state
         */
        void Flush();

    private:
        RingSrc *src_;  /**< Pointer to RingSrc feeder object */
        NullSink *sink_;    /**< Pointer to NullSink consumer object */
        Player *player_;    /**< Pointer to Player object */

};

#endif /* AUDIO_DECODER_H_ */
