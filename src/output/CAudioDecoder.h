/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDRDAB project
 *    Jaroslaw Bulat kwant@agh.edu.pl (AudioDecoder)
 *    Kacper Patro patro.kacper@gmail.com (AudioDecoder)
 *    7 July 2015 - version 1.0 beta
 *    7 July 2016 - version 2.0 beta
 *    1 November 2016 - version 2.0
 *    2.0
 *    Copyright (c) 2015 Jaroslaw Bulat, Kacper Patro
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

#ifndef CAUDIODECODER_H
#define CAUDIODECODER_H

#include <QObject>
#include <QThread>

#include "output/faad-decoder.h"
#include "output/CAudio.h"

// SDRDAB
#include "libs/sdrdab/audio_decoder.h"
#include "libs/sdrdab/AudioDecoder/abstract_sink.h"
#include "libs/sdrdab/AudioDecoder/player.h"
#include "libs/sdrdab/AudioDecoder/null_sink.h"
#include "libs/sdrdab/AudioDecoder/ring_src.h"
#include "libs/sdrdab/RingBuffer/ring_buffer.h"

class CAudioDecoder : public QObject, public AudioDecoder
{
    Q_OBJECT
public:
    explicit CAudioDecoder(float threshold, size_t length, int type = PLAYER_AAC);

    /**
     * Constructor of AudioDecoder
     * @param[in] threshold Relative to 0.5, indicates values for which audio time-stretching will take place
     * @param[in] length Length of internal blocking ring buffer
     * @param[in] type Player type: mpeg or aac
     */

    virtual ~CAudioDecoder();

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
    QThread *m_AudioThread;
    faadDecoder *m_aacDecoder;
    RingBuffer<int16_t> *m_AudioBuffer;
    CAudio *m_AudioOutput;

signals:signals:
    void operate();

public slots:
};

#endif // CAUDIODECODER_H
