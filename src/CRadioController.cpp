/*
 *    Copyright (C) 2018
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

#include <QDebug>
#include <QSettings>

#include "CRadioController.h"
#ifdef HAVE_SOAPYSDR
#include "CSoapySdr.h"
#endif /* HAVE_SOAPYSDR */
#include "CInputFactory.h"
#include "CRAWFile.h"
#include "CRTL_TCP_Client.h"
#include "CSplashScreen.h"

#define AUDIOBUFFERSIZE 32768

CRadioController::CRadioController(QVariantMap& commandLineOptions, DABParams& params, QObject *parent)
#ifdef Q_OS_ANDROID
    : CRadioControllerSource(parent)
#else
    : QObject(parent)
#endif
    , commandLineOptions(commandLineOptions)
    , dabparams(params)
    , audioBuffer(2 * AUDIOBUFFERSIZE)
    , audio(audioBuffer)
{
    Device = NULL;

    PlotType = PlotTypeEn::Spectrum;
    spectrum_fft_handler = new common_fft(dabparams.T_u);

    // Init the technical data
    ResetTechnicalData();

    // Read channels from settings
    mStationList.loadStations();
    mStationList.sort();
    emit StationsChanged(mStationList.getList());

    // Init timers
    connect(&StationTimer, &QTimer::timeout, this, &CRadioController::StationTimerTimeout);
    connect(&ChannelTimer, &QTimer::timeout, this, &CRadioController::ChannelTimerTimeout);
    connect(&SyncCheckTimer, &QTimer::timeout, this, &CRadioController::SyncCheckTimerTimeout);

    connect(this, &CRadioController::SwitchToNextChannel,
            this, &CRadioController::NextChannel);

    connect(this, &CRadioController::EnsembleAdded,
            this, &CRadioController::addtoEnsemble);

    connect(this, &CRadioController::EnsembleNameUpdated,
            this, &CRadioController::nameofEnsemble);

    qRegisterMetaType<dab_date_time_t>("dab_date_time_t");
    connect(this, &CRadioController::DateTimeUpdated,
            this, &CRadioController::displayDateTime);

    isAutoPlay = false;
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

    mErrorMsg = "";

    mIsSync = false;
    mIsFICCRC = false;
    mIsSignal = false;
    mSNR = 0;
    mFrequencyCorrection = 0;
    mBitRate = 0;
    mAudioSampleRate = 0;
    mIsStereo = true;
    mIsDAB = true;
    mFrameErrors = 0;
    mRSErrors = 0;
    mAACErrors = 0;
    mGainCount = 0;
    mStationCount = 0;
    CurrentManualGain = 0;
    CurrentManualGainValue = std::numeric_limits<float>::lowest();
    CurrentVolume = 1.0;

    startPlayback = false;
    isChannelScan = false;
    isAGC = true;
    isHwAGC = true;

    UpdateGUIData();

    // Clear MOT
    motImage.loadFromData(0, 0, Q_NULLPTR);
    emit MOTChanged(motImage);
}

void CRadioController::closeDevice()
{
    qDebug() << "RadioController:" << "Close device";

    my_rx.reset();

    delete Device;
    Device = NULL;

    audio.reset();

    SyncCheckTimer.stop();

    // Reset the technical data
    ResetTechnicalData();

    emit isHwAGCSupportedChanged(isHwAGCSupported());
    emit DeviceClosed();
}

void CRadioController::openDevice(CVirtualInput* Dev)
{
    if (Device) {
        closeDevice();
    }
    this->Device = Dev;
    Initialise();
}

void CRadioController::onEventLoopStarted()
{
#ifdef Q_OS_ANDROID
    QString dabDevice = "rtl_tcp";
#else
    QString dabDevice = "auto";
#endif

#ifdef HAVE_SOAPYSDR
    QString sdrDriverArgs;
    QString sdrAntenna;
    QString sdrClockSource;
#endif /* HAVE_SOAPYSDR */
    QString ipAddress = "127.0.0.1";
    uint16_t ipPort = 1234;
    QString rawFile = "";
    QString rawFileFormat = "u8";

    if(commandLineOptions["dabDevice"] != "")
        dabDevice = commandLineOptions["dabDevice"].toString();

#ifdef HAVE_SOAPYSDR
    if(commandLineOptions["sdr-driver-args"] != "")
        sdrDriverArgs = commandLineOptions["sdr-driver-args"].toString();

    if(commandLineOptions["sdr-antenna"] != "")
        sdrAntenna = commandLineOptions["sdr-antenna"].toString();

    if(commandLineOptions["sdr-clock-source"] != "")
        sdrClockSource = commandLineOptions["sdr-clock-source"].toString();
#endif /* HAVE_SOAPYSDR */

    if(commandLineOptions["ipAddress"] != "")
        ipAddress = commandLineOptions["ipAddress"].toString();

    if(commandLineOptions["ipPort"] != "")
        ipPort = commandLineOptions["ipPort"].toInt();

    if(commandLineOptions["rawFile"] != "")
        rawFile = commandLineOptions["rawFile"].toString();

    if(commandLineOptions["rawFileFormat"] != "")
        rawFileFormat = commandLineOptions["rawFileFormat"].toString();

    // Init device
    CSplashScreen::ShowMessage(tr("Init radio receiver"));
    Device = CInputFactory::GetDevice(*this, dabDevice.toStdString());

    // Set rtl_tcp settings
    if (Device->getID() == CDeviceID::RTL_TCP) {
        CRTL_TCP_Client* RTL_TCP_Client = (CRTL_TCP_Client*)Device;

        RTL_TCP_Client->setIP(ipAddress.toStdString());
        RTL_TCP_Client->setPort(ipPort);
    }

    // Set rawfile settings
    if (Device->getID() == CDeviceID::RAWFILE) {
        CRAWFile* RAWFile = (CRAWFile*)Device;

        RAWFile->setFileName(rawFile.toStdString(), rawFileFormat.toStdString());
    }

#ifdef HAVE_SOAPYSDR
    if (Device->getID() == CDeviceID::SOAPYSDR) {
        CSoapySdr *sdr = (CSoapySdr*)Device;

        if (!sdrDriverArgs.isEmpty()) {
            sdr->setDriverArgs(sdrDriverArgs.toStdString());
        }

        if (!sdrAntenna.isEmpty()) {
            sdr->setAntenna(sdrAntenna.toStdString());
        }

        if (!sdrClockSource.isEmpty()) {
            sdr->setClockSource(sdrClockSource.toStdString());
        }
    }
#endif /* HAVE_SOAPYSDR */

    Initialise();

    CSplashScreen::Close();
}

void CRadioController::Initialise(void)
{
    mGainCount = Device->getGainCount();
    emit GainCountChanged(mGainCount);

    Device->setHwAgc(isHwAGC);

    if (!isAGC) { // Manual AGC
        Device->setAgc(false);
        Device->setGain(CurrentManualGain);
        qDebug() << "RadioController:" << "AGC off";
    }
    else {
        Device->setAgc(true);
        qDebug() << "RadioController:" << "AGC on";
    }

    audio.setVolume(CurrentVolume);

    my_rx = std::make_unique<RadioReceiver>(*this, *Device);

    Status = Initialised;
    emit DeviceReady();
    emit isHwAGCSupportedChanged(isHwAGCSupported());
    UpdateGUIData();

    if(isAutoPlay) {
        Play(autoChannel, autoStation);
    }
}

void CRadioController::Play(QString Channel, QString Station)
{
    qDebug() << "RadioController:" << "Play channel:"
             << Channel << "station:" << Station;

    if (Status == Scanning) {
        StopScan();
    }

    DeviceRestart();
    startPlayback = false;

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

    audio.reset();

    SyncCheckTimer.stop();

    startPlayback = false;
    Status = Paused;
    UpdateGUIData();
}

void CRadioController::Stop()
{
    if (Device)
        Device->stop();

    audio.reset();

    SyncCheckTimer.stop();

    startPlayback = false;
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

qreal CRadioController::Volume() const
{
    return CurrentVolume;
}

void CRadioController::setVolume(qreal Volume)
{
    CurrentVolume = Volume;

    audio.setVolume(Volume);

    emit VolumeChanged(CurrentVolume);
}

void CRadioController::SetChannel(QString Channel, bool isScan, bool Force)
{
    if(CurrentChannel != Channel || Force == true)
    {
        if(Device && Device->getID() == CDeviceID::RAWFILE)
        {
            CurrentChannel = "File";
            CurrentEnsemble = "";
            CurrentFrequency = 0;
        }
        else // A real device
        {
            CurrentChannel = Channel;
            CurrentEnsemble = "";

            // Convert channel into a frequency
            CurrentFrequency = channels.getFrequency(Channel.toStdString());

            if(CurrentFrequency != 0 && Device)
            {
                qDebug() << "RadioController: Tune to channel" <<  Channel << "->" << CurrentFrequency/1e6 << "MHz";
                Device->setFrequency(CurrentFrequency);
            }
        }

        DecoderRestart(isScan);

        StationList.clear();

        UpdateGUIData();
    }
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

    SyncCheckTimer.stop();
    DeviceRestart();

    startPlayback = true;
    Status = Playing;
    CurrentTitle = tr("Tuning") + " ... " + Channel;

    // Clear old data
    CurrentStation = "";
    CurrentStationType = "";
    CurrentLanguageType = "";
    CurrentText = "";

    UpdateGUIData();

    // Clear MOT
    motImage.loadFromData(0, 0, Q_NULLPTR);
    emit MOTChanged(motImage);

    // Switch channel
    SetChannel(Channel, false, true);
}

void CRadioController::StartScan(void)
{
    qDebug() << "RadioController:" << "Start channel scan";

    SyncCheckTimer.stop();
    DeviceRestart();
    startPlayback = false;

    if(Device && Device->getID() == CDeviceID::RAWFILE)
    {
        CurrentTitle = tr("RAW File");
        const auto FirstChannel = QString::fromStdString(Channels::firstChannel);
        SetChannel(FirstChannel, false); // Just a dummy
        emit ScanStopped();
    }
    else
    {
        // Start with lowest frequency
        QString Channel = QString::fromStdString(Channels::firstChannel);
        SetChannel(Channel, true);

        isChannelScan = true;
        mStationCount = 0;
        CurrentTitle = tr("Scanning") + " ... " + Channel
                + " (" + QString::number((int)(1 * 100 / NUMBEROFCHANNELS)) + "%)";
        CurrentText = tr("Found channels") + ": " + QString::number(mStationCount);

        Status = Scanning;

        // Clear old data
        CurrentStation = "";
        CurrentStationType = "";
        CurrentLanguageType = "";

        UpdateGUIData();
        emit ScanProgress(0);
    }

    ClearStations();
}

void CRadioController::StopScan(void)
{
    qDebug() << "RadioController:" << "Stop channel scan";

    isChannelScan = false;
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
    mGUIData["DeviceName"] = Device ?
        QString::fromStdString(Device->getName()) : "";

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
    return motImage;
}

QString CRadioController::ErrorMsg() const
{
    return mErrorMsg;
}

QString CRadioController::DateTime() const
{
    QDateTime LocalTime = mCurrentDateTime.toLocalTime();
    return QLocale().toString(LocalTime, QLocale::ShortFormat);
}

bool CRadioController::isSync() const
{
    return mIsSync;
}

bool CRadioController::isFICCRC() const
{
    return mIsFICCRC;
}

bool CRadioController::isSignal() const
{
    return mIsSignal;
}

bool CRadioController::isStereo() const
{
    return mIsStereo;
}

bool CRadioController::isDAB() const
{
    return mIsDAB;
}

int CRadioController::SNR() const
{
    return mSNR;
}

int CRadioController::FrequencyCorrection() const
{
    return mFrequencyCorrection;
}

int CRadioController::BitRate() const
{
    return mBitRate;
}

int CRadioController::AudioSampleRate() const
{
    return mAudioSampleRate;
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

int CRadioController::GainCount() const
{
    return mGainCount;
}

bool CRadioController::isHwAGCSupported() const
{
    return (this->Device) ? this->Device->isHwAgcSupported() : false;
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

    if (Device)
    {
        CurrentManualGainValue = Device->setGain(Gain);
        int32_t mGainCount_tmp = Device->getGainCount();

        if(mGainCount != mGainCount_tmp)
        {
            mGainCount = mGainCount_tmp;
            emit GainCountChanged(mGainCount);
            UpdateGUIData();
        }
    }
    else
    {
        CurrentManualGainValue = std::numeric_limits<float>::lowest();
    }

    emit GainValueChanged(CurrentManualGainValue);
    emit GainChanged(CurrentManualGain);
}

void CRadioController::setErrorMessage(QString Text)
{
    Status = Error;
    mGUIData["Status"] = Status;
    mErrorMsg = Text;
    emit showErrorMessage(Text);
}

void CRadioController::setErrorMessage(const std::string& head, const std::string& text)
{
    if (text.empty()) {
        setErrorMessage(tr(head.c_str()));
    }
    else {
        setErrorMessage(tr(head.c_str()) + ": " + QString::fromStdString(text));
    }
}

void CRadioController::setInfoMessage(QString Text)
{
    emit showInfoMessage(Text);
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
        qDebug() << "RadioController:" << "Radio device is not ready or does not exist.";
        emit showErrorMessage(tr("Radio device is not ready or does not exist."));
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
    if (my_rx) {
        my_rx->restart(isScan);
    }
}

void CRadioController::SetStation(QString Station, bool Force)
{
    if(CurrentStation != Station || Force == true)
    {
        CurrentStation = Station;

        qDebug() << "RadioController: Tune to station" <<  Station;

        CurrentTitle = tr("Tuning") + " ... " + Station;

        // Wait if we found the station inside the signal
        StationTimer.start(1000);

        // Clear old data
        CurrentStationType = "";
        CurrentLanguageType = "";
        CurrentText = "";

        UpdateGUIData();

        // Clear MOT
        motImage.loadFromData(0, 0, Q_NULLPTR);
        emit MOTChanged(motImage);
    }
}

void CRadioController::NextChannel(bool isWait)
{
    if (isWait) { // It might be a channel, wait 10 seconds
        ChannelTimer.start(10000);
    }
    else {
        auto Channel = QString::fromStdString(channels.getNextChannel());

        if(!Channel.isEmpty()) {
            SetChannel(Channel, true);

            int index = channels.getCurrentIndex() + 1;

            CurrentTitle = tr("Scanning") + " ... " + Channel
                    + " (" + QString::number((int)(index * 100 / NUMBEROFCHANNELS)) + "%)";

            UpdateGUIData();
            emit ScanProgress(index);
        }
        else {
            StopScan();
        }
    }
}

/********************
 * Controller slots *
 ********************/

void CRadioController::StationTimerTimeout()
{
    if (!my_rx)
        return;

    if (StationList.contains(CurrentStation)) {
        const auto services = my_rx->getServiceList();

        for (const auto& s : services) {
            if (s.serviceLabel.label == CurrentStation.toStdString()) {

                const auto comps = my_rx->getComponents(s);
                for (const auto& sc : comps) {
                    if (sc.transportMode() == TransportMode::Audio && (
                            sc.audioType() == AudioServiceComponentType::DAB ||
                            sc.audioType() == AudioServiceComponentType::DABPlus) ) {
                        const auto& subch = my_rx->getSubchannel(sc);

                        if (not subch.valid()) {
                            return;
                        }

                        // We found the station inside the signal, lets stop the timer
                        StationTimer.stop();

                        std::string dumpFileName;
                        if (commandLineOptions["dumpFileName"] != "") {
                            dumpFileName = commandLineOptions["dumpFileName"].toString().toStdString();
                        }

                        bool success = my_rx->playSingleProgramme(*this, dumpFileName, s);
                        if (!success) {
                            qDebug() << "Selecting service failed";
                        }
                        else {
                            CurrentTitle = CurrentStation;

                            CurrentStationType = tr(DABConstants::getProgramTypeName(s.programType));
                            CurrentLanguageType = tr(DABConstants::getLanguageName(s.language));

                            mBitRate = subch.bitrate();
                            emit BitRateChanged(mBitRate);

                            if (sc.audioType() == AudioServiceComponentType::DABPlus)
                                mIsDAB = false;
                            else
                                mIsDAB = true;
                            emit isDABChanged(mIsDAB);

                            Status = Playing;
                            UpdateGUIData();
                        }

                        return;
                    }
                }
            }
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
    if(!mIsSync ||
       (mIsSync && !mIsFICCRC) ||
       (mIsSync && mFrameErrors >= 10))
    {
        qDebug() << "RadioController: Restart syncing. isSync:" << mIsSync << ", isFICCRC:" << mIsFICCRC << ", FrameErrors:" << mFrameErrors;
        emit showInfoMessage(tr("Lost signal or bad signal quality, trying to find it again."));

        SetChannel(CurrentChannel, false, true);
        SetStation(CurrentStation, true);
    }
}


/*****************
 * Backend slots *
 *****************/

void CRadioController::onServiceDetected(uint32_t SId, const std::string& label)
{
    emit EnsembleAdded(SId, QString::fromStdString(label));
}

void CRadioController::addtoEnsemble(quint32 SId, const QString &Station)
{
    qDebug() << "RadioController: Found station" <<  Station
             << "(" << qPrintable(QString::number(SId, 16).toUpper()) << ")";

    if (startPlayback && StationList.isEmpty()) {
        qDebug() << "RadioController: Start playback of first station" << Station;
        startPlayback = false;
        Play(CurrentChannel, Station);
    }

    StationList.append(Station);

    if (Status == Scanning) {
        mStationCount++;
        CurrentText = tr("Found channels") + ": " + QString::number(mStationCount);
        UpdateGUIData();
    }

    //	Add new station into list
    if (!mStationList.contains(Station, CurrentChannel)) {
        mStationList.append(Station, CurrentChannel);

        //	Sort stations
        mStationList.sort();

        emit StationsChanged(mStationList.getList());
        emit FoundStation(Station, CurrentChannel);

        // Save the channels
        mStationList.saveStations();
    }
}

void CRadioController::onNewEnsembleName(const std::string& name)
{
    emit EnsembleNameUpdated(QString::fromStdString(name));
}

void CRadioController::nameofEnsemble(const QString &Ensemble)
{
    qDebug() << "RadioController: Name of ensemble:" << Ensemble;

    if (CurrentEnsemble == Ensemble)
        return;
    CurrentEnsemble = Ensemble;
    UpdateGUIData();
}


void CRadioController::onDateTimeUpdate(const dab_date_time_t& dateTime)
{
    emit DateTimeUpdated(dateTime);
}

void CRadioController::displayDateTime(const dab_date_time_t& dateTime)
{
    QDate Date;
    QTime Time;

    Time.setHMS(dateTime.hour, dateTime.minutes, dateTime.seconds);
    mCurrentDateTime.setTime(Time);

    Date.setDate(dateTime.year, dateTime.month, dateTime.day);
    mCurrentDateTime.setDate(Date);

    int OffsetFromUtc = dateTime.hourOffset * 3600 +
                        dateTime.minuteOffset * 60;
    mCurrentDateTime.setOffsetFromUtc(OffsetFromUtc);
    mCurrentDateTime.setTimeSpec(Qt::OffsetFromUTC);

    QDateTime LocalTime = mCurrentDateTime.toLocalTime();
    emit DateTimeChanged(QLocale().toString(LocalTime, QLocale::ShortFormat));
}

void CRadioController::onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib)
{
    (void)fib;
    if (mIsFICCRC == crcCheckOk)
        return;
    mIsFICCRC = crcCheckOk;
    emit isFICCRCChanged(mIsFICCRC);
}

void CRadioController::onNewImpulseResponse(std::vector<float>&& data)
{
    std::lock_guard<std::mutex> lock(impulseResponseBufferMutex);
    std::swap(impulseResponseBuffer, data);
}

void CRadioController::onConstellationPoints(std::vector<DSPCOMPLEX>&& data)
{
    std::lock_guard<std::mutex> lock(constellationPointBufferMutex);
    std::swap(constellationPointBuffer, data);
}

void CRadioController::onNewNullSymbol(std::vector<DSPCOMPLEX>&& data)
{
    std::lock_guard<std::mutex> lock(nullSymbolBufferMutex);
    std::swap(nullSymbolBuffer, data);
}

void CRadioController::onTIIMeasurement(tii_measurement_t&& m)
{
    qDebug().noquote() << "TII comb " << m.comb <<
        " pattern " << m.pattern <<
        " delay " << m.delay_samples <<
        "= " << m.getDelayKm() << " km" <<
        " with error " << m.error;
}

void CRadioController::onMessage(message_level_t level, const std::string& text)
{
    switch (level) {
        case message_level_t::Information:
            emit showInfoMessage(tr(text.c_str()));
            break;
        case message_level_t::Error:
            emit showErrorMessage(tr(text.c_str()));
            break;
    }
}

void CRadioController::onSNR(int snr)
{
    if (mSNR == snr)
        return;
    mSNR = snr;
    emit SNRChanged(mSNR);
}

void CRadioController::onFrequencyCorrectorChange(int fine, int coarse)
{
    if (mFrequencyCorrection == coarse + fine)
        return;
    mFrequencyCorrection = coarse + fine;
    emit FrequencyCorrectionChanged(mFrequencyCorrection);
}

void CRadioController::onSyncChange(char isSync)
{
    bool sync = (isSync == SYNCED) ? true : false;
    if (mIsSync == sync)
        return;
    mIsSync = sync;
    emit isSyncChanged(mIsSync);
}

void CRadioController::onSignalPresence(bool isSignal)
{
    if (mIsSignal != isSignal) {
        mIsSignal = isSignal;
        emit isSignalChanged(mIsSignal);
    }

    if (isChannelScan)
        emit SwitchToNextChannel(isSignal);
}

void CRadioController::onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, bool isStereo)
{
    audioBuffer.putDataIntoBuffer(audioData.data(), audioData.size());

    if (mAudioSampleRate != sampleRate) {
        qDebug() << "RadioController: Audio sample rate" <<  sampleRate << "kHz";
        mAudioSampleRate = sampleRate;
        emit AudioSampleRateChanged(mAudioSampleRate);

        audio.setRate(sampleRate);
    }

    if (mIsStereo != isStereo) {
        mIsStereo = isStereo;
        emit isStereoChanged(mIsStereo);
    }
}

void CRadioController::onFrameErrors(int frameErrors)
{
    if (mFrameErrors == frameErrors)
        return;
    mFrameErrors = frameErrors;
    emit FrameErrorsChanged(mFrameErrors);
}

void CRadioController::onRsErrors(int rsErrors)
{
    if (mRSErrors == rsErrors)
        return;
    mRSErrors = rsErrors;
    emit RSErrorsChanged(mRSErrors);
}

void CRadioController::onAacErrors(int aacErrors)
{
    if (mAACErrors == aacErrors)
        return;
    mAACErrors = aacErrors;
    emit AACErrorsChanged(mAACErrors);
}

void CRadioController::onNewDynamicLabel(const std::string& label)
{
    auto qlabel = QString::fromUtf8(label.c_str());
    if (this->CurrentText != qlabel) {
        this->CurrentText = qlabel;
        UpdateGUIData();
    }
}

void CRadioController::onMOT(const std::vector<uint8_t>& Data, int subtype)
{
    QByteArray qdata((const char*)Data.data(), (int)Data.size());

    motImage.loadFromData(qdata, subtype == 0 ? "GIF" : subtype == 1 ? "JPEG" : subtype == 2 ? "BMP" : "PNG");

    emit MOTChanged(motImage);
}

void CRadioController::UpdateSpectrum()
{
    int Samples = 0;
    int16_t T_u = dabparams.T_u;

    qreal tunedFrequency_MHz = 0;
    qreal sampleFrequency_MHz = 2048000 / 1e6;
    qreal dip_MHz = sampleFrequency_MHz / T_u;

    qreal x(0);
    qreal y(0);
    qreal y_max(0);

    qreal x_min = 0;
    qreal x_max = 0;

    if (PlotType == PlotTypeEn::Spectrum) {
        spectrum_data.resize(T_u);

        // Get FFT buffer
        DSPCOMPLEX* spectrumBuffer = spectrum_fft_handler->getVector();

        // Get samples
        tunedFrequency_MHz = CurrentFrequency / 1e6;
        if (Device)
            Samples = Device->getSpectrumSamples(spectrumBuffer, T_u);

        // Continue only if we got data
        if (Samples <= 0)
            return;

        // Do FFT to get the spectrum
        spectrum_fft_handler->do_FFT();

        //	Process samples one by one
        for (int i = 0; i < T_u; i++) {
            int half_Tu = T_u / 2;

            //	Shift FFT samples
            if (i < half_Tu)
                y = abs(spectrumBuffer[i + half_Tu]);
            else
                y = abs(spectrumBuffer[i - half_Tu]);

            // Apply a cumulative moving average filter
            int avg = 4; // Number of y values to average
            qreal CMA = spectrum_data[i].y();
            y = (CMA * avg + y) / (avg + 1);

            //	Find maximum value to scale the plotter
            if (y > y_max)
                y_max = y;

            // Calc x frequency
            x = (i * dip_MHz) + (tunedFrequency_MHz - (sampleFrequency_MHz / 2));

            spectrum_data[i]= QPointF(x, y);
          }

        x_min = tunedFrequency_MHz - (sampleFrequency_MHz / 2);
        x_max = tunedFrequency_MHz + (sampleFrequency_MHz / 2);
    }
    else if (PlotType == PlotTypeEn::QPSK) {
        std::unique_lock<std::mutex> lock(constellationPointBufferMutex);
        // Plotting all points is too costly, take only a subset
        const size_t decim = OfdmDecoder::constellationDecimation;
        const size_t num_iqpoints = (dabparams.L-1) * dabparams.K / decim;
        if (constellationPointBuffer.size() == num_iqpoints) {
            spectrum_data.resize(num_iqpoints);
            for (size_t i = 0; i < num_iqpoints; i++) {
                y = 180.0f / (float)M_PI * std::arg(constellationPointBuffer[i]);
                spectrum_data[i] = QPointF(i, y);
            }

            x_min = 0;
            x_max = num_iqpoints;
        }
        else {
            qDebug() << "IQ" << constellationPointBuffer.size() << num_iqpoints;
        }
    }
    else if (PlotType == PlotTypeEn::ImpulseResponse) {
        std::lock_guard<std::mutex> lock(impulseResponseBufferMutex);
        if (impulseResponseBuffer.size() == (size_t)T_u) {
            spectrum_data.resize(T_u);
            for (int i = 0; i < T_u; i++) {
                y = 10.0f * std::log10(impulseResponseBuffer[i]);
                x = i;

                // Find maximum value to scale the plotter
                if (y > y_max)
                    y_max = y;
                spectrum_data[i] = QPointF(x, y);
            }

            x_min = 0;
            x_max = T_u;
        }
    }
    else if (PlotType == PlotTypeEn::Null) {
        std::unique_lock<std::mutex> lock(nullSymbolBufferMutex);
        if (nullSymbolBuffer.size() == (size_t)dabparams.T_null) {
            spectrum_data.resize(T_u);
            // Get FFT buffer
            DSPCOMPLEX* spectrumBuffer = spectrum_fft_handler->getVector();

            std::copy(nullSymbolBuffer.begin(), nullSymbolBuffer.begin() + T_u,
                    spectrumBuffer);
            lock.unlock();

            // Do FFT to get the spectrum
            spectrum_fft_handler->do_FFT();

            tunedFrequency_MHz = CurrentFrequency / 1e6;

            // Process samples one by one
            for (int i = 0; i < T_u; i++) {
                int half_Tu = T_u / 2;

                // Shift FFT samples
                if (i < half_Tu)
                    y = abs(spectrumBuffer[i + half_Tu]);
                else
                    y = abs(spectrumBuffer[i - half_Tu]);

                // Apply a cumulative moving average filter
                int avg = 4; // Number of y values to average
                qreal CMA = spectrum_data[i].y();
                y = (CMA * avg + y) / (avg + 1);

                // Find maximum value to scale the plotter
                if (y > y_max)
                    y_max = y;

                // Calc x frequency
                x = (i * dip_MHz) + (tunedFrequency_MHz - (sampleFrequency_MHz / 2));

                spectrum_data[i]= QPointF(x, y);
            }

            x_min = tunedFrequency_MHz - (sampleFrequency_MHz / 2);
            x_max = tunedFrequency_MHz + (sampleFrequency_MHz / 2);
        }
    }

    float y_max_with_margin = (PlotType == PlotTypeEn::QPSK) ?
        180 :
        round(y_max) + 1;

    //	Set new data
    emit SpectrumUpdated(y_max_with_margin,
                         x_min,
                         x_max,
                         spectrum_data);
}

void CRadioController::setPlotType(PlotTypeEn PlotType)
{
    this->PlotType = PlotType;
}

void CRadioController::setAutoPlay(QString Channel, QString Station)
{
    isAutoPlay = true;
    autoChannel = Channel;
    autoStation = Station;
}
