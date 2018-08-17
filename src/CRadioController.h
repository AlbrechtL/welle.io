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
    Q_PROPERTY(QString dateTime READ dateTime NOTIFY dateTimeChanged)
    Q_PROPERTY(bool isSync READ isSync NOTIFY isSyncChanged)
    Q_PROPERTY(bool isFICCRC READ isFICCRC NOTIFY isFICCRCChanged)
    Q_PROPERTY(bool isSignal READ isSignal NOTIFY isSignalChanged)
    Q_PROPERTY(bool isStereo READ isStereo NOTIFY isStereoChanged)
    Q_PROPERTY(bool isDAB READ isDAB NOTIFY isDABChanged)
    Q_PROPERTY(int snr READ snr NOTIFY snrChanged)
    Q_PROPERTY(int frequencyCorrection READ frequencyCorrection NOTIFY frequencyCorrectionChanged)
    Q_PROPERTY(float frequencyCorrectionPpm READ frequencyCorrectionPpm NOTIFY frequencyCorrectionPpmChanged)
    Q_PROPERTY(int bitRate READ bitRate NOTIFY bitRateChanged)
    Q_PROPERTY(int audioSampleRate READ audioSampleRate NOTIFY audioSampleRateChanged)
    Q_PROPERTY(int frameErrors READ frameErrors NOTIFY frameErrorsChanged)
    Q_PROPERTY(int rsErrors READ rsErrors NOTIFY rsErrorsChanged)
    Q_PROPERTY(int aacErrors READ aacErrors NOTIFY aacErrorsChanged)
    Q_PROPERTY(bool isHwAGCSupported READ isHwAGCSupported NOTIFY isHwAGCSupportedChanged)
    Q_PROPERTY(bool hwAgc READ hwAgc WRITE setHwAGC NOTIFY hwAgcChanged)
    Q_PROPERTY(bool agc READ agc WRITE setAGC NOTIFY agcChanged)
    Q_PROPERTY(float gainValue READ gainValue NOTIFY gainValueChanged)
    Q_PROPERTY(int gainCount READ gainCount NOTIFY gainCountChanged)
    Q_PROPERTY(int gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QList<StationElement*> stations READ stations NOTIFY stationsChanged)
    Q_PROPERTY(QVariantMap guiData READ guiData NOTIFY guiDataChanged)
    Q_PROPERTY(QString errorMsg READ errorMsg NOTIFY showErrorMessage)
    Q_PROPERTY(QImage mot READ mot NOTIFY motChanged)
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
    void openDevice(CVirtualInput* new_device);
    Q_INVOKABLE void play(QString Channel, QString Station);
    void pause();
    void stop();
    Q_INVOKABLE void clearStations();
    void setStation(QString Station, bool Force = false);
    void setChannel(QString Channel, bool isScan, bool Force = false);
    Q_INVOKABLE void setManualChannel(QString Channel);
    Q_INVOKABLE void startScan(void);
    Q_INVOKABLE void stopScan(void);
    void setAutoPlay(QString Channel, QString Station);    
    void setVolume(qreal volume);
    Q_INVOKABLE void setAGC(bool isAGC);
    Q_INVOKABLE void setHwAGC(bool isHwAGC);
    Q_INVOKABLE void disableCoarseCorrector(bool disable);
    Q_INVOKABLE void enableTIIDecode(bool enable);
    Q_INVOKABLE void enableOldFFTWindowPlacement(bool old);
    Q_INVOKABLE void setFreqSyncMethod(int fsm_ix);
    void setGain(int gain);
    DABParams& getDABParams(void);
    int getCurrentFrequency();

    // Buffer getter
    std::vector<float> getImpulseResponse(void);
    std::vector<DSPCOMPLEX> getSignalProbe(void);
    std::vector<DSPCOMPLEX> getNullSymbol(void);
    std::vector<DSPCOMPLEX> getConstellationPoint(void);

    //called from the backend
    virtual void onFrameErrors(int frameErrors) override;
    virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, bool isStereo, const std::string& mode) override;
    virtual void onRsErrors(int rsErrors) override;
    virtual void onAacErrors(int aacErrors) override;
    virtual void onNewDynamicLabel(const std::string& label) override;
    virtual void onMOT(const std::vector<uint8_t>& data, int subtype) override;
    virtual void onPADLengthError(size_t announced_xpad_len, size_t xpad_len) override;
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

    QList<StationElement*> stations() const;
    QVariantMap guiData(void) const;
    QString errorMsg() const;
    QImage mot() const;
    QString dateTime() const;
    bool isSync() const;
    bool isFICCRC() const;
    bool isSignal() const;
    bool isStereo() const;
    bool isDAB() const;
    int snr() const;
    int frequencyCorrection() const;
    float frequencyCorrectionPpm() const;
    int bitRate() const;
    int audioSampleRate() const;
    int frameErrors() const;
    int rsErrors() const;
    int aacErrors() const;
    qreal volume() const;
    bool isHwAGCSupported() const;
    bool hwAgc() const;
    bool agc() const;
    int gain() const;
    int gainCount() const;
    float gainValue() const;

private:
    void initialise(void);
    void resetTechnicalData(void);
    void deviceRestart(void);
    void decoderRestart(bool isScan);
    void updateGUIData();

    // Back-end objects
    std::shared_ptr<CVirtualInput> device;
    QVariantMap commandLineOptions;
    DABParams dabparams;
    Channels channels;
    RadioReceiverOptions rro;

    std::unique_ptr<RadioReceiver> my_rx;
    RingBuffer<int16_t> audioBuffer;
    CAudio audio;
    std::mutex impulseResponseBufferMutex;
    std::vector<float> impulseResponseBuffer;
    std::mutex nullSymbolBufferMutex;
    std::vector<DSPCOMPLEX> nullSymbolBuffer;
    std::mutex constellationPointBufferMutex;
    std::vector<DSPCOMPLEX> constellationPointBuffer;

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

    DabStatus dabStatus;
    QString currentChannel;
    QString currentEnsemble;
    int32_t currentFrequency;
    QString currentStation;
    QString currentStationType;
    QString currentLanguageType;
    QString currentTitle;
    QString currentText;
    int32_t currentManualGain;
    float currentManualGainValue;
    qreal currentVolume;

    CStationList stationList;
    QList<QString> stationListStr;
    QTimer stationTimer;
    QTimer channelTimer;
    QTimer syncCheckTimer;

    bool startPlayback;
    bool isChannelScan;
    bool isAGC;
    bool isHwAGC;
    bool isAutoPlay;
    QString autoChannel;
    QString autoStation;

private slots:
    void stationTimerTimeout(void);
    void channelTimerTimeout(void);
    void syncCheckTimerTimeout(void);
    void nextChannel(bool isWait);
    void addtoEnsemble(quint32 SId, const QString &Station);
    void displayDateTime(const dab_date_time_t& dateTime);

signals:
    void switchToNextChannel(bool isWait);
    void ensembleAdded(quint32 SId, const QString& station);
    void ensembleNameUpdated(const QString& name);
    void dateTimeUpdated(const dab_date_time_t& dateTime);

#ifndef Q_OS_ANDROID
signals:
    void dateTimeChanged(QString);
    void isSyncChanged(bool);
    void isFICCRCChanged(bool);
    void isSignalChanged(bool);
    void isStereoChanged(bool);
    void isDABChanged(bool);
    void snrChanged(int);
    void frequencyCorrectionChanged(int);
    void frequencyCorrectionPpmChanged(float);
    void bitRateChanged(int);
    void audioSampleRateChanged(int);
    void frameErrorsChanged(int);
    void rsErrorsChanged(int);
    void aacErrorsChanged(int);
    void gainCountChanged(int);

    void isHwAGCSupportedChanged(bool);
    void hwAgcChanged(bool);
    void agcChanged(bool);
    void gainValueChanged(float);
    void gainChanged(int);
    void volumeChanged(qreal);
    void motChanged(QImage MOTImage);

    void stationsChanged(QList<StationElement*> stations);
    void guiDataChanged(QVariantMap guiData);
    void deviceReady();
    void deviceClosed();
    void stationsCleared();
    void foundStation(QString Station, QString currentChannel);
    void scanStopped();
    void scanProgress(int Progress);
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
