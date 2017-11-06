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

#include <QDebug>
#include <QSettings>

#include "CRadioController.h"
//#include "CInputFactory.h"
//#include "CRAWFile.h"
//#include "CRTL_TCP_Client.h"

#define AUDIOBUFFERSIZE 32768

CRadioController::CRadioController(QVariantMap& commandLineOptions, QObject *parent)
#ifdef Q_OS_ANDROID
    : CRadioControllerSource(parent)
#else
    : QObject(parent)
#endif
    , commandLineOptions(commandLineOptions)
{

    MOTImage = new QImage();

//    spectrum_fft_handler = new common_fft(DABParams.T_u);

    // Init the technical data
    ResetTechnicalData();

    // Read channels from settings
    mStationList.loadStations();
    mStationList.sort();
    emit StationsChanged(mStationList.getList());

    // Init SDRDAB interface
    connect(&SDRDABInterface, &CSDRDABInterface::NewStationFound, this, &CRadioController::NewStation);
}

CRadioController::~CRadioController(void)
{
//    delete Audio;
}

void CRadioController::ResetTechnicalData(void)
{
    Status = Unknown;
    CurrentChannel = tr("Unknown");
    CurrentEnsemble = "";
    CurrentFrequency = 0;
    CurrentStation = "";
    CurrentStationType = "";
    CurrentLanguageType = "";
    CurrentTitle = tr("No Station");
    CurrentText = "";

    CurrentManualGain = 0;
    CurrentManualGainValue = 0.0;
    CurrentVolume = 1.0;

    UpdateGUIData();

    // Clear MOT
    MOTImage->loadFromData(0, 0, Q_NULLPTR);
    emit MOTChanged(*MOTImage);
}

void CRadioController::closeDevice()
{
    qDebug() << "RadioController:" << "Close device";

    // Reset the technical data
    ResetTechnicalData();
}

void CRadioController::openDevice(CVirtualInput* Dev) // Called from CAndroidJNI
{
    Initialise();
}

void CRadioController::onEventLoopStarted()
{
#ifdef Q_OS_ANDROID
    QString dabDevice = "rtl_tcp";
#else
    QString dabDevice = "auto";
#endif

    QString ipAddress = "127.0.0.1";
    uint16_t ipPort = 1234;
    QString rawFile = "";
    QString rawFileFormat = "u8";

    if(commandLineOptions["dabDevice"] != "")
        dabDevice = commandLineOptions["dabDevice"].toString();

    if(commandLineOptions["ipAddress"] != "")
        ipAddress = commandLineOptions["ipAddress"].toString();

    if(commandLineOptions["ipPort"] != "")
        ipPort = commandLineOptions["ipPort"].toInt();

    if(commandLineOptions["rawFile"] != "")
        rawFile = commandLineOptions["rawFile"].toString();

    if(commandLineOptions["rawFileFormat"] != "")
        rawFileFormat = commandLineOptions["rawFileFormat"].toString();

    if(dabDevice == "rawfile")
        SDRDABInterface.SetRAWInput(rawFile);

    Initialise();
}

void CRadioController::Initialise(void)
{
    Status = Initialised;

    UpdateGUIData();
}

void CRadioController::Play(QString Channel, QString Station)
{
    qDebug() << "RadioController:" << "Play channel:"
             << Channel << "station:" << Station;

    if (Status == Scanning) {
        StopScan();
    }

    if(Status != Playing)
        SDRDABInterface.Start();

    SDRDABInterface.TuneToStation(Station);

    Status = Playing;
    UpdateGUIData();

    // Store as last station
    QSettings Settings;
    QStringList StationElement;
    StationElement. append (Station);
    StationElement. append (Channel);
    Settings.setValue("lastchannel", StationElement);
}

void CRadioController::Pause()
{
    Status = Paused;
    UpdateGUIData();
}

void CRadioController::Stop()
{
    Status = Stopped;
    UpdateGUIData();
}

void CRadioController::ClearStations()
{
    //	Clear old channels
    emit StationsCleared();
    mStationList.reset();
    emit StationsChanged(mStationList.getList());

    // Save the channels
    mStationList.saveStations();

    // Clear last station
    QSettings Settings;
    Settings.remove("lastchannel");
}

void CRadioController::SetChannel(QString Channel, bool isScan, bool Force)
{
//    if(CurrentChannel != Channel || Force == true)
//    {
//        if(Device && Device->getID() == CDeviceID::RAWFILE)
//        {
//            CurrentChannel = "File";
//            CurrentEnsemble = "";
//            CurrentFrequency = 0;
//        }
//        else // A real device
//        {
//            CurrentChannel = Channel;
//            CurrentEnsemble = "";

//            // Convert channel into a frequency
//            CurrentFrequency = Channels.getFrequency(Channel);

//            if(CurrentFrequency != 0 && Device)
//            {
//                qDebug() << "RadioController: Tune to channel" <<  Channel << "->" << CurrentFrequency/1e6 << "MHz";
//                Device->setFrequency(CurrentFrequency);
//            }
//        }

//        DecoderRestart(isScan);

//        StationList.clear();

//        UpdateGUIData();
//    }
}

void CRadioController::SetManualChannel(QString Channel)
{
    // Play channel's first station, if available
    foreach(StationElement* station, mStationList.getList())
    {
        if (station->getChannelName() == Channel)
        {
            QString stationName = station->getStationName();
            qDebug() << "RadioController: Play channel" <<  Channel << "and first station" << stationName;
            Play(Channel, stationName);
            return;
        }
    }

    // Otherwise tune to channel and play first found station
    qDebug() << "RadioController: Tune to channel" <<  Channel;

    Status = Playing;
    CurrentTitle = tr("Tuning") + " ... " + Channel;

    // Clear old data
    CurrentStation = "";
    CurrentStationType = "";
    CurrentLanguageType = "";
    CurrentText = "";

    UpdateGUIData();

    // Clear MOT
    MOTImage->loadFromData(0, 0, Q_NULLPTR);
    emit MOTChanged(*MOTImage);

    // Switch channel
    SetChannel(Channel, false, true);
}

void CRadioController::StartScan(void)
{
    qDebug() << "RadioController:" << "Start channel scan";

    // ToDo: Just for testing
    SDRDABInterface.Start();

//    if(Device && Device->getID() == CDeviceID::RAWFILE)
//    {
//        CurrentTitle = tr("RAW File");
//        SetChannel(CChannels::FirstChannel, false); // Just a dummy
//        emit ScanStopped();
//    }
//    else
//    {
//        // Start with lowest frequency
//        QString Channel = CChannels::FirstChannel;
//        SetChannel(Channel, true);

//        isChannelScan = true;
//        mStationCount = 0;
//        CurrentTitle = tr("Scanning") + " ... " + Channel
//                + " (" + QString::number((int)(1 * 100 / NUMBEROFCHANNELS)) + "%)";
//        CurrentText = tr("Found channels") + ": " + QString::number(mStationCount);

//        Status = Scanning;

//        // Clear old data
//        CurrentStation = "";
//        CurrentStationType = "";
//        CurrentLanguageType = "";

//        UpdateGUIData();
//        emit ScanProgress(0);
//    }

    ClearStations();
}

void CRadioController::StopScan(void)
{
    qDebug() << "RadioController:" << "Stop channel scan";

    CurrentTitle = tr("No Station");
    CurrentText = "";

    Status = Stopped;
    UpdateGUIData();
    emit ScanStopped();
}

QList<StationElement *> CRadioController::Stations() const
{
    return mStationList.getList();
}

QVariantMap CRadioController::GUIData(void) const
{
    return mGUIData;
}

void CRadioController::UpdateGUIData()
{
//    mGUIData["DeviceName"] = (Device) ? Device->getName() : "";

    // Init the GUI data map
    mGUIData["Status"] = Status;
    mGUIData["Channel"] = CurrentChannel;
    mGUIData["Ensemble"] = CurrentEnsemble.trimmed();
    mGUIData["Frequency"] = CurrentFrequency;
    mGUIData["Station"] = CurrentStation;
    mGUIData["StationType"] = CurrentStationType;
    mGUIData["Title"] = CurrentTitle.simplified();
    mGUIData["Text"] = CurrentText;
    mGUIData["LanguageType"] = CurrentLanguageType;

    //qDebug() << "RadioController:" <<  "UpdateGUIData";
    emit GUIDataChanged(mGUIData);
}

QImage CRadioController::MOT() const
{
    return *MOTImage;
}



void CRadioController::setErrorMessage(QString Text)
{
    Status = Error;
    emit showErrorMessage(Text);
}

void CRadioController::setInfoMessage(QString Text)
{
    emit showInfoMessage(Text);
}

void CRadioController::SetStation(QString Station, bool Force)
{
    if(CurrentStation != Station || Force == true)
    {
        CurrentStation = Station;

        qDebug() << "RadioController: Tune to station" <<  Station;

        CurrentTitle = tr("Tuning") + " ... " + Station;

        // Clear old data
        CurrentStationType = "";
        CurrentLanguageType = "";
        CurrentText = "";

        UpdateGUIData();

        // Clear MOT
        MOTImage->loadFromData(0, 0, Q_NULLPTR);
        emit MOTChanged(*MOTImage);
    }
}

void CRadioController::NextChannel(bool isWait)
{
    /*if(isWait) // It might be a channel, wait 10 seconds
    {
        ChannelTimer.start(10000);
    }
    else
    {
        QString Channel = Channels.getNextChannel();

        if(!Channel.isEmpty()) {
            SetChannel(Channel, true);

            int index = Channels.getCurrentIndex() + 1;

            CurrentTitle = tr("Scanning") + " ... " + Channel
                    + " (" + QString::number((int)(index * 100 / NUMBEROFCHANNELS)) + "%)";

            UpdateGUIData();
            emit ScanProgress(index);
        } else {
            StopScan();
        }
    }*/
}

void CRadioController::NewStation(QString StationName)
{
    // ToDo Just for testing
    CurrentChannel = "File";

    //	Add new station into list
    if (!mStationList.contains(StationName, CurrentChannel))
    {
        mStationList.append(StationName, CurrentChannel);

        //	Sort stations
        mStationList.sort();

        emit StationsChanged(mStationList.getList());
        emit FoundStation(StationName, CurrentChannel);

        // Save the channels
        mStationList.saveStations();
    }
}

void CRadioController::UpdateSpectrum()
{
//    int Samples = 0;
//    int16_t T_u = DABParams.T_u;

//    //	Delete old data
//    spectrum_data.resize(T_u);

//    qreal tunedFrequency_MHz = 0;
//    qreal sampleFrequency_MHz = 2048000 / 1e6;
//    qreal dip_MHz = sampleFrequency_MHz / T_u;

//    qreal x(0);
//    qreal y(0);
//    qreal y_max(0);

//    // Get FFT buffer
//    DSPCOMPLEX* spectrumBuffer = spectrum_fft_handler->getVector();

//    // Get samples
//    tunedFrequency_MHz = CurrentFrequency / 1e6;
//    if(Device)
//        Samples = Device->getSpectrumSamples(spectrumBuffer, T_u);

//    // Continue only if we got data
//    if (Samples <= 0)
//        return;

//    // Do FFT to get the spectrum
//    spectrum_fft_handler->do_FFT();

//    //	Process samples one by one
//    for (int i = 0; i < T_u; i++) {
//        int half_Tu = T_u / 2;

//        //	Shift FFT samples
//        if (i < half_Tu)
//            y = abs(spectrumBuffer[i + half_Tu]);
//        else
//            y = abs(spectrumBuffer[i - half_Tu]);

//        // Apply a cumulative moving average filter
//        int avg = 4; // Number of y values to average
//        qreal CMA = spectrum_data[i].y();
//        y = (CMA * avg + y) / (avg + 1);

//        //	Find maximum value to scale the plotter
//        if (y > y_max)
//            y_max = y;

//        // Calc x frequency
//        x = (i * dip_MHz) + (tunedFrequency_MHz - (sampleFrequency_MHz / 2));

//        spectrum_data[i]= QPointF(x, y);
//    }

//    //	Set new data
//    emit SpectrumUpdated(round(y_max) + 1,
//                         tunedFrequency_MHz - (sampleFrequency_MHz / 2),
//                         tunedFrequency_MHz + (sampleFrequency_MHz / 2),
//                         spectrum_data);
}
