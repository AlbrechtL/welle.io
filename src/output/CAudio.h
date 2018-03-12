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

#ifndef __CAUDIO__
#define	__CAUDIO__
#include	<stdio.h>
#include    <QAudioOutput>
#include    <memory>
#include    <QTimer>

#include	"DabConstants.h"
#include	"ringbuffer.h"

class CAudioIODevice : public QIODevice
{
    Q_OBJECT

public:
    CAudioIODevice(std::shared_ptr<RingBuffer<int16_t>> Buffer, QObject *parent);
    ~CAudioIODevice();

    void start();
    void stop();
    void flush();

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qint64 bytesAvailable() const;

private:
    std::shared_ptr<RingBuffer<int16_t>> Buffer;
};


class	CAudio: public QObject{
Q_OBJECT
public:
    CAudio(std::shared_ptr<RingBuffer<int16_t>> Buffer);
    ~CAudio(void);
    void stop (void);
    void reset(void);
    void setRate (int sampleRate);
    void setVolume (qreal volume);

private:

    QAudioFormat AudioFormat;
    QAudioOutput* AudioOutput;
    CAudioIODevice *AudioIODevice;
    QTimer  CheckAudioBufferTimer;
    std::shared_ptr<RingBuffer<int16_t>> Buffer;

    QAudio::State CurrentState;
    int32_t		CardRate;

signals:
    void rateChanged(int newRate);

private slots:
    void init(int sampleRate);
    void handleStateChanged(QAudio::State newState);
    void checkAudioBufferTimeout();
};
#endif

