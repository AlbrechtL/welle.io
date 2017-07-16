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
#include <QImage>
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

#ifdef Q_OS_ANDROID
#include "rep_CRadioController_source.h"
class CRadioController : public CRadioControllerSource
#else
class CRadioController : public QObject
#endif
{
    Q_OBJECT
public:
    enum DabStatus {
        Error       = -1,
        Unknown     = 0,
        Initialised = 1,
        Playing     = 2,
        Paused      = 3,
        Stopped     = 4,
        Scanning    = 5,
    };

    CRadioController(QVariantMap &commandLineOptions, CDABParams& DABParams, QObject* parent = NULL);
    ~CRadioController(void);
    void setDevice(CVirtualInput* Device);
    void Play(QString Channel, QString Station);
    void Pause();
    void Stop();
    void SetStation(QString Station, bool Force = false);
    void SetChannel(QString Channel, bool isScan, bool Force = false);
    void StartScan(void);
    void StopScan(void);
    QVariantMap GUIData(void) const;
    QImage MOT() const;
    int32_t GetSpectrumSamples(DSPCOMPLEX* Buffer, int32_t Size);
    int GetCurrentFrequency(void);

    qreal Volume() const;
    void setVolume(qreal Volume);

    bool HwAGC() const;
    void setHwAGC(bool isHwAGC);

    bool AGC() const;
    void setAGC(bool isAGC);

    int Gain() const;
    void setGain(int Gain);

    float GainValue() const;

    void setErrorMessage(QString Text);
    void setInfoMessage(QString Text);
    void setAndroidInstallDialog(QString Title, QString Text);

private:
    void DeviceRestart(void);
    void DecoderRestart(bool isScan);
    void NextChannel(bool isWait);
    void UpdateGUIData();
    void SetFrequencyCorrection(int FrequencyCorrection);

    // Back-end objects
    CVirtualInput* Device;
    QVariantMap commandLineOptions;
    CDABParams DABParams;
    CChannels Channels;

    ofdmProcessor* my_ofdmProcessor;
    ficHandler* my_ficHandler;
    mscHandler* my_mscHandler;
    CAudio* Audio;
    RingBuffer<int16_t>* AudioBuffer;

    // Objects set by the back-end
    QVariantMap _GUIData;
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
    QImage *MOTImage;

    // Controller objects
    DabStatus Status;
    QString CurrentChannel;
    int32_t CurrentFrequency;
    QString CurrentStation;
    QString CurrentDisplayStation;
    QString CurrentStationType;
    QString CurrentLanguageType;
    int32_t CurrentManualGain;
    float CurrentManualGainValue;
    qreal CurrentVolume;

    QList<QString> StationList;
    QTimer StationTimer;
    QTimer ChannelTimer;
    QTimer SyncCheckTimer;

    // Handling variables
    bool isChannelScan;
    bool isGUIInit;
    bool isAGC;
    bool isHwAGC;
    int GainCount;

private slots:
    void StationTimerTimeout(void);
    void ChannelTimerTimeout(void);
    void SyncCheckTimerTimeout(void);

#ifndef Q_OS_ANDROID
signals:
    void HwAGCChanged(bool);
    void AGCChanged(bool);
    void GainValueChanged(float);
    void GainChanged(int);
    void VolumeChanged(qreal);
    void MOTChanged(QImage MOTImage);

    void GUIDataChanged(QVariantMap GUIData);
    void DeviceReady();
    void FoundStation(QString SId, QString Station, QString CurrentChannel);
    void ScanStopped();
    void ScanProgress(int Progress);
    void showErrorMessage(QString Text);
    void showInfoMessage(QString Text);
    void showAndroidInstallDialog(QString Title, QString Text);
#endif

public slots:
    // This slots are called from the backend
    void addtoEnsemble(quint32 SId, const QString &Station);
    void nameofEnsemble(int id, const QString&v);
    void changeinConfiguration(void);
    void displayDateTime(int* DateTime);
    void show_ficSuccess(bool isFICCRC);
    void show_snr(int SNR);
    void set_fineCorrectorDisplay(int FineFrequencyCorr);
    void set_coarseCorrectorDisplay(int CoarseFreuqencyCorr);
    void setSynced(char isSync);
    void setSignalPresent(bool isSignal);
    void newAudio(int SampleRate);
    void setStereo(bool isStereo);
    void show_frameErrors(int FrameErrors);
    void show_rsErrors(int RSErrors);
    void show_aacErrors(int AACErrors);
    void showLabel(QString Label);
    void showMOT(QByteArray Data, int Subtype, QString s);
    void onEventLoopStarted(void);
};

#endif // CRADIOCONTROLLER_H
