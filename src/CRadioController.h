/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
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
#include <mutex>

#include "CAudio.h"
#include "CStationList.h"
#include "dab-constants.h"
#include "radio-receiver.h"
#include "ringbuffer.h"
#include "channels.h"

class CVirtualInput;

enum class PlotTypeEn { Spectrum, ImpulseResponse, QPSK, Null, Unknown };

#ifdef Q_OS_ANDROID
#include "rep_CRadioController_source.h"
class CRadioController : public CRadioControllerSource,
                         public RadioControllerInterface
{
    Q_OBJECT
#else
class CRadioController :
    public QObject,
    public RadioControllerInterface,
    public ProgrammeHandlerInterface
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
    Q_PROPERTY(float FrequencyCorrectionPpm READ FrequencyCorrectionPpm NOTIFY FrequencyCorrectionPpmChanged)
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
    Q_PROPERTY(QString ErrorMsg READ ErrorMsg NOTIFY showErrorMessage)
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

    CRadioController(QVariantMap &commandLineOptions, DABParams& params, QObject* parent = NULL);
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
    void setAutoPlay(QString Channel, QString Station);
    QList<StationElement*> Stations() const;
    QVariantMap GUIData(void) const;
    QString ErrorMsg() const;
    QImage MOT() const;

    QString DateTime() const;
    bool isSync() const;
    bool isFICCRC() const;
    bool isSignal() const;
    bool isStereo() const;
    bool isDAB() const;
    int SNR() const;
    int FrequencyCorrection() const;
    float FrequencyCorrectionPpm() const;
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

    //called from the backend
    virtual void onFrameErrors(int frameErrors) override;
    virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, bool isStereo, const std::string& mode) override;
    virtual void onRsErrors(int rsErrors) override;
    virtual void onAacErrors(int aacErrors) override;
    virtual void onNewDynamicLabel(const std::string& label) override;
    virtual void onMOT(const std::vector<uint8_t>& data, int subtype) override;
    virtual void onSNR(int snr) override;
    virtual void onFrequencyCorrectorChange(int fine, int coarse) override;
    virtual void onSyncChange(char isSync) override;
    virtual void onSignalPresence(bool isSignal) override;
    virtual void onServiceDetected(uint32_t sId, const std::string& label) override;
    virtual void onNewEnsembleName(const std::string& name) override;
    virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override;
    virtual void onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib) override;
    virtual void onNewImpulseResponse(std::vector<float>&& data) override;
    virtual void onConstellationPoints(std::vector<DSPCOMPLEX>&& data) override;
    virtual void onNewNullSymbol(std::vector<DSPCOMPLEX>&& data) override;
    virtual void onTIIMeasurement(tii_measurement_t&& m) override;
    virtual void onMessage(message_level_t level, const std::string& text) override;

    // Buffer getter
    std::vector<float> &&getImpulseResponse(void);
    std::vector<DSPCOMPLEX> &&getSignalProbe(void);
    std::vector<DSPCOMPLEX> &&getNullSymbol(void);
    std::vector<DSPCOMPLEX> &&getConstellationPoint(void);

    DABParams& getDABParams(void);

private:
    void Initialise(void);
    void ResetTechnicalData(void);
    void DeviceRestart(void);
    void DecoderRestart(bool isScan);
    void UpdateGUIData();

    // Back-end objects
    CVirtualInput* Device;
    QVariantMap commandLineOptions;
    DABParams dabparams;
    Channels channels;

    std::unique_ptr<RadioReceiver> my_rx;
    RingBuffer<int16_t> audioBuffer;
    CAudio audio;
    std::mutex signalProbeBufferMutex;
    std::vector<DSPCOMPLEX> getSignalProbeBuffer;
    std::mutex impulseResponseBufferMutex;
    std::vector<float> impulseResponseBuffer;
    std::mutex nullSymbolBufferMutex;
    std::vector<DSPCOMPLEX> nullSymbolBuffer;
    std::mutex constellationPointBufferMutex;
    std::vector<DSPCOMPLEX> constellationPointBuffer;

    // Objects set by the back-end
    QVariantMap mGUIData;
    QString mErrorMsg;
    QDateTime mCurrentDateTime;
    bool mIsSync;
    bool mIsFICCRC;
    bool mIsSignal;
    bool mIsStereo;
    bool mIsDAB;
    int mSNR;
    int mFrequencyCorrection;
    float mFrequencyCorrectionPpm;
    int mBitRate;
    int mAudioSampleRate;
    int mFrameErrors;
    int mRSErrors;
    int mAACErrors;
    int mGainCount;
    int mStationCount;

    QImage motImage;

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
    bool isAutoPlay;
    QString autoChannel;
    QString autoStation;

private slots:
    void StationTimerTimeout(void);
    void ChannelTimerTimeout(void);
    void SyncCheckTimerTimeout(void);
    void NextChannel(bool isWait);
    void addtoEnsemble(quint32 SId, const QString &Station);
    void displayDateTime(const dab_date_time_t& dateTime);

signals:
    void SwitchToNextChannel(bool isWait);
    void EnsembleAdded(quint32 SId, const QString& station);
    void EnsembleNameUpdated(const QString& name);
    void DateTimeUpdated(const dab_date_time_t& dateTime);

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
    void FrequencyCorrectionPpmChanged(float);
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
    void showErrorMessage(QString Text);
    void showInfoMessage(QString Text);
#endif

public slots:
    // This slots are called from the backend
    void nameofEnsemble(const QString&v);
    void onEventLoopStarted(void);
    void setErrorMessage(QString Text);
    /* head will be translated, text will be left untranslated */
    void setErrorMessage(const std::string& head, const std::string& text = "");
    void setInfoMessage(QString Text);
};

#endif // CRADIOCONTROLLER_H
