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
#include "input/CInputFactory.h"
#include "input/CRAWFile.h"
#include "input/CRTL_TCP_Client.h"

#define AUDIOBUFFERSIZE 32768

CRadioController *CRadioController::m_RadioController = nullptr;

CRadioController::CRadioController(QVariantMap& commandLineOptions, QObject *parent)
#ifdef Q_OS_ANDROID
    : CRadioControllerSource(parent)
#else
    : QObject(parent)
#endif
    , commandLineOptions(commandLineOptions)
{
    if(m_RadioController != nullptr)
        throw std::string("Only one CRadioController instance is allowed");
    else
         m_RadioController = this;

    MOTImage = new QImage();

//    spectrum_fft_handler = new common_fft(DABParams.T_u);

    // Init the technical data
    resetTechnicalData();

    // Read channels from settings
    mStationList.loadStations();
    mStationList.sort();
    emit StationsChanged(mStationList.getList());

    // Init SDRDAB interface
    connect(&SDRDABInterface, &CSdrDabInterface::newStationFound, this, &CRadioController::newStation);
    connect(&SDRDABInterface, &CSdrDabInterface::stationInfoUpdate, this, &CRadioController::ficUpdate);
    connect(&SDRDABInterface, &CSdrDabInterface::snrChanged, this, &CRadioController::snrUpdate);
    connect(&SDRDABInterface, &CSdrDabInterface::fcDriftChanged, this, &CRadioController::fcDriftUpdate);
    connect(&SDRDABInterface, &CSdrDabInterface::syncStateChanged, this, &CRadioController::syncStateUpdate);
    connect(&SDRDABInterface, &CSdrDabInterface::rsErrorsChanged, this, &CRadioController::rsErrorsUpdate);
    connect(&SDRDABInterface, &CSdrDabInterface::superFrameErrorsChanged, this, &CRadioController::superFrameErrorsUpdate);
    connect(&SDRDABInterface, &CSdrDabInterface::aacCrcChanged, this, &CRadioController::aacCrcUpdate);
    connect(&SDRDABInterface, &CSdrDabInterface::ficCrcChanged, this, &CRadioController::ficCrcUpdate);

}

CRadioController::~CRadioController(void)
{
}

void CRadioController::resetTechnicalData(void)
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

    mIsDAB = true;
    mIsStereo = false;

    updateGUIData();

    // Clear MOT
    MOTImage->loadFromData(0, 0, Q_NULLPTR);
    emit MOTChanged(*MOTImage);
}

void CRadioController::closeDevice()
{
    qDebug() << "RadioController:" << "Close device";

    // Reset the technical data
    resetTechnicalData();
}

void CRadioController::openDevice(CVirtualInput* Dev) // Called from CAndroidJNI
{
    initialise();
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

    /*if(dabDevice == "rawfile")
        SDRDABInterface.setRAWInput(rawFile);*/

    // Init device
    Device = CInputFactory::GetDevice(*this, dabDevice);

    // Set rtl_tcp settings
    if (Device->getID() == CDeviceID::RTL_TCP) {
        std::shared_ptr<CRTL_TCP_Client> RTL_TCP_Client = std::static_pointer_cast<CRTL_TCP_Client>(Device);

        RTL_TCP_Client->setIP(ipAddress);
        RTL_TCP_Client->setPort(ipPort);
    }

    // Set rawfile settings
    if (Device->getID() == CDeviceID::RAWFILE) {
        std::shared_ptr<CRAWFile> RAWFile = std::static_pointer_cast<CRAWFile>(Device);

        RAWFile->setFileName(rawFile, rawFileFormat);
    }

    initialise();
}

void CRadioController::initialise(void)
{
    Status = Initialised;

    updateGUIData();
}

void CRadioController::play(QString Channel, QString Station, int SubChannelID)
{
    qDebug() << "RadioController:" << "Play channel:"
             << Channel << "station:" << Station;

    if (Status == Scanning)
    {
        stopScan();
    }

    CurrentStation = Station;
    CurrentTitle = tr("Tuning") + " ... " + CurrentStation;

    // Clear old data
    CurrentStationType = "";
    CurrentLanguageType = "";
    CurrentText = "";

    updateGUIData();

    // Clear MOT
    MOTImage->loadFromData(0, 0, Q_NULLPTR);
    emit MOTChanged(*MOTImage);

    // Implement the different states of playing
    if(Status != Playing)
    {
        setChannel(Channel, false);
        SDRDABInterface.start(true, SubChannelID);
        Status = Tuning;
    }
    else
    {
        if(CurrentChannel == Channel)
        {
            SDRDABInterface.tuneToStation(SubChannelID);
            Status = Playing;
            CurrentTitle = CurrentStation;
        }
        else
        {
            SDRDABInterface.stop();
            SDRDABInterface.start(true, SubChannelID);
            Status = Tuning;
        }
    }

    updateGUIData();

    // Store as last station
    QSettings Settings;
    QStringList StationElement;
    StationElement.append (Station);
    StationElement.append (Channel);
    Settings.setValue("lastchannel", StationElement);
}

void CRadioController::pause()
{
    Status = Paused;
    updateGUIData();
}

void CRadioController::stop()
{
    Status = Stopped;
    updateGUIData();
}

void CRadioController::clearStations()
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

void CRadioController::setChannel(QString Channel, bool isScan, bool Force)
{
    if(CurrentChannel != Channel || Force == true)
    {
        if(Device->getID() == CDeviceID::RAWFILE)
        {
            CurrentChannel = "File";
            CurrentEnsemble = "";
            CurrentFrequency = 0;
        }
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

        updateGUIData();
    }
}

void CRadioController::setManualChannel(QString Channel)
{
    // Play channel's first station, if available
    foreach(StationElement* station, mStationList.getList())
    {
        if (station->getChannelName() == Channel)
        {
            QString stationName = station->getStationName();
            qDebug() << "RadioController: Play channel" <<  Channel << "and first station" << stationName;
            play(Channel, stationName);
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

    updateGUIData();

    // Clear MOT
    MOTImage->loadFromData(0, 0, Q_NULLPTR);
    emit MOTChanged(*MOTImage);

    // Switch channel
    setChannel(Channel, false, true);
}

void CRadioController::startScan(void)
{
    qDebug() << "RadioController:" << "Start channel scan";

    // ToDo: Just for testing
    SDRDABInterface.start(false);

    if(Device->getID() == CDeviceID::RAWFILE)
    {
        CurrentTitle = tr("RAW File");
        setChannel(CChannels::FirstChannel, false); // Just a dummy
        emit ScanStopped();
    }
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

    clearStations();
}

void CRadioController::stopScan(void)
{
    qDebug() << "RadioController:" << "Stop channel scan";

    CurrentTitle = tr("No Station");
    CurrentText = "";

    Status = Stopped;
    updateGUIData();
    emit ScanStopped();
}

QList<StationElement *> CRadioController::stations() const
{
    return mStationList.getList();
}

QVariantMap CRadioController::guiData(void) const
{
    return mGUIData;
}

void CRadioController::updateGUIData()
{
    mGUIData["DeviceName"] = (Device) ? Device->getName() : "";

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

QImage CRadioController::mot() const
{
    return *MOTImage;
}

bool CRadioController::isDAB() const
{
    return mIsDAB;
}

int CRadioController::BitRate() const
{
    return mBitRate;
}

bool CRadioController::isStereo() const
{
    return mIsStereo;
}

int CRadioController::SNR() const
{
    return mSNR;
}

int CRadioController::FrequencyCorrection() const
{
    return mFrequencyCorrection;
}

bool CRadioController::isSync() const
{
    return mIsSync;
}

int CRadioController::FrameErrors() const
{
    return mFrameErrors;
}

int CRadioController::RSErrors() const
{
    return mRSErrors;
}

int CRadioController::AACErrors() const
{
    return mAACErrors;
}

bool CRadioController::isFICCRC() const
{
    return mIsFICCRC;
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

void CRadioController::nextChannel(bool isWait)
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

void CRadioController::newStation(QString StationName, uint8_t SubChannelId)
{
    //	Add new station into list
    if (!mStationList.contains(StationName, CurrentChannel))
    {
        mStationList.append(StationName, CurrentChannel, SubChannelId);

        //	Sort stations
        mStationList.sort();

        emit StationsChanged(mStationList.getList());
        emit FoundStation(StationName, CurrentChannel);

        // Save the channels
        mStationList.saveStations();
    }

    // Update station display if needed
    if(Status == Tuning && CurrentStation == StationName)
    {
        CurrentTitle = CurrentStation;
        Status = Playing;

        updateGUIData();
    }
}

void CRadioController::ficUpdate(bool isDABPlus, size_t bitrate, QString programme_type)
{
    if(mIsDAB != !isDABPlus)
    {
        mIsDAB = !isDABPlus;
        emit isDABChanged(mIsDAB);
    }

    if(mBitRate != bitrate)
    {
        mBitRate = bitrate;
        emit BitRateChanged(mBitRate);
    }

    CurrentStationType = programme_type;
}

void CRadioController::snrUpdate(int SNR)
{
    mSNR = SNR;
    emit SNRChanged(mSNR);
}

void CRadioController::fcDriftUpdate(int estimated_fc_drift)
{
    mFrequencyCorrection = estimated_fc_drift;
    emit FrequencyCorrectionChanged(mFrequencyCorrection);
}

void CRadioController::syncStateUpdate(bool isSync)
{
    mIsSync = isSync;
    emit isSyncChanged(mIsSync);
}

void CRadioController::rsErrorsUpdate(int rs_errors)
{
    mRSErrors = rs_errors;
    emit RSErrorsChanged(mRSErrors);
}

void CRadioController::superFrameErrorsUpdate(int super_frame_error)
{
    mFrameErrors = super_frame_error;
    emit FrameErrorsChanged(mFrameErrors);
}

void CRadioController::aacCrcUpdate(int aac_crc_errors)
{
    mAACErrors = aac_crc_errors;
    emit AACErrorsChanged(mAACErrors);
}

void CRadioController::ficCrcUpdate(int fic_crc_errors)
{
    if(fic_crc_errors)
    {
        mIsFICCRC = false;
        emit isFICCRCChanged(false);
    }
    else
    {
        mIsFICCRC = true;
        emit isFICCRCChanged(true);
    }
}

void CRadioController::updateSpectrum()
{
    // Get samples
    SDRDABInterface.getSpectrumData(SpectrumBuffer);
    int fftSize = SpectrumBuffer.size() / 2;

    // Continue only if we got data
    if(fftSize <= 0)
        return;

    // Resize if required
    Spectrum.resize(fftSize);

    qreal tunedFrequency_MHz = CurrentFrequency / 1e6;
    qreal sampleFrequency_MHz = 2048000 / 1e6;
    qreal dip_MHz = sampleFrequency_MHz / fftSize;

    qreal x(0);
    qreal y(0);
    qreal y_max(0);

    //	Process samples one by one
    for (int i = 0; i < fftSize; i++) {
        int half_Tu = fftSize / 2;

        //	Shift FFT samples
        if (i < half_Tu)
            y = sqrt((SpectrumBuffer[(i+half_Tu)*2] * SpectrumBuffer[(i+half_Tu)*2]) +
                    (SpectrumBuffer[(i+half_Tu)*2 + 1] * SpectrumBuffer[(i+half_Tu)*2 + 1]));
        else
            y = sqrt((SpectrumBuffer[(i-half_Tu)*2] * SpectrumBuffer[(i-half_Tu)*2]) +
                    (SpectrumBuffer[(i-half_Tu)*2 + 1] * SpectrumBuffer[(i-half_Tu)*2 + 1]));

        // Apply a cumulative moving average filter
//        int avg = 4; // Number of y values to average
//        qreal CMA = Spectrum[i].y();
//        y = (CMA * avg + y) / (avg + 1);

        //	Find maximum value to scale the plotter
        if (y > y_max)
            y_max = y;

        // Calc x frequency
        x = (i * dip_MHz) + (tunedFrequency_MHz - (sampleFrequency_MHz / 2));

        Spectrum[i]= QPointF(x, y);
    }

    //	Set new data
    emit SpectrumUpdated(round(y_max) + 1,
                         tunedFrequency_MHz - (sampleFrequency_MHz / 2),
                         tunedFrequency_MHz + (sampleFrequency_MHz / 2),
                         Spectrum);
}

std::shared_ptr<CVirtualInput> CRadioController::getDevice()
{
    if(m_RadioController)
        return m_RadioController->Device;
    else
        return nullptr;
}

void CRadioController::setStereo(bool isStereo)
{
    if(m_RadioController)
    {
        m_RadioController->mIsStereo = isStereo;
        emit m_RadioController->isStereoChanged(isStereo);
    }
}
