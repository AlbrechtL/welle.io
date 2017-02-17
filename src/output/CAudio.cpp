/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
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

#include	"CAudio.h"
#include	<stdio.h>


CAudio::CAudio(RingBuffer<int16_t> *Buffer, int16_t latency)
{
    this->latency = latency;
    this->CardRate = 48000;

    AudioIODevice = new CAudioIODevice(Buffer, this);

    AudioFormat.setSampleRate(CardRate);
    AudioFormat.setChannelCount(2);
    AudioFormat.setSampleSize(16);
    AudioFormat.setCodec("audio/pcm");
    AudioFormat.setByteOrder(QAudioFormat::LittleEndian);
    AudioFormat.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(AudioFormat))
    {
        fprintf(stderr,"Raw audio format not supported by backend, cannot play audio.\n");
        return;
    }

    AudioOutput = new QAudioOutput(AudioFormat, this);
    connect(AudioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    AudioIODevice->start();
    AudioOutput->start(AudioIODevice);
}

CAudio::~CAudio(void)
{
    delete AudioOutput;
    delete AudioIODevice;
}

void CAudio::restart(void)
{
}

void CAudio::audioOut(int SampleRate)
{
    if(CardRate != SampleRate)
    {
        CardRate = SampleRate;
        AudioFormat.setSampleRate(CardRate);
        fprintf(stderr, "Sample rate %i Hz\n", SampleRate);
    }
}

void CAudio::stop(void)
{
}

void CAudio::handleStateChanged(QAudio::State newState)
{
    switch (newState) {
        case QAudio::IdleState:
            // Finished playing (no more data)
            /*AudioOutput->stop();
            AudioIODevice->close();
            delete AudioOutput;*/
            break;

        case QAudio::StoppedState:
            // Stopped for other reasons
            if (AudioOutput->error() != QAudio::NoError) {
                // Error handling
            }
            break;

        default:
            // ... other cases as appropriate
            break;
    }
}


CAudioIODevice::CAudioIODevice(RingBuffer<int16_t> *Buffer, QObject *parent)
    :   QIODevice(parent)
{
    this->Buffer = Buffer;
}

CAudioIODevice::~CAudioIODevice()
{
}

void CAudioIODevice::start()
{
    open(QIODevice::ReadOnly);
}

void CAudioIODevice::stop()
{
    Buffer->FlushRingBuffer();
    close();
}

qint64 CAudioIODevice::readData(char *data, qint64 len)
{
    qint64 total = 0;

    total = Buffer->getDataFromBuffer(data, len / 2); // we have int16 samples

    return total * 2;
}

qint64 CAudioIODevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 CAudioIODevice::bytesAvailable() const
{
    return Buffer->GetRingBufferReadAvailable();
}

