/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
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

#ifndef __RTL_TCP_CLIENT
#define __RTL_TCP_CLIENT
#include "CVirtualInput.h"
#include "dab-constants.h"
#include "ringbuffer.h"
#include <QByteArray>
#include <QHostAddress>
#include <QSettings>
#include <QTcpSocket>
#include <QTimer>
#include <QtNetwork>

class CRTL_TCP_Client : public virtualInput {
    Q_OBJECT
public:
    CRTL_TCP_Client(QSettings* settings, bool* success);
    ~CRTL_TCP_Client(void);
    void setFrequency(int32_t);
    bool restart(void);
    void stop(void);
    int32_t getSamples(DSPCOMPLEX* V, int32_t size);
    int32_t getSpectrumSamples(DSPCOMPLEX* V, int32_t size);
    int32_t getSamplesToRead(void);
    void reset(void);
    float setGain(int32_t gain);
    int32_t getGainCount(void);
    void setAgc(bool AGC);

private slots:
    void readData(void);
    void TCPConnectionWatchDogTimeout(void);

private:
    int32_t lastFrequency;
    int theGain;
    void sendVFO(int32_t);
    void sendRate(int32_t);
    void setGainMode(int32_t gainMode);
    void sendCommand(uint8_t, int32_t);
    bool isvalidRate(int32_t);
    QSettings* remoteSettings;
    int32_t Frequency;
    RingBuffer<uint8_t>* SampleBuffer;
    RingBuffer<uint8_t>* SpectrumSampleBuffer;
    bool connected;
    QHostAddress serverAddress;
    QTcpSocket TCPSocket;
    qint64 basePort;
    QTimer TCPConnectionWatchDog;
};

#endif
