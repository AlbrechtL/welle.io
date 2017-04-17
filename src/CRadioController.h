/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012
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

#ifndef CRADIOCONTROLLER_H
#define CRADIOCONTROLLER_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QPixmap>
#include <QVariantMap>

#include "CAudio.h"
#include "DabConstants.h"
#include "ofdm-processor.h"
#include "ringbuffer.h"
#include "DabConstants.h"
#include "fic-handler.h"
#include "msc-handler.h"
#include "CChannels.h"

class CVirtualInput;

class CRadioController : public QObject
{
    Q_OBJECT
public:
    CRadioController(CVirtualInput* Device, CDABParams& DABParams, QObject* parent = NULL);
    void Play(QString Channel, QString Station);
    void StartScan(void);
    void StopScan(void);
    QVariantMap GetGUIData(void);
    QPixmap GetMOTImage(void);
    int32_t GetSpectrumSamples(DSPCOMPLEX* Buffer, int32_t Size);
    int GetCurrentFrequency(void);
    int GetGainCount(void);
    void SetAGC(bool isAGC);
    float SetGain(int Gain);

private:
    void DeviceRestart(void);
    void DecoderRestart(bool isScan);
    void SetChannel(QString Channel, bool isScan, bool Force = false);
    void SetStation(QString Station, bool Force = false);
    void NextChannel(bool isWait);

    // Back-end objects
    CVirtualInput* Device;
    CDABParams DABParams;
    CChannels Channels;

    ofdmProcessor* my_ofdmProcessor;
    ficHandler* my_ficHandler;
    mscHandler* my_mscHandler;
    CAudio* Audio;
    RingBuffer<int16_t>* AudioBuffer;

    // Objects set by the back-end
    QVariantMap GUIData;
    QDateTime CurrentDateTime;
    bool isFICCRC;
    bool isSync;
    bool isSignal;
    int SNR;
    int FrequencyCorrection;
    int BitRate;
    int AudioSampleRate;
    int FrameErrors;
    int RSErrors;
    int AACErrors;
    bool isStereo;
    bool isDAB;
    QString Label;
    QPixmap *MOTImage;

    // Controller objects
    QString CurrentChannel;
    int32_t CurrentFrequency;
    QString CurrentStation;
    QString CurrentDisplayStation;
    QString CurrentStationType;
    QString CurrentLanguageType;
    int32_t CurrentManualGain;
    float CurrentManualGainValue;

    QList<QString> StationList;
    QTimer StationTimer;
    QTimer ChannelTimer;
    QTimer SyncCheckTimer;

    // Handling variables
    bool isChannelScan;
    bool isGUIInit;

private slots:
    void StationTimerTimeout(void);
    void ChannelTimerTimeout(void);
    void SyncCheckTimerTimeout(void);

signals:
    void FoundStation(QString Station, QString CurrentChannel);
    void ScanStopped(void);
    void ScanProgress(int Progress);
    void MOTChanged(QPixmap MOTImage);
    void ShowErrorMessage(QString Text);

public slots:
    // This slots are called from the backend
    void addtoEnsemble(const QString &Station);
    void nameofEnsemble(int id, const QString&v);
    void changeinConfiguration(void);
    void displayDateTime(int* DateTime);
    void show_ficSuccess(bool isFICCRC);
    void show_snr(int SNR);
    void set_fineCorrectorDisplay(int FineFrequencyCorr);
    void set_coarseCorrectorDisplay(int CoarseFreuqencyCorr);
    void setSynced(char isSync);
    void setSignalPresent(bool isSignal);
    void setErrorMessage(QString ErrorMessage);
    void newAudio(int SampleRate);
    void setStereo(bool isStereo);
    void show_frameErrors(int FrameErrors);
    void show_rsErrors(int RSErrors);
    void show_aacErrors(int AACErrors);
    void showLabel(QString Label);
    void showMOT(QByteArray Data, int Subtype, QString s);
};

#endif // CRADIOCONTROLLER_H
