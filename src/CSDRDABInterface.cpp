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

#include "CSDRDABInterface.h"

CSDRDABInterface::CSDRDABInterface(QObject *parent) : QObject(parent)
{
    connect(this, &CSDRDABInterface::ficDataUpdated, this, &CSDRDABInterface::ficDataUpdate);

    m_SDRDevice = SDRDevice_t::Unknown;
    m_RAWFile = "";
}

CSDRDABInterface::~CSDRDABInterface()
{
    stop();
}

void CSDRDABInterface::setRAWInput(QString File)
{
    m_SDRDevice = SDRDevice_t::RAW;
    m_RAWFile = File;
}

void CSDRDABInterface::start(bool isAudio, uint8_t stationNumber)
{
    m_isAudio = isAudio;
    m_stationNumber = stationNumber;

    //Launch a thread
    m_SchedulerThread = new std::thread(schedularThreadWrapper, this);
}

void CSDRDABInterface::stop()
{
    this->Scheduler::Stop();

    // Wait for thead end
    m_SchedulerThread->join();
    delete m_SchedulerThread;
}

void CSDRDABInterface::schedularThreadWrapper(CSDRDABInterface *SDRDABInterface)
{
    if(SDRDABInterface)
        SDRDABInterface->schedulerRunThread();
}

void CSDRDABInterface::schedulerRunThread()
{
    SchedulerConfig_t config; //invokes default SchedulerConfig_t constructor
    config.sampling_rate  = 2048000;
    config.start_station_nr = m_stationNumber; // Start with first channel (255 = default)
    config.use_speakers = m_isAudio;

    if(m_SDRDevice == SDRDevice_t::RAW)
    {
        QByteArray *ba = new QByteArray(m_RAWFile.toLocal8Bit()); // ToDo Memory leak!!
        config.input_filename = ba->data();
        config.data_source = Scheduler::DATA_FROM_FILE;
    }
    else // Assume RTL-SDR
    {
        config.data_source = Scheduler::DATA_FROM_DONGLE;
        config.carrier_frequency = 202928000;
    }

    this->Scheduler::Start(config);
}

/********* sdrdab overrides *********/
void CSDRDABInterface::ParametersFromSDR(Scheduler::scheduler_error_t error_code)
{
    switch(error_code)
    {
    case OK: qDebug() << "SDRDABInterface: no error"; break;
    case ERROR_UNKNOWN: qDebug() << "SDRDABInterface: unknown error"; break;
    case FILE_NOT_FOUND: qDebug() << "SDRDABInterface: FileDataFeeder was unable to open raw file" << m_RAWFile; break;
    case DEVICE_NOT_FOUND: qDebug() << "SDRDABInterface: DataFeeder was unable to use tuner"; break;
    case DEVICE_DISCONNECTED: qDebug() << "SDRDABInterface: tuner device has been disconnected"; break;
    case FILE_END: qDebug() << "SDRDABInterface: input file with raw samples has ended"; break;
    case DAB_NOT_DETECTED: qDebug() << "SDRDABInterface: DAB signal was not detected"; break;
    case STATION_NOT_FOUND: qDebug() << "SDRDABInterface: given station number is incorrect"; break;
    }
}

void CSDRDABInterface::ParametersFromSDR(float snr)
{
    qDebug() << "SDRDABInterface: SNR " <<  snr;
}

void CSDRDABInterface::ParametersFromSDR(UserFICData_t *user_fic_extra_data)
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

void CSDRDABInterface::ficDataUpdate()
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
            qDebug() << "SDRDABInterface: Found station" <<  StationName
                     << "(" << qPrintable(QString::number(station.ServiceId, 16).toUpper()) << ")"
                     << "SubChannelId " << station.SubChannelId;

            m_StationList.append(StationName);
            emit newStationFound(StationName, station.SubChannelId);
        }
    }

    m_FICDataMutex.unlock();
}

void CSDRDABInterface::tuneToStation(int SubChannelID)
{
    this->ParametersToSDR(STATION_NUMBER, (uint8_t) SubChannelID);
}

SDRDevice_t CSDRDABInterface::getSDRDevice()
{
    return m_SDRDevice;
}
