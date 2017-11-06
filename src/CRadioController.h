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

//#include "CAudio.h"
#include "CStationList.h"
#include "CChannels.h"
#include "scheduler.h"
#include "CSDRDABInterface.h"

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



private:
    void Initialise(void);
    void ResetTechnicalData(void);
    void NextChannel(bool isWait);
    void UpdateGUIData();
    void SetFrequencyCorrection(int FrequencyCorrection);

    CSDRDABInterface SDRDABInterface;

    // Back-end objects
    QVariantMap commandLineOptions;

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
    void NewStation(QString StationName);
    void setErrorMessage(QString Text);
    void setInfoMessage(QString Text);

#ifndef Q_OS_ANDROID
signals:
    void SpectrumUpdated(qreal Ymax, qreal Xmin, qreal Xmax, QVector<QPointF> Data);
    void GUIDataChanged(QVariantMap GUIData);
    void FoundStation(QString Station, QString CurrentChannel);
    void ScanStopped();
    void ScanProgress(int Progress);
    void StationsCleared();
    void MOTChanged(QImage MOTImage);
    void StationsChanged(QList<StationElement*> Stations);
    void FICExtraDataUpdated(void);
    void showErrorMessage(QString Text);
     void showInfoMessage(QString Text);
#endif

public slots:
    void onEventLoopStarted(void);

};

#endif // CRADIOCONTROLLER_H
