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
#include <QTimer>

//#include "CAudio.h"
#include "CStationList.h"
#include "CChannels.h"
#include "scheduler.h"

class CVirtualInput;

#ifdef Q_OS_ANDROID
#include "rep_CRadioController_source.h"
class CRadioController : public CRadioControllerSource
{
    Q_OBJECT
#else
class CRadioController : public QObject, public Scheduler
{
    Q_OBJECT
    Q_PROPERTY(QString DateTime READ DateTime NOTIFY DateTimeChanged)
    Q_PROPERTY(bool isSync READ isSync NOTIFY isSyncChanged)
    Q_PROPERTY(bool isFICCRC READ isFICCRC NOTIFY isFICCRCChanged)
    Q_PROPERTY(bool isSignal READ isSignal NOTIFY isSignalChanged)
    Q_PROPERTY(bool isStereo READ isStereo NOTIFY isStereoChanged)
    Q_PROPERTY(bool isDAB READ isDAB NOTIFY isDABChanged)
    Q_PROPERTY(int SNR READ SNR NOTIFY SNRChanged)
    Q_PROPERTY(int FrequencyCorrection READ FrequencyCorrection NOTIFY FrequencyCorrectionChanged)
    Q_PROPERTY(int BitRate READ BitRate NOTIFY BitRateChanged)
    Q_PROPERTY(int AudioSampleRate READ AudioSampleRate NOTIFY AudioSampleRateChanged)
    Q_PROPERTY(int FrameErrors READ FrameErrors NOTIFY FrameErrorsChanged)
    Q_PROPERTY(int RSErrors READ RSErrors NOTIFY RSErrorsChanged)
    Q_PROPERTY(int AACErrors READ AACErrors NOTIFY AACErrorsChanged)
    Q_PROPERTY(bool isHwAGCSupported READ isHwAGCSupported NOTIFY isHwAGCSupportedChanged)
    Q_PROPERTY(bool HwAGC READ HwAGC WRITE setHwAGC NOTIFY HwAGCChanged)
    Q_PROPERTY(bool AGC READ AGC WRITE setAGC NOTIFY AGCChanged)
    Q_PROPERTY(float GainValue READ GainValue NOTIFY GainValueChanged)
    Q_PROPERTY(int GainCount READ GainCount NOTIFY GainCountChanged)
    Q_PROPERTY(int Gain READ Gain WRITE setGain NOTIFY GainChanged)
    Q_PROPERTY(qreal Volume READ Volume WRITE setVolume NOTIFY VolumeChanged)
    Q_PROPERTY(QList<StationElement*> Stations READ Stations NOTIFY StationsChanged)
    Q_PROPERTY(QVariantMap GUIData READ GUIData NOTIFY GUIDataChanged)
    Q_PROPERTY(QImage MOT READ MOT NOTIFY MOTChanged)
#endif

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
    Q_ENUMS(DabStatus)

    CRadioController(QVariantMap &commandLineOptions, QObject* parent = NULL);
    ~CRadioController(void);
    void closeDevice();
    void openDevice(CVirtualInput* Dev);
    void Play(QString Channel, QString Station);
    void Pause();
    void Stop();
    void ClearStations();
    void SetStation(QString Station, bool Force = false);
    void SetChannel(QString Channel, bool isScan, bool Force = false);
    void SetManualChannel(QString Channel);
    void StartScan(void);
    void StopScan(void);
    void UpdateSpectrum(void);
    QList<StationElement*> Stations() const;
    QVariantMap GUIData(void) const;
    QImage MOT() const;

    QString DateTime() const;
    bool isSync() const;
    bool isFICCRC() const;
    bool isSignal() const;
    bool isStereo() const;
    bool isDAB() const;
    int SNR() const;
    int FrequencyCorrection() const;
    int BitRate() const;
    int AudioSampleRate() const;
    int FrameErrors() const;
    int RSErrors() const;
    int AACErrors() const;

    qreal Volume() const;
    void setVolume(qreal Volume);

    bool isHwAGCSupported() const;
    bool HwAGC() const;
    void setHwAGC(bool isHwAGC);

    bool AGC() const;
    void setAGC(bool isAGC);

    int Gain() const;
    void setGain(int Gain);

    int GainCount() const;
    float GainValue() const;

private:
    void Initialise(void);
    void ResetTechnicalData(void);
    void DeviceRestart(void);
    void DecoderRestart(bool isScan);
    void NextChannel(bool isWait);
    void UpdateGUIData();
    void SetFrequencyCorrection(int FrequencyCorrection);

    // Back-end objects
    CVirtualInput* Device;
    QVariantMap commandLineOptions;
    CChannels Channels;

//    CAudio* Audio;
    RingBuffer<int16_t>* AudioBuffer;

    // Objects set by the back-end
    QVariantMap mGUIData;
    QDateTime mCurrentDateTime;
    bool mIsSync;
    bool mIsFICCRC;
    bool mIsSignal;
    bool mIsStereo;
    bool mIsDAB;
    int mSNR;
    int mFrequencyCorrection;
    int mBitRate;
    int mAudioSampleRate;
    int mFrameErrors;
    int mRSErrors;
    int mAACErrors;
    int mGainCount;
    int mStationCount;

    QImage *MOTImage;

    // Controller objects
    DabStatus Status;
    QString CurrentChannel;
    QString CurrentEnsemble;
    int32_t CurrentFrequency;
    QString CurrentStation;
    QString CurrentStationType;
    QString CurrentLanguageType;
    QString CurrentTitle;
    QString CurrentText;
    int32_t CurrentManualGain;
    float CurrentManualGainValue;
    qreal CurrentVolume;

    CStationList mStationList;
    QList<QString> StationList;
    QTimer StationTimer;
    QTimer ChannelTimer;
    QTimer SyncCheckTimer;

    // Handling variables
    bool startPlayback;
    bool isChannelScan;
    bool isAGC;
    bool isHwAGC;

    // Spectrum variables
//    common_fft* spectrum_fft_handler;
    QVector<QPointF> spectrum_data;

private slots:
    void StationTimerTimeout(void);
    void ChannelTimerTimeout(void);
    void SyncCheckTimerTimeout(void);

#ifndef Q_OS_ANDROID
signals:
    void DateTimeChanged(QString);
    void isSyncChanged(bool);
    void isFICCRCChanged(bool);
    void isSignalChanged(bool);
    void isStereoChanged(bool);
    void isDABChanged(bool);
    void SNRChanged(int);
    void FrequencyCorrectionChanged(int);
    void BitRateChanged(int);
    void AudioSampleRateChanged(int);
    void FrameErrorsChanged(int);
    void RSErrorsChanged(int);
    void AACErrorsChanged(int);
    void GainCountChanged(int);

    void isHwAGCSupportedChanged(bool);
    void HwAGCChanged(bool);
    void AGCChanged(bool);
    void GainValueChanged(float);
    void GainChanged(int);
    void VolumeChanged(qreal);
    void MOTChanged(QImage MOTImage);

    void StationsChanged(QList<StationElement*> Stations);
    void GUIDataChanged(QVariantMap GUIData);
    void DeviceReady();
    void DeviceClosed();
    void StationsCleared();
    void FoundStation(QString Station, QString CurrentChannel);
    void ScanStopped();
    void ScanProgress(int Progress);
    void SpectrumUpdated(qreal Ymax, qreal Xmin, qreal Xmax, QVector<QPointF> Data);
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
    void setErrorMessage(QString Text);
    void setInfoMessage(QString Text);
    void setAndroidInstallDialog(QString Title, QString Text);
};

#endif // CRADIOCONTROLLER_H
