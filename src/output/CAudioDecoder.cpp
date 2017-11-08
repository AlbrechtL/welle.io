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

#include <unistd.h>
#include "CAudioDecoder.h"

CAudioDecoder::CAudioDecoder(float threshold, size_t length, int type)
    : QObject(nullptr)
{
    m_AudioThread = new QThread;
    m_AudioBuffer = new RingBuffer<int16_t>(2 * 32768);
    m_aacDecoder = new faadDecoder(m_AudioBuffer);
    m_AudioOutput = new CAudio(m_AudioBuffer);

    m_AudioOutput->moveToThread(m_AudioThread);
    m_AudioThread->start();
    connect(this, &CAudioDecoder::operate, m_AudioOutput, &CAudio::start);

    emit operate();
}

CAudioDecoder::~CAudioDecoder()
{
    delete m_AudioBuffer;
    delete m_aacDecoder;
    delete m_AudioOutput;
    delete m_AudioThread;
}

void CAudioDecoder::RemoveSink(AbstractSink *sink)
{

}

AbstractSink *CAudioDecoder::AddSink(AbstractSink *sink)
{

}

size_t CAudioDecoder::Write(uint8_t *buffer, size_t length)
{
    uint8_t *bufferStart = buffer;
    size_t remainingData = length;

    while(remainingData != 0)
    {
        // Use the ADTS header to figure out the AU size
        size_t adts_size = ((bufferStart[3]&0x3) << 14) | (bufferStart[4] << 3) | ((bufferStart[5] & 0xE0) >> 5);

        // Skip the ADTS header
        bufferStart += 7;
        size_t au_size = adts_size - 7;

        m_aacDecoder->MP42PCM (1, //dacRate
                1, // ToDo: sbrFlag
                0, // ToDo: mpegSurround
                1, // ToDo: aacChannelMode
                bufferStart,
                au_size);

        remainingData -= adts_size;
        bufferStart += au_size;
    }

    return length;
}

void CAudioDecoder::LastFrame()
{
}

void CAudioDecoder::RegisterTagsMapCallback(TagsMapCallback cb_func, void *cb_data)
{

}

void CAudioDecoder::RegisterReadCallback(ReadCallback cb_func, void *cb_data)
{
}

void CAudioDecoder::Process()
{
    // ToDo What does this function??? Just sleep
    while(1)
    {
        sleep(1); // A dirty timer
    }
}

int CAudioDecoder::PlayerType() const
{

}

void CAudioDecoder::Flush()
{
}
