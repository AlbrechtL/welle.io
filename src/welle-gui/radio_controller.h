/*
 *    Copyright (C) 2019
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
#include <QFile>
#include <mutex>
#include <list>

#include "audio_output.h"
#include "dab-constants.h"
#include "radio-receiver.h"
#include "ringbuffer.h"
#include "channels.h"

class CVirtualInput;

enum class PlotTypeEn { Spectrum, ImpulseResponse, QPSK, Null, Unknown };

Q_DECLARE_METATYPE(CDeviceID)

class CRadioController :
    public QObject,
    public RadioControllerInterface,
    public ProgrammeHandlerInterface
{
    Q_OBJECT
    Q_PROPERTY(QString deviceName MEMBER deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(CDeviceID deviceId  MEMBER deviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(QDateTime dateTime MEMBER currentDateTime NOTIFY dateTimeChanged)
    Q_PROPERTY(bool isSync MEMBER isSync NOTIFY isSyncChanged)
    Q_PROPERTY(bool isFICCRC MEMBER isFICCRC NOTIFY isFICCRCChanged)
    Q_PROPERTY(bool isSignal MEMBER isSignal NOTIFY isSignalChanged)
    Q_PROPERTY(QString audioMode MEMBER audioMode NOTIFY audioModeChanged)
    Q_PROPERTY(bool isDAB MEMBER isDAB NOTIFY isDABChanged)
    Q_PROPERTY(int snr MEMBER snr NOTIFY snrChanged)
    Q_PROPERTY(int frequencyCorrection MEMBER frequencyCorrection NOTIFY frequencyCorrectionChanged)
    Q_PROPERTY(float frequencyCorrectionPpm MEMBER frequencyCorrectionPpm NOTIFY frequencyCorrectionPpmChanged)
    Q_PROPERTY(int bitRate MEMBER bitRate NOTIFY bitRateChanged)
    Q_PROPERTY(int frameErrors MEMBER frameErrors NOTIFY frameErrorsChanged)
    Q_PROPERTY(int rsErrors MEMBER rsErrors NOTIFY rsErrorsChanged)
    Q_PROPERTY(int aacErrors MEMBER aaErrors NOTIFY aacErrorsChanged)
    Q_PROPERTY(bool agc MEMBER isAGC WRITE setAGC NOTIFY agcChanged)
    Q_PROPERTY(float gainValue MEMBER currentManualGainValue NOTIFY gainValueChanged)
    Q_PROPERTY(int gainCount MEMBER gainCount NOTIFY gainCountChanged)
    Q_PROPERTY(int gain MEMBER currentManualGain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(qreal volume MEMBER currentVolume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QString errorMsg MEMBER errorMsg NOTIFY showErrorMessage)

    Q_PROPERTY(QString channel MEMBER currentChannel NOTIFY channelChanged)
    Q_PROPERTY(QStringList lastChannel MEMBER currentLastChannel NOTIFY lastChannelChanged)
    Q_PROPERTY(QString ensemble MEMBER currentEnsembleLabel NOTIFY ensembleChanged)
    Q_PROPERTY(int frequency MEMBER currentFrequency NOTIFY frequencyChanged)
    Q_PROPERTY(quint32 service MEMBER currentService NOTIFY stationChanged)
    Q_PROPERTY(QString stationType MEMBER currentStationType NOTIFY stationTypChanged)
    Q_PROPERTY(QString languageType MEMBER currentLanguageType NOTIFY languageTypeChanged)
    Q_PROPERTY(QString title MEMBER currentTitle NOTIFY titleChanged)
    Q_PROPERTY(QString text MEMBER currentText NOTIFY textChanged)

public:
    CRadioController(QVariantMap &commandLineOptions, QObject* parent = nullptr);
    ~CRadioController();
    CRadioController(const CRadioController&) = delete;
    void operator=(const CRadioController&) = delete;

    void closeDevice();
    CDeviceID openDevice(CDeviceID deviceId, bool force = false, QVariant param1 = QVariant(), QVariant param2 = QVariant());
    CDeviceID openDevice();
    void setDeviceParam(QString param, int value);
    void setDeviceParam(QString param, QString value);
    Q_INVOKABLE void play(QString channel, QString title, quint32 service);
    void pause();
    void stop();
    void setService(uint32_t service, bool force = false);
    void setChannel(QString Channel, bool isScan, bool Force = false);
    Q_INVOKABLE void setManualChannel(QString Channel);
    Q_INVOKABLE void startScan(void);
    Q_INVOKABLE void stopScan(void);
    Q_INVOKABLE void setAutoPlay(bool isAutoPlayValue, QString channel, QString serviceid_as_string);
    Q_INVOKABLE void setVolume(qreal volume);
    Q_INVOKABLE void setAGC(bool isAGC);
    Q_INVOKABLE void disableCoarseCorrector(bool disable);
    Q_INVOKABLE void enableTIIDecode(bool enable);
    Q_INVOKABLE void selectFFTWindowPlacement(int fft_window_placement_ix);
    Q_INVOKABLE void setFreqSyncMethod(int fsm_ix);
    Q_INVOKABLE void setGain(int gain);
    Q_INVOKABLE void initRecorder(int size);
    Q_INVOKABLE void triggerRecorder(QString filename);
    DABParams& getParams(void);
    int getCurrentFrequency();

    // Buffer getter
    std::vector<float> getImpulseResponse(void);
    std::vector<DSPCOMPLEX> getSignalProbe(void);
    std::vector<DSPCOMPLEX> getNullSymbol(void);
    std::vector<DSPCOMPLEX> getConstellationPoint(void);

    //called from the backend
    virtual void onFrameErrors(int frameErrors) override;
    virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, const std::string& mode) override;
    virtual void onRsErrors(bool uncorrectedErrors, int numCorrectedErrors) override;
    virtual void onAacErrors(int aacErrors) override;
    virtual void onNewDynamicLabel(const std::string& label) override;
    virtual void onMOT(const mot_file_t& mot_file) override;
    virtual void onPADLengthError(size_t announced_xpad_len, size_t xpad_len) override;
    virtual void onSNR(int snr) override;
    virtual void onFrequencyCorrectorChange(int fine, int coarse) override;
    virtual void onSyncChange(char isSync) override;
    virtual void onSignalPresence(bool isSignal) override;
    virtual void onServiceDetected(uint32_t sId) override;
    virtual void onNewEnsemble(uint16_t eId) override;
    virtual void onSetEnsembleLabel(DabLabel& label) override;
    virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override;
    virtual void onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib) override;
    virtual void onNewImpulseResponse(std::vector<float>&& data) override;
    virtual void onConstellationPoints(std::vector<DSPCOMPLEX>&& data) override;
    virtual void onNewNullSymbol(std::vector<DSPCOMPLEX>&& data) override;
    virtual void onTIIMeasurement(tii_measurement_t&& m) override;
    virtual void onMessage(message_level_t level, const std::string& text, const std::string& text2 = std::string()) override;
    virtual void onInputFailure(void) override;

private:
    void initialise(void);
    void resetTechnicalData(void);
    void deviceRestart(void);

    std::shared_ptr<CVirtualInput> device;
    QVariantMap commandLineOptions;
    std::map<DeviceParam, std::string> deviceParametersString;
    std::map<DeviceParam, int> deviceParametersInt;
    Channels channels;
    RadioReceiverOptions rro;

    std::unique_ptr<RadioReceiver> radioReceiver;
    RingBuffer<int16_t> audioBuffer;
    CAudio audio;
    std::mutex impulseResponseBufferMutex;
    std::vector<float> impulseResponseBuffer;
    std::mutex nullSymbolBufferMutex;
    std::vector<DSPCOMPLEX> nullSymbolBuffer;
    std::mutex constellationPointBufferMutex;
    std::vector<DSPCOMPLEX> constellationPointBuffer;

    QString errorMsg;
    QDateTime currentDateTime;
    bool isSync = false;
    bool isFICCRC = false;
    bool isSignal = false;
    bool isDAB = false;
    QString audioMode = "";
    int snr = 0;
    int frequencyCorrection = 0;
    float frequencyCorrectionPpm = 0.0;
    int bitRate = 0;
    int audioSampleRate = 0;
    int frameErrors = 0;
    int rsErrors = 0;
    int aaErrors = 0;
    int gainCount = 0;
    int stationCount = 0;

    QString currentChannel;
    QStringList currentLastChannel;
    std::list<uint32_t> pendingLabels;
    QString currentEnsembleLabel;
    uint16_t currentEId;
    int32_t currentFrequency;
    uint32_t currentService;
    QString currentStationType;
    QString currentLanguageType;
    QString currentTitle;
    QString currentText;
    int32_t currentManualGain;
    float currentManualGainValue = 0.0;
    qreal currentVolume = 1.0;
    QString deviceName = "Unknown";
    CDeviceID deviceId = CDeviceID::UNKNOWN;

    QTimer labelTimer;
    QTimer stationTimer;
    QTimer channelTimer;

    bool isChannelScan = false;
    bool isAGC = false;
    bool isAutoPlay = false;
    QString autoChannel;
    quint32 autoService;

#ifdef __ANDROID__
    std::unique_ptr<QFile> rawFileAndroid;
#endif

public slots:
    void setErrorMessage(QString Text);
    void setErrorMessage(const std::string& head, const std::string& text = "");
    void setInfoMessage(QString Text);

private slots:
    void ensembleId(quint16);
    void ensembleLabel(DabLabel&);
    void serviceId(quint32);
    void labelTimerTimeout(void);
    void stationTimerTimeout(void);
    void channelTimerTimeout(void);
    void nextChannel(bool isWait);
    void displayDateTime(const dab_date_time_t& dateTime);

signals:
    void switchToNextChannel(bool isWait);
    void serviceDetected(quint32 sId);
    void ensembleIdUpdated(quint16 eId);
    void ensembleLabelUpdated(DabLabel& label);
    void dateTimeUpdated(const dab_date_time_t& dateTime);

    void deviceNameChanged();
    void deviceIdChanged();
    void dateTimeChanged(QDateTime);
    void isSyncChanged(bool);
    void isFICCRCChanged(bool);
    void isSignalChanged(bool);
    void isDABChanged(bool);
    void audioModeChanged(QString);
    void snrChanged(int);
    void frequencyCorrectionChanged(int);
    void frequencyCorrectionPpmChanged(float);
    void bitRateChanged(int);
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
    void motChanged(mot_file_t);
    void motReseted(void);

    void channelChanged();
    void lastChannelChanged();
    void ensembleChanged();
    void frequencyChanged();
    void stationChanged();
    void stationTypChanged();
    void titleChanged();
    void textChanged();
    void languageTypeChanged();

    void deviceReady();
    void deviceClosed();
    void stationsCleared();
    void foundStation(QString Station, QString currentChannel);
    void newStationNameReceived(QString station, quint32 sId, QString channel);
    void scanStopped();
    void scanProgress(int Progress);
    void showErrorMessage(QString Text);
    void showInfoMessage(QString Text);
};

#endif // CRADIOCONTROLLER_H
