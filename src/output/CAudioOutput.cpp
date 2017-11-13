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

#include <QDebug>
#include <stdio.h>

#include "CAudioOutput.h"
#include "various/Tools.h"

CAudioOutput::CAudioOutput(CRingBuffer<int16_t> *Buffer)
{
    AudioOutput = NULL;
    CardRate = 0;
    this->Buffer = Buffer;
}

CAudioOutput::~CAudioOutput(void)
{
    delete AudioOutput;
    delete AudioIODevice;
    delete CheckAudioBufferTimer;
}

void CAudioOutput::start(void)
{
    AudioIODevice = new CAudioIODevice(Buffer, this);

    init(48000);

    CheckAudioBufferTimer = new QTimer;
    connect(CheckAudioBufferTimer, &QTimer::timeout, this, &CAudioOutput::checkAudioBufferTimeout);
    // Check audio state every 1 s, start audio if bytes are available
    CheckAudioBufferTimer->start(1000);
}

void CAudioOutput::setRate(int sampleRate)
{
    if (CardRate != sampleRate) {
        qDebug() << "Audio:"
                 << "Sample rate" << sampleRate << "Hz";
        CardRate = sampleRate;
        init(sampleRate);
    }
}

void CAudioOutput::setVolume(qreal volume)
{
    if (AudioOutput != NULL) {
        qDebug() << "Audio:"
                 << "Volume" << volume;
        AudioOutput->setVolume(volume);
    }
}

void CAudioOutput::init(int sampleRate)
{
    if (AudioOutput != NULL) {
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
    if (!info.isFormatSupported(AudioFormat)) {
        qDebug() << "Audio:"
                 << "Audio format \"audio/pcm\" 16-bit stereo not supported. Your audio may not work!";
    }

    AudioOutput = new QAudioOutput(AudioFormat, this);
    connect(AudioOutput, &QAudioOutput::stateChanged, this, &CAudioOutput::handleStateChanged);

    AudioIODevice->start();
    AudioOutput->start(AudioIODevice);
}

void CAudioOutput::stop(void)
{
    AudioIODevice->stop();
    AudioOutput->stop();
}

void CAudioOutput::reset(void)
{
    AudioIODevice->flush();
    AudioOutput->reset();
}

void CAudioOutput::handleStateChanged(QAudio::State newState)
{
    CurrentState = newState;

    switch (newState) {
    case QAudio::ActiveState:
        qDebug() << "Audio:"
                 << "ActiveState";
        break;
    case QAudio::SuspendedState:
        qDebug() << "Audio:"
                 << "SuspendedState";
        break;
    case QAudio::StoppedState:
        qDebug() << "Audio:"
                 << "StoppedState";
        break;
    case QAudio::IdleState:
        qDebug() << "Audio:"
                 << "IdleState";
        // Necessary to avoid a IdleState, ActiveState, IdleState, ActiveState ... loop under Ubuntu. I don't know why.
        AudioOutput->stop();
        break;
    default:
        qDebug() << "Audio:"
                 << "Unknown state:" << newState;
        break;
    }
}

void CAudioOutput::checkAudioBufferTimeout()
{
    int32_t Bytes = Buffer->GetRingBufferReadAvailable();

    // Start audio if bytes are available and audio is not active
    if (AudioOutput && Bytes && CurrentState != QAudio::ActiveState) {
        AudioIODevice->start();
        AudioOutput->start(AudioIODevice);
    }
}

CAudioIODevice::CAudioIODevice(CRingBuffer<int16_t>* Buffer, QObject* parent)
    : QIODevice(parent)
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

void CAudioIODevice::flush()
{
    Buffer->FlushRingBuffer();
}

qint64 CAudioIODevice::readData(char* data, qint64 len)
{
    qint64 total = 0;
    static CTimeDuration TimeDuration;
    static bool isEmpty = false;

    // Sometimes it can be 0 samples
    if(len == 0)
        return 0;

    total = Buffer->getDataFromBuffer((int16_t*) data, len / 2); // we have int16 samples

    // If the buffer is empty return zeros.
    if(total == 0)
    {
        isEmpty = true;
        memset(data, 0, len);
        total = len / 2;
    }
    else
    {
        if(isEmpty)
        {
            TimeDuration.stop();
            qDebug() << "CAudioIODevice: Silence for" << TimeDuration.getDuration() << "ms";
        }
        else
        {
            TimeDuration.start();
        }
        isEmpty = false;
    }

    return total * 2;
}

qint64 CAudioIODevice::writeData(const char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 CAudioIODevice::bytesAvailable() const
{
    return Buffer->GetRingBufferReadAvailable();
}
