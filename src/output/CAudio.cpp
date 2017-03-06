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

CAudio::CAudio(RingBuffer<int16_t> *Buffer)
{
    AudioOutput = NULL;
    CardRate = 0;
    this->Buffer = Buffer;

    AudioIODevice = new CAudioIODevice(Buffer, this);

    init(48000);

    connect(&CheckAudioBufferTimer, SIGNAL(timeout()), this, SLOT(checkAudioBufferTimeout()));
    // Check audio state every 1 s, start audio if bytes are available
    CheckAudioBufferTimer.start(1000);
}

CAudio::~CAudio(void)
{
    delete AudioOutput;
    delete AudioIODevice;
}

void CAudio::setRate(int sampleRate)
{
    if(CardRate != sampleRate)
    {
        fprintf(stderr, "Sample rate %i Hz\n", sampleRate);
        CardRate = sampleRate;
        init(sampleRate);
    }
}

void CAudio::init(int sampleRate)
{
    if(AudioOutput != NULL)
    {
        delete AudioOutput;
        AudioOutput = NULL;
    }

    AudioFormat.setSampleRate(sampleRate);
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

void CAudio::stop(void)
{
    AudioIODevice->stop();
    AudioOutput->stop();
}

void CAudio::handleStateChanged(QAudio::State newState)
{
    CurrentState = newState;

    switch (newState)
    {
    case QAudio::ActiveState: fprintf(stderr, "Audio State: ActiveState\n"); break;
    case QAudio::SuspendedState: fprintf(stderr, "Audio State: SuspendedState\n"); break;
    case QAudio::StoppedState: fprintf(stderr, "Audio State: StoppedState\n"); break;
    case QAudio::IdleState: fprintf(stderr, "Audio State: IdleState\n"); break;
    default: fprintf(stderr, "Audio State: Unknown state e: %i\n", newState); break;
    }
}

void CAudio::checkAudioBufferTimeout()
{
    int32_t Bytes = Buffer->GetRingBufferReadAvailable();

    // Start audio if bytes are available and audio is not active
    if(AudioOutput && Bytes && CurrentState != QAudio::ActiveState)
    {
        AudioIODevice->start();
        AudioOutput->start(AudioIODevice);
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

