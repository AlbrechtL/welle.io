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

#include <QByteArray>
#include <QHostAddress>
#include <QSettings>
#include <QTcpSocket>
#include <QTimer>
#include <QtNetwork>

#include "CVirtualInput.h"
#include "DabConstants.h"
#include "MathHelper.h"
#include "ringbuffer.h"
#include "CRadioController.h"

class CRTL_TCP_Client : public CVirtualInput {
    Q_OBJECT
public:
    CRTL_TCP_Client(CRadioController &RadioController);
    ~CRTL_TCP_Client(void);

    // Interface methods
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
    void setHwAgc(bool hwAGC);
    QString getName(void);
    CDeviceID getID(void);

    // Specific methods
    void setIP(QString IPAddress);
    void setPort(uint16_t Port);

    CRadioController *RadioController;

private slots:
    void readData(void);
    void TCPConnectionWatchDogTimeout(void);
    void AGCTimerTimeout(void);

private:
    QTcpSocket TCPSocket;
    QTimer TCPConnectionWatchDog;
    QTimer AGCTimer;

    int32_t LastFrequency;
    float CurrentGain;
    uint16_t CurrentGainCount;
    uint8_t MinValue;
    uint8_t MaxValue;
    bool isAGC;
    bool isHwAGC;
    int32_t Frequency;
    RingBuffer<uint8_t>* SampleBuffer;
    RingBuffer<uint8_t>* SpectrumSampleBuffer;
    bool connected;
    QHostAddress serverAddress;
    uint16_t serverPort;

    void sendVFO(int32_t frequency);
    void sendRate(int32_t theRate);
    void setGainMode(int32_t gainMode);
    void sendCommand(uint8_t cmd, int32_t param);
    float getGainValue(uint16_t GainCount);
};

#endif
