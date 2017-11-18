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

#include <thread>

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QImage>
#include <QVariantMap>
#include <QTimer>

#include "CStationList.h"
#include "CChannels.h"
#include "scheduler.h"
#include "CSdrDabInterface.h"

class CVirtualInput;

#ifdef Q_OS_ANDROID
#include "rep_CRadioController_source.h"
class CRadioController : public CRadioControllerSource
{
    Q_OBJECT
#else
class CRadioController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isDAB READ isDAB NOTIFY isDABChanged)
    Q_PROPERTY(int BitRate READ BitRate NOTIFY BitRateChanged)
    Q_PROPERTY(bool isStereo READ isStereo NOTIFY isStereoChanged)
    Q_PROPERTY(int SNR READ SNR NOTIFY SNRChanged)
    Q_PROPERTY(int FrequencyCorrection READ FrequencyCorrection NOTIFY FrequencyCorrectionChanged)

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
        Tuning      = 6
    };
    Q_ENUMS(DabStatus)

    CRadioController(QVariantMap &commandLineOptions, QObject* parent = NULL);
    ~CRadioController(void);
    void closeDevice();
    void openDevice(CVirtualInput* Dev);
    void play(QString Channel, QString Station, int SubChannelID = 255);
    void pause();
    void stop();
    void clearStations();
    void setChannel(QString Channel, bool isScan, bool Force = false);
    void setManualChannel(QString Channel);
    void startScan(void);
    void stopScan(void);
    void updateSpectrum(void);
    QList<StationElement*> stations() const;
    QVariantMap guiData(void) const;
    QImage mot() const;

    bool isDAB() const;
    int BitRate() const;
    bool isStereo() const;
    int SNR() const;
    int FrequencyCorrection() const;

private:
    void initialise(void);
    void resetTechnicalData(void);
    void nextChannel(bool isWait);
    void updateGUIData();

    CSdrDabInterface SDRDABInterface;

    // Back-end objects
    std::shared_ptr<CVirtualInput> Device;
    QVariantMap commandLineOptions;
    bool mIsDAB;
    int mBitRate;
    bool mIsStereo;
    int mSNR;
    QString ProgrammeType;
    int mFrequencyCorrection;

    // Objects set by the back-end
    QVariantMap mGUIData;

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

    // Spectrum variables
//    common_fft* spectrum_fft_handler;
    QVector<QPointF> spectrum_data;

private slots:
    void newStation(QString StationName, uint8_t SubChannelId);
    void ficUpdate(bool isDABPlus, size_t bitrate, QString programme_type);
    void newSnrValue(int SNR);
    void newFcDriftValue(int estimated_fc_drift);

#ifndef Q_OS_ANDROID
signals:
    void isDABChanged(bool);
    void BitRateChanged(int);
    void isStereoChanged(bool);
    void SNRChanged(int);
    void FrequencyCorrectionChanged(int);

    void SpectrumUpdated(qreal Ymax, qreal Xmin, qreal Xmax, QVector<QPointF> Data);
    void GUIDataChanged(QVariantMap guiData);
    void FoundStation(QString Station, QString CurrentChannel);
    void ScanStopped();
    void ScanProgress(int Progress);
    void StationsCleared();
    void MOTChanged(QImage MOTImage);
    void StationsChanged(QList<StationElement*> stations);
    void FICExtraDataUpdated(void);
    void showErrorMessage(QString Text);
    void showInfoMessage(QString Text);
#endif

public slots:
    void onEventLoopStarted(void);
    void setErrorMessage(QString Text);
    void setInfoMessage(QString Text);

// Static members
private:
    static CRadioController *m_RadioController;

public:
    static std::shared_ptr<CVirtualInput> getDevice(void);
    static void setStereo(bool isStereo);
};

#endif // CRADIOCONTROLLER_H
