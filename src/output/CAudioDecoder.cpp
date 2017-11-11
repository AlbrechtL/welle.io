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
#include <QDebug>

#include "CAudioDecoder.h"
#include "various/Tools.h"

CAudioDecoder::CAudioDecoder(float threshold, size_t length, int type)
    : QObject(nullptr)
{
    m_StopProcess = false;

    m_AudioThread = new QThread;
    m_WAVBuffer = new CRingBuffer<int16_t>(2 * 32768);

    m_CodedBufferSize = length;
    m_CodedBuffer  = new CRingBuffer<uint8_t>(m_CodedBufferSize);
    m_aacDecoder = new CFaadDecoder(m_WAVBuffer);
    m_AudioOutput = new CAudioOutput(m_WAVBuffer);

    // CAudioOutput uses QTimers and these requires an event loop in QThread
    m_AudioOutput->moveToThread(m_AudioThread);
    m_AudioThread->start();

    // Init audio output
    connect(this, &CAudioDecoder::operate, m_AudioOutput, &CAudioOutput::start);
    emit operate();
}

CAudioDecoder::~CAudioDecoder()
{
    delete m_WAVBuffer;
    delete m_CodedBuffer;
    delete m_aacDecoder;
    delete m_AudioOutput;

    // Stop thread
    m_AudioThread->quit();
    m_AudioThread->wait();
    delete m_AudioThread;
}

void CAudioDecoder::RemoveSink(AbstractSink *sink)
{
    // Not used
}

AbstractSink *CAudioDecoder::AddSink(AbstractSink *sink)
{
    // Not used
}

size_t CAudioDecoder::Write(uint8_t *buffer, size_t length)
{
    // Wait until buffer has enough space
    while((m_CodedBufferSize - m_CodedBuffer->GetRingBufferReadAvailable()) < length)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Copy data to buffer
    size_t size = m_CodedBuffer->putDataIntoBuffer(buffer, length);

    // Wake Process thread
    m_NewData.notify_one();

    if(length != size)
        qDebug() << "CAudioDecoder:" << "Only" << size << "bytes witten of"  << length << "bytes";

    return length;

    /*
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
*/
}

void CAudioDecoder::LastFrame()
{
    m_StopProcess = true;
    m_NewData.notify_one();
}

void CAudioDecoder::RegisterTagsMapCallback(TagsMapCallback cb_func, void *cb_data)
{
    // ???
}

void CAudioDecoder::RegisterReadCallback(ReadCallback cb_func, void *cb_data)
{
    // ???
}

void CAudioDecoder::Process()
{
    uint8_t adts[7] = {0};
    size_t max_au_size = 1024; // ToDo Figure out max AU size
    uint8_t au_data[max_au_size] = {0};
    bool haveHeader = false;
    size_t current_au_size = 0;

    // Wait until data is inside the buffer
    std::unique_lock<std::mutex> locker(m_NewDataLock);

    m_StopProcess = false;

    while(!m_StopProcess)
    {
        m_NewData.wait(locker);

        // Wait until buffer 50 % of buffer is filled
        /*while(m_CodedBuffer->GetRingBufferReadAvailable() < m_CodedBufferSize / 2)
        {
            CTimeDuration TimeDuration;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            TimeDuration.stop();

            //qDebug() << TimeDuration.getDuration() << "ms" << m_CodedBuffer->GetRingBufferReadAvailable();
        }*/

        while(m_CodedBuffer->GetRingBufferReadAvailable() > 7)
        {
            // Get ADTS header
            if(!haveHeader)
            {
                m_CodedBuffer->getDataFromBuffer((uint8_t*) adts, 7);

                // Get AU size from ADTS header
                size_t adts_size = ((adts[3]&0x3) << 14) | (adts[4] << 3) | ((adts[5] & 0xE0) >> 5);
                current_au_size = adts_size - 7;

                if(current_au_size > max_au_size)
                {
                    qDebug() << "CAudioDecoder:" << "AU to large:" << current_au_size;

                    current_au_size = max_au_size;
                }

                // Next process AU
                haveHeader = true;
            }

            // Get AU
            if(haveHeader && m_CodedBuffer->GetRingBufferReadAvailable() >= current_au_size)
            {
                m_CodedBuffer->getDataFromBuffer((uint8_t*) au_data, current_au_size);

                // Decode AAC
                m_aacDecoder->MP42PCM (1, //dacRate
                        1, // ToDo: sbrFlag
                        0, // ToDo: mpegSurround
                        1, // ToDo: aacChannelMode
                        au_data,
                        current_au_size);

                // Next process header
                haveHeader = false;
            }
        }
    }

    qDebug() << "CAudioDecoder:" << "Process stop.";
}

int CAudioDecoder::PlayerType() const
{
    // ???
}

void CAudioDecoder::Flush()
{
    m_StopProcess = true;
    m_NewData.notify_one();
}
