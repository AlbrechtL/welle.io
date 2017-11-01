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
    connect(this, &CSDRDABInterface::FICDataUpdated, this, &CSDRDABInterface::FICDataUpdate);

    SDRDevice = SDRDevice_t::Unknown;
    RAWFile = "";
}

void CSDRDABInterface::SetRAWInput(QString File)
{
    SDRDevice = SDRDevice_t::RAW;
    RAWFile = File;
}


void CSDRDABInterface::Start()
{
    //Launch a thread
    SchedulerThread = new std::thread(SchedularThreadWrapper, this);
}

void CSDRDABInterface::SchedularThreadWrapper(CSDRDABInterface *SDRDABInterface)
{
    if(SDRDABInterface)
        SDRDABInterface->SchedulerRunThread();
}

void CSDRDABInterface::SchedulerRunThread()
{
    SchedulerConfig_t config; //invokes default SchedulerConfig_t constructor
    config.sampling_rate  = 2048000;

    if(SDRDevice == SDRDevice_t::RAW)
    {
        QByteArray *ba = new QByteArray(RAWFile.toLocal8Bit()); // ToDo Memory leak!!
        config.input_filename = ba->data();
        config.data_source = Scheduler::DATA_FROM_FILE;
    }
    else // Assume RTL-SDR
    {
        config.data_source = Scheduler::DATA_FROM_DONGLE;
        config.carrier_frequency = 202928000;
    }

    //config.use_speakers = !(this_->user_input_->silent_);
    //config.output_filename = this_->user_input_->output_;
    //config.convolutional_alg = this_->user_input_->decodingAlg_;
    //if (this_->user_input_->channel_nr > 0)
    //    config.start_station_nr = this_->user_input_->channel_nr;

    this->Scheduler::Start(config);
}

/********* sdrdab overrides *********/
void CSDRDABInterface::ParametersFromSDR(Scheduler::scheduler_error_t error_code)
{
    switch(error_code)
    {
    case OK: qDebug() << "SDRDABInterface: no error"; break;
    case ERROR_UNKNOWN: qDebug() << "SDRDABInterface: unknown error"; break;
    case FILE_NOT_FOUND: qDebug() << "SDRDABInterface: FileDataFeeder was unable to open raw file" << RAWFile; break;
    case DEVICE_NOT_FOUND: qDebug() << "SDRDABInterface: DataFeeder was unable to use tuner"; break;
    case DEVICE_DISCONNECTED: qDebug() << "SDRDABInterface: tuner device has been disconnected"; break;
    case FILE_END: qDebug() << "SDRDABInterface: input file with raw samples has ended"; break;
    case DAB_NOT_DETECTED: qDebug() << "SDRDABInterface: DAB signal was not detected"; break;
    case STATION_NOT_FOUND: qDebug() << "SDRDABInterface: given station number is incorrect"; break;
    }
}

void CSDRDABInterface::ParametersFromSDR(float snr)
{

}

void CSDRDABInterface::ParametersFromSDR(UserFICData_t *user_fic_extra_data)
{
    // Check for valid data
    if(!user_fic_extra_data)
        return;

    // Copy FIC data
    FICDataMutex.lock();
    FICData.Set(user_fic_extra_data);
    FICDataMutex.unlock();

    // Fire signal
    emit FICDataUpdated();

    delete user_fic_extra_data;
}

void CSDRDABInterface::FICDataUpdate()
{
    FICDataMutex.lock();

    // Check for new stations
    for(auto station : FICData.stations)
    {
        QString StationName = QString::fromStdString(station.station_name);

        //	Add new station into list
        if ((StationName != "Packet Mode") &&
            (StationName != "Not Available") &&
            (!StationList.contains(StationName)))
        {
            qDebug() << "SDRDABInterface: Found station" <<  StationName
                     << "(" << qPrintable(QString::number(station.ServiceId, 16).toUpper()) << ")"
                     << "SubChannelId " << station.SubChannelId;

            StationList.append(StationName);
            emit NewStationFound(StationName);
        }
    }

    FICDataMutex.unlock();
}

void CSDRDABInterface::TuneToStation(QString StationName)
{
    FICDataMutex.lock();
    // Search for station
    for(auto station : FICData.stations)
    {
        if(QString::fromStdString(station.station_name) == StationName)
        {
            this->ParametersToSDR(STATION_NUMBER, station.SubChannelId);
            break; // break for loop
        }
    }
    FICDataMutex.unlock();
}
