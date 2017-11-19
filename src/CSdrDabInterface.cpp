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

#include <QDebug>

#include "CSdrDabInterface.h"

CSdrDabInterface::CSdrDabInterface(QObject *parent) : QObject(parent)
{
    connect(this, &CSdrDabInterface::ficDataUpdated, this, &CSdrDabInterface::ficDataUpdate);

    //VerbosityOn();
}

CSdrDabInterface::~CSdrDabInterface()
{
    stop();
}

void CSdrDabInterface::start(bool isAudio, uint8_t stationNumber)
{
    m_isAudio = isAudio;
    m_stationNumber = stationNumber;

    //Launch a thread
    m_SchedulerThread = new std::thread(schedularThreadWrapper, this);
}

void CSdrDabInterface::stop()
{
    this->Scheduler::Stop();

    // Wait for thead end
    m_SchedulerThread->join();
    delete m_SchedulerThread;
}

void CSdrDabInterface::schedularThreadWrapper(CSdrDabInterface *SDRDABInterface)
{
    if(SDRDABInterface)
        SDRDABInterface->schedulerRunThread();
}

void CSdrDabInterface::schedulerRunThread()
{
    SchedulerConfig_t config; //invokes default SchedulerConfig_t constructor
    config.sampling_rate  = 2048000;
    config.start_station_nr = m_stationNumber; // Start with first channel (255 = default)
    config.use_speakers = m_isAudio;

    //config.input_filename = "../DAB-Test/rai12d.raw";
    //config.data_source = Scheduler::DATA_FROM_FILE;
    //config.data_source = Scheduler::DATA_FROM_DONGLE;

    config.data_source = Scheduler::DATA_FROM_WELLE_IO;
    config.carrier_frequency = 178352000; // 5C
    //config.carrier_frequency = 222064000; // 11D

    this->Scheduler::Start(config);
}

/********* sdrdab overrides *********/
void CSdrDabInterface::ParametersFromSDR(Scheduler::scheduler_error_t error_code)
{
    switch(error_code)
    {
    case OK: qDebug() << "CSdrDabInterface: no error"; break;
    case ERROR_UNKNOWN: qDebug() << "CSdrDabInterface: unknown error"; break;
    case FILE_NOT_FOUND: qDebug() << "CSdrDabInterface: FileDataFeeder was unable to open raw file"; break;
    case DEVICE_NOT_FOUND: qDebug() << "CSdrDabInterface: DataFeeder was unable to use tuner"; break;
    case DEVICE_DISCONNECTED: qDebug() << "CSdrDabInterface: tuner device has been disconnected"; break;
    case FILE_END: qDebug() << "CSdrDabInterface: input file with raw samples has ended"; break;
    case DAB_NOT_DETECTED: qDebug() << "CSdrDabInterface: DAB signal was not detected"; break;
    case STATION_NOT_FOUND: qDebug() << "CSdrDabInterface: given station number is incorrect"; break;
    }
}

void CSdrDabInterface::ParametersFromSDR(float snr, float estimated_fc_drift)
{
    //qDebug() << "CSdrDabInterface: SNR " <<  snr;

    // Round to two decimal points
    int snr_round = roundf(snr * 100);

    if(snr_round != m_SNR)
    {
        m_SNR = snr_round;
        snrChanged(m_SNR);
    }

    int m_estimated_fc_drift_round = roundf(estimated_fc_drift * 1000); // Now we have Hz

    if(m_estimated_fc_drift_round != m_estimated_fc_drift)
    {
        m_estimated_fc_drift = m_estimated_fc_drift_round;
        fcDriftChanged(m_estimated_fc_drift);
    }
}

void CSdrDabInterface::ParametersFromSDR(Scheduler::state_t state)
{
    static const QString states_string[9] = {"INIT",
                  "SYNC",
                  "CONF",
                  "CONFSTATION",
                  "CONFCONVALG",
                  "PLAY",
                  "INIT_ERROR",
                  "INTERNAL_ERROR",
                  "EXTERNAL_STOP"};

    if(m_state != state)
    {
        m_state = state;
        qDebug() << "CSdrDabInterface: State:" <<  states_string[m_state];

        if(m_state == CONF || m_state == CONFSTATION || m_state == PLAY)
            emit syncStateChanged(true);
        else
            emit syncStateChanged(false);
    }
}

void CSdrDabInterface::ParametersFromSDR(UserFICData_t *user_fic_extra_data)
{
    // Check for valid data
    if(!user_fic_extra_data)
        return;

    // Copy FIC data
    m_FICDataMutex.lock();
    m_FICData.Set(user_fic_extra_data);
    m_FICDataMutex.unlock();

    // Fire signal
    emit ficDataUpdated();

    delete user_fic_extra_data;
}

void CSdrDabInterface::ficDataUpdate()
{
    m_FICDataMutex.lock();

    // Check for new stations
    for(auto station : m_FICData.stations)
    {
        QString StationName = QString::fromStdString(station.station_name);

        //	Add new station into list
        if ((StationName != "Packet Mode") && // Todo Find a better way
            (StationName != "Not Available") &&  // Todo Find a better way
            (!m_StationList.contains(StationName)))
        {
            qDebug() << "CSdrDabInterface: Found station" <<  StationName
                     << "(" << qPrintable(QString::number(station.ServiceId, 16).toUpper()) << ")"
                     << "SubChannelId " << station.SubChannelId;

            m_StationList.append(StationName);
            emit newStationFound(StationName, station.SubChannelId);
        }
    }

    emit stationInfoUpdate(m_FICData.DAB_plus_, m_FICData.bitrate_, QString(m_FICData.programme_type_));
    m_FICDataMutex.unlock();
}

void CSdrDabInterface::tuneToStation(int SubChannelID)
{
    // ToDo Scrutinize if SNR calculation is correct
    this->ParametersToSDR(STATION_NUMBER, (uint8_t) SubChannelID);
}
