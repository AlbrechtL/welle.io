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
    Q_UNUSED(threshold);

    m_codecType = type;
    m_StopProcess = false;
    m_sampleRate = 48000;

    m_AudioThread = new QThread;
    m_WAVBuffer = std::make_shared<CRingBuffer<int16_t>>(2 * 32768);

    m_CodedBufferSize = length;
    m_CodedBuffer  = std::make_unique<CRingBuffer<uint8_t>>(m_CodedBufferSize);
    m_aacDecoder = std::make_unique<CFaadDecoder>(m_WAVBuffer);
    m_AudioOutput = new CAudioOutput(m_WAVBuffer);

    // CAudioOutput uses QTimers and these requires an event loop in QThread
    m_AudioOutput->moveToThread(m_AudioThread);
    m_AudioThread->start();

    // Init audio output
    connect(this, &CAudioDecoder::operate, m_AudioOutput, &CAudioOutput::start);
    connect(this, &CAudioDecoder::sampleRateChanged, m_AudioOutput, &CAudioOutput::setRate);
    emit operate();
}

CAudioDecoder::~CAudioDecoder()
{
    delete m_AudioOutput;

    // Stop thread
    m_AudioThread->quit();
    m_AudioThread->wait();
    delete m_AudioThread;
}

void CAudioDecoder::RemoveSink(AbstractSink *sink)
{
    Q_UNUSED(sink);
}

AbstractSink *CAudioDecoder::AddSink(AbstractSink *sink)
{
    Q_UNUSED(sink);

    return nullptr;
}

size_t CAudioDecoder::Write(uint8_t *buffer, size_t length)
{
    bool isOverload=false;

    // Wait until buffer has enough free space
    while(m_CodedBuffer->FreeSpace() < length)
    {
        isOverload = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if(isOverload)
        qDebug() << "CAudioDecoder:" << "Compressed audio buffer overload!";

    // Copy data to buffer
    m_CodedBuffer->putDataIntoBuffer(buffer, length);

    // Wake Process thread
    m_NewData.notify_one();

    return length;
}

void CAudioDecoder::LastFrame()
{
    m_StopProcess = true;
    m_NewData.notify_one();
}

void CAudioDecoder::RegisterTagsMapCallback(TagsMapCallback cb_func, void *cb_data)
{
    // ???
    Q_UNUSED(cb_func);
    Q_UNUSED(cb_data);
}

void CAudioDecoder::RegisterReadCallback(ReadCallback cb_func, void *cb_data)
{
    // ???
    Q_UNUSED(cb_func);
    Q_UNUSED(cb_data);
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

        while(m_CodedBuffer->GetRingBufferReadAvailable() > 7)
        {
            uint8_t adts_dacsbr = 0;

            // Get ADTS header
            if(!haveHeader)
            {
                m_CodedBuffer->getDataFromBuffer((uint8_t*) adts, 7);

                // Process ADTS header
                adts_dacsbr = (adts[2]>>2) & 0xF;

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

                uint32_t sampleRate = 0;

                // Decode AAC
                m_aacDecoder->MP42PCM (adts_dacsbr,
                        0, // ToDo: mpegSurround
                        1, // ToDo: aacChannelMode
                        au_data,
                        current_au_size,
                        &sampleRate);

                if(sampleRate != m_sampleRate && sampleRate > 0)
                {
                    m_sampleRate = sampleRate;
                    emit sampleRateChanged(m_sampleRate);
                }

                // Next process header
                haveHeader = false;
            }
        }
    }

    qDebug() << "CAudioDecoder:" << "Process stop.";
}

int CAudioDecoder::PlayerType() const
{
    return m_codecType;
}

void CAudioDecoder::Flush()
{
    m_CodedBuffer->FlushRingBuffer();
    m_WAVBuffer->FlushRingBuffer();

    m_StopProcess = true;
    m_NewData.notify_one();
}
