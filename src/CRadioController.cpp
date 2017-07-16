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
#include "CInputFactory.h"
#include "CRAWFile.h"
#include "CRTL_TCP_Client.h"

#define AUDIOBUFFERSIZE 32768

CRadioController::CRadioController(QVariantMap& commandLineOptions, CDABParams& DABParams, QObject *parent)
#ifdef Q_OS_ANDROID
    : CRadioControllerSource(parent)
#else
    : QObject(parent)
#endif
    , commandLineOptions(commandLineOptions)
    , DABParams(DABParams)
{
    Device = NULL;
    my_ficHandler = NULL;
    my_mscHandler = NULL;
    my_ofdmProcessor = NULL;

    AudioBuffer = new RingBuffer<int16_t>(2 * AUDIOBUFFERSIZE);
    Audio = new CAudio(AudioBuffer);

    MOTImage = new QImage();
    isChannelScan = false;
    isGUIInit = false;
    isAGC = true;
    isHwAGC = true;

    // Init the technical data
    Status = Unknown;
    CurrentChannel = tr("Unknown");
    CurrentFrequency = 0;
    CurrentStation = "";
    CurrentDisplayStation = tr("No Station");
    CurrentStationType = "";
    CurrentLanguageType = "";
    Label = "";
    isSync = false;
    isFICCRC = false;
    isSignal = false;
    SNR = 0;
    FrequencyCorrection = 0;
    BitRate = 0;
    AudioSampleRate = 0;
    isStereo = true;
    isDAB = true;
    FrameErrors = 0;
    RSErrors = 0;
    AACErrors = 0;
    CurrentManualGain = 0;
    CurrentManualGainValue = 0.0;
    GainCount = 0;
    CurrentVolume = 1.0;

    connect(&StationTimer, &QTimer::timeout, this, &CRadioController::StationTimerTimeout);
    connect(&ChannelTimer, &QTimer::timeout, this, &CRadioController::ChannelTimerTimeout);
    connect(&SyncCheckTimer, &QTimer::timeout, this, &CRadioController::SyncCheckTimerTimeout);
}

CRadioController::~CRadioController(void)
{
    // Shutdown the demodulator and decoder in the correct order
    delete my_ficHandler;
    delete my_mscHandler;
    delete my_ofdmProcessor;

    delete Audio;
}

void CRadioController::setDevice(CVirtualInput* Dev)
{
    this->Device = Dev;
}

void CRadioController::onEventLoopStarted()
{
#ifdef Q_OS_ANDROID
    if (Device == NULL)
        return;
#else
    QString dabDevice = "auto";

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

    // Init device
    Device = CInputFactory::GetDevice(*this, dabDevice);

    // Set rtl_tcp settings
    if (Device->getID() == CDeviceID::RTL_TCP) {
        CRTL_TCP_Client* RTL_TCP_Client = (CRTL_TCP_Client*)Device;

        RTL_TCP_Client->setIP(ipAddress);
        RTL_TCP_Client->setPort(ipPort);
    }

    // Set rawfile settings
    if (Device->getID() == CDeviceID::RAWFILE) {
        CRAWFile* RAWFile = (CRAWFile*)Device;

        RAWFile->setFileName(rawFile, rawFileFormat);
    }
#endif

    GainCount = Device->getGainCount();

    Device->setHwAgc(isHwAGC);

    if(!isAGC) // Manual AGC
    {
        Device->setAgc(false);
        Device->setGain(CurrentManualGain);
        qDebug() << "RadioController:" << "AGC off";
    }
    else
    {
        qDebug() << "RadioController:" << "AGC on";
    }

    if(Audio)
        Audio->setVolume(CurrentVolume);

    /**
    *	The actual work is done elsewhere: in ofdmProcessor
    *	and ofdmDecoder for the ofdm related part, ficHandler
    *	for the FIC's and mscHandler for the MSC.
    *	The ficHandler shares information with the mscHandler
    *	but the handlers do not change each others modes.
    */
    my_mscHandler = new mscHandler(this,
        &DABParams,
        AudioBuffer,
        false);

    my_ficHandler = new ficHandler(this);

    /**
    *	The default for the ofdmProcessor depends on
    *	the input device, note that in this setup the
    *	device is selected on start up and cannot be changed.
    */
    my_ofdmProcessor = new ofdmProcessor(Device,
        &DABParams,
        this,
        my_mscHandler,
        my_ficHandler,
        3, 3);

    Status = Initialised;
    emit DeviceReady();
    UpdateGUIData();
}

void CRadioController::Play(QString Channel, QString Station)
{
    qDebug() << "RadioController:" << "Play channel:"
             << Channel << "station:" << Station;

    DeviceRestart();
    SetChannel(Channel, false);
    SetStation(Station);

    Status = Playing;
    UpdateGUIData();

    // Store as last station
    QSettings Settings;
    QStringList StationElement;
    StationElement. append (Station);
    StationElement. append (Channel);
    Settings.setValue("lastchannel", StationElement);

    // Check every 15 s for a correct sync
    SyncCheckTimer.start(15000);
}

void CRadioController::Pause()
{
    if (Device)
        Device->stop();

    if (Audio)
        Audio->reset();

    SyncCheckTimer.stop();

    Status = Paused;
    UpdateGUIData();
}

void CRadioController::Stop()
{
    if (Device)
        Device->stop();

    if (Audio)
        Audio->reset();

    SyncCheckTimer.stop();

    Status = Stopped;
    UpdateGUIData();
}

qreal CRadioController::Volume() const
{
    return CurrentVolume;
}

void CRadioController::setVolume(qreal Volume)
{
    CurrentVolume = Volume;

    if (Audio)
        Audio->setVolume(Volume);

    emit VolumeChanged(CurrentVolume);
}

void CRadioController::SetChannel(QString Channel, bool isScan, bool Force)
{
    if(CurrentChannel != Channel || Force == true)
    {
        if(Device && Device->getID() == CDeviceID::RAWFILE)
        {
            CurrentChannel = "File";
            CurrentFrequency = 0;
        }
        else // A real device
        {
            CurrentChannel = Channel;

            // Convert channel into a frequency
            CurrentFrequency = Channels.getFrequency(Channel);

            if(CurrentFrequency != 0 && Device)
            {
                qDebug() << "RadioController: Tune to channel" <<  Channel << "->" << CurrentFrequency/1e6 << "MHz";
                Device->setFrequency(CurrentFrequency);
            }
        }

        DecoderRestart(isScan);

        StationList.clear();
    }
}

void CRadioController::StartScan(void)
{
    qDebug() << "RadioController:" << "Start channel scan";

    SyncCheckTimer.stop();
    DeviceRestart();

    if(Device && Device->getID() == CDeviceID::RAWFILE)
    {
        CurrentDisplayStation = tr("RAW File");
        SetChannel(CChannels::FirstChannel, false); // Just a dummy
        emit ScanStopped();
    }
    else
    {
        // Start with lowest frequency
        SetChannel(CChannels::FirstChannel, true);

        isChannelScan = true;
        CurrentDisplayStation = tr("Scanning") + " ...";
        Label = CChannels::FirstChannel;
    }

    Status = Scanning;
    UpdateGUIData();
}

void CRadioController::StopScan(void)
{
    qDebug() << "RadioController:" << "Stop channel scan";

    isChannelScan = false;
    CurrentDisplayStation = tr("No Station");
    Label = "";

    Status = Stopped;
    UpdateGUIData();
    emit ScanStopped();
}

QVariantMap CRadioController::GUIData(void) const
{
    return _GUIData;
}

void CRadioController::UpdateGUIData()
{
    if(!isGUIInit)
    {
        isGUIInit = true;

        if(Device)
           _GUIData["DeviceName"] = Device->getName();
    }

    // Init the GUI data map
    _GUIData["Status"] = Status;
    _GUIData["Channel"] = CurrentChannel;
    _GUIData["Frequency"] = CurrentFrequency;
    _GUIData["Station"] = CurrentDisplayStation.simplified();

    QDateTime LocalTime = CurrentDateTime.toLocalTime();
    _GUIData["DateTime"] = QLocale().toString(LocalTime, QLocale::ShortFormat);

    _GUIData["isSync"] = isSync;
    _GUIData["isFICCRC"] = isFICCRC;
    _GUIData["isSignal"] = isSignal;
    _GUIData["SNR"] = SNR;
    _GUIData["FrequencyCorrection"] = FrequencyCorrection;
    _GUIData["BitRate"] = BitRate;
    _GUIData["isStereo"] = isStereo;
    _GUIData["FrameErrors"] = FrameErrors;
    _GUIData["RSErrors"] = RSErrors;
    _GUIData["AACErrors"] = AACErrors;
    _GUIData["Label"] = Label;
    _GUIData["StationType"] = CurrentStationType;
    _GUIData["LanguageType"] = CurrentLanguageType;
    _GUIData["isDAB"] = isDAB;
    _GUIData["GainCount"] = GainCount;

    //qDebug() << "RadioController:" <<  "UpdateGUIData";
    emit GUIDataChanged(_GUIData);
}

QImage CRadioController::MOT() const
{
    return *MOTImage;
}

int32_t CRadioController::GetSpectrumSamples(DSPCOMPLEX *Buffer, int32_t Size)
{
    int Samples = 0;

    if(Device)
        Samples = Device->getSpectrumSamples(Buffer, Size);

    return Samples;
}

int CRadioController::GetCurrentFrequency(void)
{
    return CurrentFrequency;
}

bool CRadioController::HwAGC() const
{
    return this->isHwAGC;
}

void CRadioController::setHwAGC(bool isHwAGC)
{
    this->isHwAGC = isHwAGC;

    if (Device)
    {
        Device->setHwAgc(isHwAGC);
        qDebug() << "RadioController:" << (isHwAGC ? "HwAGC on" : "HwAGC off");
    }
    emit HwAGCChanged(isHwAGC);
}

bool CRadioController::AGC() const
{
    return this->isAGC;
}

void CRadioController::setAGC(bool isAGC)
{
    this->isAGC = isAGC;

    if (Device)
    {
        Device->setAgc(isAGC);

        if (!isAGC)
        {
            Device->setGain(CurrentManualGain);
            qDebug() << "RadioController:" << "AGC off";
        }
        else
        {
            qDebug() << "RadioController:" <<  "AGC on";
        }
    }
    emit AGCChanged(isAGC);
}

float CRadioController::GainValue() const
{
    return CurrentManualGainValue;
}

int CRadioController::Gain() const
{
    return CurrentManualGain;
}

void CRadioController::setGain(int Gain)
{
    CurrentManualGain = Gain;
    emit GainChanged(CurrentManualGain);

    if (Device)
        CurrentManualGainValue = Device->setGain(Gain);
    else
        CurrentManualGainValue = -1.0;

    emit GainValueChanged(CurrentManualGainValue);
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

void CRadioController::setAndroidInstallDialog(QString Title, QString Text)
{
    Status = Error;
    emit showAndroidInstallDialog(Title, Text);
}

/********************
 * Private methods  *
 ********************/

void CRadioController::DeviceRestart()
{
    bool isPlay = false;

    if(Device)
        isPlay = Device->restart();

    if(!isPlay)
    {
        qDebug() << "RadioController:" << "Radio device is not ready or does not exits.";
        emit showErrorMessage(tr("Radio device is not ready or does not exits."));
        return;
    }
}

void CRadioController::DecoderRestart(bool isScan)
{
    //	The ofdm processor is "conditioned" to send one signal
    //	per "scanning tour". This signal is either "false"
    //	if we are pretty certain that the channel does not contain
    //	a signal, or "true" if there is a fair chance that the
    //	channel contains useful data
    if(my_ofdmProcessor && my_mscHandler && my_ficHandler)
    {
        my_ofdmProcessor->set_scanMode(isScan);

        my_mscHandler->stopProcessing();
        my_ficHandler->clearEnsemble();
        my_ofdmProcessor->coarseCorrectorOn();
        my_ofdmProcessor->reset();
    }
}

void CRadioController::SetStation(QString Station, bool Force)
{
    if(CurrentStation != Station || Force == true)
    {
        CurrentStation = Station;

        qDebug() << "RadioController: Tune to station" <<  Station;

        CurrentDisplayStation = tr("Tuning") + " ... " + Station;

        // Wait if we found the station inside the signal
        StationTimer.start(1000);

        // Clear old data
        CurrentStationType = "";
        CurrentLanguageType = "";
        Label = "";

        UpdateGUIData();

        // Clear MOT
        QImage MOT(320, 240, QImage::Format_Alpha8);
        MOT.fill(Qt::transparent);
        emit MOTChanged(MOT);
    }
}

void CRadioController::NextChannel(bool isWait)
{
    if(isWait) // It might be a channel, wait 10 seconds
    {
        ChannelTimer.start(10000);
    }
    else
    {
        QString Channel = Channels.getNextChannel();
        Label = Channel;

        if(!Channel.isEmpty())
            SetChannel(Channel, true);
        else
            StopScan();

        emit ScanProgress(Channels.getCurrentIndex() + 1);
    }
}

/********************
 * Controller slots *
 ********************/

void CRadioController::StationTimerTimeout()
{
    if(StationList.contains(CurrentStation))
    {
        audiodata AudioData;
        memset(&AudioData, 0, sizeof(audiodata));

        my_ficHandler->dataforAudioService(CurrentStation, &AudioData);

        if(AudioData.defined == true)
        {
            // We found the station inside the signal, lets stop the timer
            StationTimer.stop();

            // Set station
            my_mscHandler->set_audioChannel(&AudioData);

            CurrentDisplayStation = CurrentStation;

            CurrentStationType = CDABConstants::getProgramTypeName(AudioData.programType);
            CurrentLanguageType = CDABConstants::getLanguageName(AudioData.language);
            BitRate = AudioData.bitRate;

            if (AudioData.ASCTy == 077)
                isDAB = false;
            else
                isDAB = true;

            Status = Playing;
            UpdateGUIData();
        }
    }
}

void CRadioController::ChannelTimerTimeout(void)
{
    ChannelTimer.stop();

    if(isChannelScan)
        NextChannel(false);
}

void CRadioController::SyncCheckTimerTimeout(void)
{
    // A better approach is to use the MER since it is not implemented we use the this one
    if(!isSync ||
       (isSync && !isFICCRC) ||
       (isSync && FrameErrors >= 10))
    {
        qDebug() << "RadioController: Restart syncing. isSync:" << isSync << ", isFICCRC:" << isFICCRC << ", FrameErrors:" << FrameErrors;
        emit showInfoMessage(tr("Lost signal or bad signal quality, trying to find it again."));

        SetChannel(CurrentChannel, false, true);
        SetStation(CurrentStation, true);
    }
}


/*****************
 * Backend slots *
 *****************/

void CRadioController::addtoEnsemble(quint32 SId, const QString &Station)
{
    QString StationId = QString::number(SId, 16).toUpper();
    qDebug() << "RadioController: Found station" <<  Station
             << "(" << qPrintable(StationId) << ")";

    StationList.append(Station);

    emit FoundStation(StationId, Station, CurrentChannel);
}

void CRadioController::nameofEnsemble(int id, const QString &v)
{
    // ToDo: Maybe display of the ensemble name
    (void)id;
    (void)v;
}

void CRadioController::changeinConfiguration()
{
    // Unknown use case
}

void CRadioController::displayDateTime(int *DateTime)
{
    QDate Date;
    QTime Time;

    int Year = DateTime[0];
    int Month = DateTime[1];
    int Day = DateTime[2];
    int Hour = DateTime[3];
    int Minute = DateTime[4];
    int Seconds	= DateTime [5];
    int HourOffset = DateTime[6];
    int MinuteOffset = DateTime[7];

    Time.setHMS(Hour, Minute, Seconds);
    CurrentDateTime.setTime(Time);

    Date.setDate(Year, Month, Day);
    CurrentDateTime.setDate(Date);

    int OffsetFromUtc = ((HourOffset * 3600) + (MinuteOffset * 60));
    CurrentDateTime.setOffsetFromUtc(OffsetFromUtc);
    CurrentDateTime.setTimeSpec(Qt::OffsetFromUTC);

    UpdateGUIData();

    return;
}

void CRadioController::show_ficSuccess(bool isFICCRC)
{
    if (this->isFICCRC == isFICCRC)
        return;
    this->isFICCRC = isFICCRC;
    UpdateGUIData();
}

void CRadioController::show_snr(int SNR)
{
    if (this->SNR == SNR)
        return;
    this->SNR = SNR;
    UpdateGUIData();
}

void CRadioController::set_fineCorrectorDisplay(int FineFrequencyCorr)
{
    int CoarseFrequencyCorr = (FrequencyCorrection / 1000);
    SetFrequencyCorrection((CoarseFrequencyCorr * 1000) + FineFrequencyCorr);
}

void CRadioController::set_coarseCorrectorDisplay(int CoarseFreuqencyCorr)
{
    int OldCoareFrequencyCorrr = (FrequencyCorrection / 1000);
    int FineFrequencyCorr = FrequencyCorrection - (OldCoareFrequencyCorrr * 1000);
    SetFrequencyCorrection((CoarseFreuqencyCorr * 1000) + FineFrequencyCorr);
}

void CRadioController::SetFrequencyCorrection(int FrequencyCorrection)
{
    if (this->FrequencyCorrection == FrequencyCorrection)
        return;
    this->FrequencyCorrection = FrequencyCorrection;
    UpdateGUIData();
}

void CRadioController::setSynced(char isSync)
{
    bool sync = (isSync == SYNCED) ? true : false;
    if (this->isSync == sync)
        return;
    this->isSync = sync;
    UpdateGUIData();
}

void CRadioController::setSignalPresent(bool isSignal)
{
    if (this->isSignal != isSignal) {
        this->isSignal = isSignal;
        UpdateGUIData();
    }

    if(isChannelScan)
        NextChannel(isSignal);
}

void CRadioController::newAudio(int SampleRate)
{
    if(AudioSampleRate != SampleRate)
    {
        qDebug() << "RadioController: Audio sample rate" <<  SampleRate << "kHz";
        AudioSampleRate = SampleRate;

        Audio->setRate(SampleRate);
    }
}

void CRadioController::setStereo(bool isStereo)
{
    if (this->isStereo == isStereo)
        return;
    this->isStereo = isStereo;
    UpdateGUIData();
}

void CRadioController::show_frameErrors(int FrameErrors)
{
    if (this->FrameErrors == FrameErrors)
        return;
    this->FrameErrors = FrameErrors;
    UpdateGUIData();
}

void CRadioController::show_rsErrors(int RSErrors)
{
    if (this->RSErrors == RSErrors)
        return;
    this->RSErrors = RSErrors;
    UpdateGUIData();
}

void CRadioController::show_aacErrors(int AACErrors)
{
    if (this->AACErrors == AACErrors)
        return;
    this->AACErrors = AACErrors;
    UpdateGUIData();
}

void CRadioController::showLabel(QString Label)
{
    if (this->Label == Label)
        return;
    this->Label = Label;
    UpdateGUIData();
}

void CRadioController::showMOT(QByteArray Data, int Subtype, QString s)
{
    (void)s; // Not used, can be removed

    MOTImage->loadFromData(Data, Subtype == 0 ? "GIF" : Subtype == 1 ? "JPEG" : Subtype == 2 ? "BMP" : "PNG");

    emit MOTChanged(*MOTImage);
}
