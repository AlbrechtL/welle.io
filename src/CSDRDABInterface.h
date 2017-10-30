/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
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

#ifndef CSDRDABINTERFACE_H
#define CSDRDABINTERFACE_H

#include <thread>
#include <mutex>
#include <QObject>

// SDRDAB header
#include "scheduler.h"

// welle.io header
#include "CFICData.h"

enum class SDRDevice_t {Unknown, RAW, RTLSDR};

class CSDRDABInterface : public QObject, public Scheduler
{
    Q_OBJECT
public:
    explicit CSDRDABInterface(QObject *parent = nullptr);
    void SetRAWInput(QString File);
    void Start(void);
    void TuneToStation(QString StationName);

private:
    static void SchedularThreadWrapper(CSDRDABInterface *SDRDABInterface);
    void SchedulerRunThread(void);

    /********* sdrdab overrides *********/
    /**
     * "Callback" executed whenever something interesting happens.
     * Error code variant.
     * @brief Error callback
     * @param[in] error_code error code
     */
    virtual void ParametersFromSDR(scheduler_error_t error_code);

    /**
     * "Callback" executed whenever something interesting happens.
     * SNR variant.
     * @brief SNR measurement callback
     * @param[in] snr current SNR level [dB]
     */
    virtual void ParametersFromSDR(float snr);

    /**
     * "Callback" executed whenever something interesting happens.
     * FIG & SlideShow variant.
     * @brief new UserFICData_t callback
     * @param[in] user_fic_extra_data pointer to the structure
     * @note user_fic_extra_data has to be freed before return!
     */
    virtual void ParametersFromSDR(UserFICData_t *user_fic_extra_data);


    std::thread *SchedulerThread;
    std::mutex FICDataMutex;
    CFICData FICData;
    QList<QString> StationList;

    SDRDevice_t SDRDevice;
    QString RAWFile;

signals:
    void FICDataUpdated(void);
    void NewStationFound(QString StationName);

public slots:
    void FICDataUpdate(void);
};

#endif // CSDRDABINTERFACE_H
