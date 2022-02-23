/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
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

#include "audio_output.h"

CAudioThread::CAudioThread(RingBuffer<int16_t>& buffer, QObject *parent) :
    QThread(parent),
    buffer(buffer),
    audioIODevice(buffer, this),
    cardRate(48000)
{
    connect(&checkAudioBufferTimer, &QTimer::timeout,
            this, &CAudioThread::checkAudioBufferTimeout);

    // Check audio state every 1 s, start audio if bytes are available
    checkAudioBufferTimer.start(1000);

    // Move event processing of CAudioThread to this thread
    QObject::moveToThread(this);
}

CAudioThread::~CAudioThread(void)
{
    if (audioOutput != nullptr) {
        delete audioOutput;
        audioOutput = nullptr;
    }

    if (info != nullptr) {
        delete info;
        info = nullptr;
    }
}

void CAudioThread::setRate(int sampleRate)
{
    if (cardRate != sampleRate) {
        qDebug() << "Audio:"
                 << "Sample rate" << sampleRate << "Hz";
        cardRate = sampleRate;
        // restart audio within thread with new sample rate
        init(cardRate);
    }
}

void CAudioThread::setVolume(qreal volume)
{
    if (audioOutput != nullptr) {
        qDebug() << "Audio:"
                 << "Volume" << volume;
        audioOutput->setVolume(volume);
    }
}

void CAudioThread::init(int sampleRate)
{
    if (audioOutput != nullptr) {
        delete audioOutput;
        audioOutput = nullptr;
        currentState = QAudio::StoppedState;
    }

    audioFormat.setSampleRate(sampleRate);
    audioFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    audioFormat.setSampleFormat(QAudioFormat::Int16);

    info = new QAudioDevice(QMediaDevices::defaultAudioOutput());
    if (!info->isFormatSupported(audioFormat)) {
        qDebug() << "Audio:"
                 << "Audio format \"audio/pcm\" 16-bit stereo not supported. Your audio may not work!";
    }

    qDebug() << "Audio: Current sound output" << info->description();
    //qDebug() << "Audio: bytesPerSample: " << audioFormat.bytesPerSample();
    //qDebug() << "Audio: bytesPerFrame: " << audioFormat.bytesPerFrame();

//    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
//        qDebug() << "Audio:" << "Available sound output device: " << deviceInfo.deviceName();

    audioOutput = new QAudioSink(*info, audioFormat, this);
    connect(audioOutput, &QAudioSink::stateChanged, this, &CAudioThread::handleStateChanged);
}

void CAudioThread::run()
{
    // QAudioOutput needs to create within run()
    init(cardRate);
    // start event loop of QThread
    exec();
}

void CAudioThread::stop(void)
{
    audioIODevice.stop();
    audioOutput->stop();
}

void CAudioThread::reset(void)
{
    audioIODevice.flush();

    // For some reason audio under Windows stops working if reset() is used
    //audioOutput->reset();
}

void CAudioThread::handleStateChanged(QAudio::State newState)
{
    currentState = newState;

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
        break;
    default:
        qDebug() << "Audio:"
                 << "Unknown state:" << newState;
        break;
    }
}

void CAudioThread::checkAudioBufferTimeout()
{
    int32_t Bytes = buffer.GetRingBufferReadAvailable() * 2;

    // Start audio if bytes are available and audio is not active
    if (audioOutput && Bytes && currentState != QAudio::ActiveState) {
        audioIODevice.start();
        audioOutput->start(&audioIODevice);
    }
}

CAudioIODevice::CAudioIODevice(RingBuffer<int16_t>& buffer, QObject* parent) :
    QIODevice(parent),
    buffer(buffer)
{
}

void CAudioIODevice::start()
{
    open(QIODevice::ReadOnly);
}

void CAudioIODevice::stop()
{
    buffer.FlushRingBuffer();
    close();
}

void CAudioIODevice::flush()
{
    buffer.FlushRingBuffer();
}

qint64 CAudioIODevice::readData(char* data, qint64 len)
{
    if(len == 0)
        return 0;

    qint64 total = 0;

    total = buffer.getDataFromBuffer(data, len / 2); // we have int16 samples

    // If the buffer is empty return zeros.
    if (total == 0) {
        //qDebug() << "Audio:" << "Buffer is empty, fill with zeros.";
        memset(data, 0, len);
        total = len / 2;
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
    // Return always that data is available if not it will be handelt inside readData()
    return 16384;
}

CAudio::CAudio(RingBuffer<int16_t>& buffer, QObject *parent) :
    QObject(parent),
    audioThread(nullptr),
    buffer(buffer),
    audioIODevice(buffer, this)
{
    audioThread = std::make_unique<CAudioThread>(buffer);
    audioThread->start();
}

CAudio::~CAudio(void)
{
    if (audioThread != nullptr) {
        audioThread->quit();
        audioThread->wait();
    }
}

void CAudio::stop(void)
{
    // Call stopInternal of CAudioThread (and invoke it in the other thread)
    QMetaObject::invokeMethod(audioThread.get(), "stop", Qt::QueuedConnection);
}

void CAudio::reset(void)
{
    // Call resetInternal of CAudioThread (and invoke it in the other thread)
    QMetaObject::invokeMethod(audioThread.get(), "reset", Qt::QueuedConnection);
}

void CAudio::setRate(int sampleRate)
{
    // Call setRateInternal of CAudioThread (and invoke it in the other thread)
    QMetaObject::invokeMethod(audioThread.get(), "setRate", Qt::QueuedConnection, Q_ARG(int, sampleRate));
}

void CAudio::setVolume(qreal volume)
{
    // Call setVolumeInternal of CAudioThread (and invoke it in the other thread)
    QMetaObject::invokeMethod(audioThread.get(), "setVolume", Qt::QueuedConnection, Q_ARG(qreal, volume));
}


