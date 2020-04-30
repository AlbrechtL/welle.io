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

#include <QCoreApplication>
#include <QDebug>
#include <QSettings>
#include <QStandardPaths>
#include <stdexcept>

#include "radio_controller.h"
#ifdef HAVE_SOAPYSDR
#include "soapy_sdr.h"
#endif /* HAVE_SOAPYSDR */
#include "input_factory.h"
#include "raw_file.h"
#include "rtl_tcp.h"

#define AUDIOBUFFERSIZE 32768

static QString serialise_serviceid(quint32 serviceid) {
    return QString::asprintf("%x", serviceid);
}

static quint32 deserialise_serviceid(const char *input)
{
    long value = 0;
    errno = 0;

    char* endptr = nullptr;
    value = strtol(input, &endptr, 16);

    if ((value == LONG_MIN or value == LONG_MAX) and errno == ERANGE) {
        return 0;
    }
    else if (value == 0 and errno != 0) {
        return 0;
    }
    else if (input == endptr) {
        return 0;
    }
    else if (*endptr != '\0') {
        return 0;
    }

    return value;
}

CRadioController::CRadioController(QVariantMap& commandLineOptions, QObject *parent)
    : QObject(parent)
    , commandLineOptions(commandLineOptions)
    , audioBuffer(2 * AUDIOBUFFERSIZE)
    , audio(audioBuffer)
{
    // Init the technical data
    resetTechnicalData();

    // Init timers
    connect(&labelTimer, &QTimer::timeout, this, &CRadioController::labelTimerTimeout);
    connect(&stationTimer, &QTimer::timeout, this, &CRadioController::stationTimerTimeout);
    connect(&channelTimer, &QTimer::timeout, this, &CRadioController::channelTimerTimeout);

    // Use the signal slot mechanism is necessary because the backend runs in a different thread
    connect(this, &CRadioController::switchToNextChannel,
            this, &CRadioController::nextChannel);

    connect(this, &CRadioController::ensembleIdUpdated,
            this, &CRadioController::ensembleId);

    qRegisterMetaType<DabLabel>("DabLabel&");
    connect(this, &CRadioController::ensembleLabelUpdated,
            this, &CRadioController::ensembleLabel);
        
    connect(this, &CRadioController::serviceDetected,
            this, &CRadioController::serviceId);

    qRegisterMetaType<dab_date_time_t>("dab_date_time_t");
    connect(this, &CRadioController::dateTimeUpdated,
            this, &CRadioController::displayDateTime);
}

CRadioController::~CRadioController()
{
    closeDevice();
    qDebug() << "RadioController:" << "Deleting CRadioController";
}

void CRadioController::closeDevice()
{
    qDebug() << "RadioController:" << "Close device";

    radioReceiver.reset();
    device.reset();
    audio.reset();

    // Reset the technical data
    resetTechnicalData();

    emit deviceClosed();
}

CDeviceID CRadioController::openDevice(CDeviceID deviceId, bool force, QVariant param1, QVariant param2)
{
    if(this->deviceId != deviceId || force) {
        closeDevice();
        device.reset(CInputFactory::GetDevice(*this, deviceId));

        // Set rtl_tcp settings
        if (device->getID() == CDeviceID::RTL_TCP) {
            CRTL_TCP_Client* RTL_TCP_Client = static_cast<CRTL_TCP_Client*>(device.get());

            RTL_TCP_Client->setServerAddress(param1.toString().toStdString());
            RTL_TCP_Client->setPort(param2.toInt());
        }

        // Set rtl_tcp settings
        if (device->getID() == CDeviceID::RAWFILE) {
            CRAWFile* rawFile = static_cast<CRAWFile*>(device.get());
#ifdef __ANDROID__
            // Using QFile is necessary to get access to com.android.externalstorage.ExternalStorageProvider
            rawFileAndroid.reset(new QFile(param1.toString()));
            bool ret = rawFileAndroid->open(QIODevice::ReadOnly);
            if(!ret) {
                setErrorMessage(tr("Error while opening file ") + param1.toString());
            }
            else {
                rawFile->setFileHandle(rawFileAndroid->handle(), param2.toString().toStdString());
            }
#else
            rawFile->setFileName(param1.toString().toStdString(), param2.toString().toStdString());
#endif
        }

        initialise();
    }

    return device->getID();
}

CDeviceID CRadioController::openDevice()
{
    closeDevice();
    device.reset(CInputFactory::GetDevice(*this, "auto"));
    initialise();

    return device->getID();
}

void CRadioController::setDeviceParam(QString param, int value)
{
    if (param == "biastee") {
        deviceParametersInt[DeviceParam::BiasTee] = value;
    }
    else {
        qDebug() << "Invalid device parameter setting: " << param;
    }

    if (device) {
        device->setDeviceParam(DeviceParam::BiasTee, value);
    }
}

void CRadioController::setDeviceParam(QString param, QString value)
{
    DeviceParam dp;
    bool deviceParamChanged = false;

    if (param == "SoapySDRAntenna") {
        dp = DeviceParam::SoapySDRAntenna;
    }
    else if (param == "SoapySDRDriverArgs") {
        dp = DeviceParam::SoapySDRDriverArgs;
    }
    else if (param == "SoapySDRClockSource") {
        dp = DeviceParam::SoapySDRClockSource;
    }
    else {
        qDebug() << "Invalid device parameter setting: " << param;
        return;
    }

    std::string v = value.toStdString();

    if (deviceParametersString[dp] != v) {
        deviceParamChanged = true;
        deviceParametersString[dp] = v;
    }

    if (device && deviceParamChanged) {
        device->setDeviceParam(dp, v);
        if (dp == DeviceParam::SoapySDRDriverArgs)
            openDevice(CDeviceID::SOAPYSDR,1);
    }
}

void CRadioController::play(QString channel, QString title, quint32 service)
{
    if (channel == "") {
        return;
    }

    currentTitle = title;
    emit titleChanged();


    qDebug() << "RadioController:" << "Play:" << title << serialise_serviceid(service) << "on channel" << channel;

    if (isChannelScan == true) {
        stopScan();
    }

    deviceRestart();
    setChannel(channel, false);
    setService(service);

    currentLastChannel = QStringList() << serialise_serviceid(service) << channel;
    QSettings settings;
    settings.setValue("lastchannel", currentLastChannel);
}

void CRadioController::stop()
{
    if (device) {
        device->stop();
    }
    else
        throw std::runtime_error("device is null in file " + std::string(__FILE__) +":"+ std::to_string(__LINE__));

    audio.reset();
    labelTimer.stop();
}

void CRadioController::setService(uint32_t service, bool force)
{
    if (currentService != service or force) {
        currentService = service;
        autoService = service;
        emit stationChanged();

        // Wait if we found the station inside the signal
        stationTimer.start(1000);

        // Clear old data
        currentStationType = "";
        emit stationTypChanged();

        currentLanguageType = "";
        emit languageTypeChanged();

        currentText = "";
        emit textChanged();

        emit motReseted();
    }
}

void CRadioController::setAutoPlay(bool isAutoPlayValue, QString channel, QString service)
{
    isAutoPlay = isAutoPlayValue;
    autoChannel = channel;
    autoService = deserialise_serviceid(service.toStdString().c_str());
    currentLastChannel = QStringList() << service << channel;
}

void CRadioController::setVolume(qreal Volume)
{
    currentVolume = Volume;
    audio.setVolume(Volume);
    emit volumeChanged(currentVolume);
}

void CRadioController::setChannel(QString Channel, bool isScan, bool Force)
{
    if (currentChannel != Channel || Force == true) {
        if (device && device->getID() == CDeviceID::RAWFILE) {
            currentChannel = "File";
            autoChannel = currentChannel;
            currentEId = 0;
            currentEnsembleLabel = "";
            currentFrequency = 0;
        }
        else { // A real device
            currentChannel = Channel;
            autoChannel = currentChannel;
            currentEId = 0;
            currentEnsembleLabel = "";

            // Convert channel into a frequency
            currentFrequency = channels.getFrequency(Channel.toStdString());

            if(currentFrequency != 0 && device) {
                qDebug() << "RadioController: Tune to channel" <<  Channel << "->" << currentFrequency/1e6 << "MHz";
                device->setFrequency(currentFrequency);
                device->reset(); // Clear buffer
            }
        }

        // Restart demodulator and decoder
        if(device) {
            radioReceiver = std::make_unique<RadioReceiver>(*this, *device, rro, 1);
            radioReceiver->setReceiverOptions(rro);
            radioReceiver->restart(isScan);
        }

        emit channelChanged();
        emit ensembleChanged();
        emit frequencyChanged();
    }
}

void CRadioController::setManualChannel(QString Channel)
{
    // Otherwise tune to channel and play first found station
    qDebug() << "RadioController: Tune to channel" <<  Channel;

    deviceRestart();

    currentTitle = Channel;
    emit titleChanged();

    currentService = 0;
    emit stationChanged();

    currentStationType = "";
    emit stationTypChanged();

    currentLanguageType = "";
    emit languageTypeChanged();

    currentText = "";
    emit textChanged();

    emit motReseted();

    // Switch channel
    setChannel(Channel, false, true);
}

void CRadioController::startScan(void)
{
    qDebug() << "RadioController:" << "Start channel scan";

    deviceRestart();

    if(device && device->getID() == CDeviceID::RAWFILE) {
        currentTitle = tr("RAW File");
        const auto FirstChannel = QString::fromStdString(Channels::firstChannel);
        setChannel(FirstChannel, false); // Just a dummy
        emit scanStopped();
    }
    else
    {
        // Start with lowest frequency
        QString Channel = QString::fromStdString(Channels::firstChannel);
        setChannel(Channel, true);

        isChannelScan = true;
        stationCount = 0;
        currentTitle = tr("Scanning") + " ... " + Channel
                + " (" + QString::number((1 * 100 / NUMBEROFCHANNELS)) + "%)";
        emit titleChanged();

        currentText = tr("Found channels") + ": " + QString::number(stationCount);
        emit textChanged();

        currentService = 0;
        emit stationChanged();

        currentStationType = "";
        emit stationTypChanged();

        currentLanguageType = "";
        emit languageTypeChanged();

        emit scanProgress(0);
    }
}

void CRadioController::stopScan(void)
{
    qDebug() << "RadioController:" << "Stop channel scan";

    currentTitle = tr("No Station");
    emit titleChanged();

    currentText = "";
    emit textChanged();

    isChannelScan = false;
    emit scanStopped();

    stop();
}

void CRadioController::setAGC(bool isAGC)
{
    this->isAGC = isAGC;

    if (device) {
        device->setAgc(isAGC);

        if (!isAGC) {
            device->setGain(currentManualGain);
            qDebug() << "RadioController:" << "AGC off";
        }
        else {
            qDebug() << "RadioController:" <<  "AGC on";
        }
    }

    emit agcChanged(isAGC);
}

void CRadioController::disableCoarseCorrector(bool disable)
{
    rro.disableCoarseCorrector = disable;
    if (radioReceiver) {
        radioReceiver->setReceiverOptions(rro);
    }
}

void CRadioController::enableTIIDecode(bool enable)
{
    rro.decodeTII = enable;
    if (radioReceiver) {
        radioReceiver->setReceiverOptions(rro);
    }
}

void CRadioController::selectFFTWindowPlacement(int fft_window_placement_ix)
{
    if (fft_window_placement_ix == 0) {
        rro.fftPlacementMethod = FFTPlacementMethod::StrongestPeak;
    }
    else if (fft_window_placement_ix == 1) {
        rro.fftPlacementMethod = FFTPlacementMethod::EarliestPeakWithBinning;
    }
    else if (fft_window_placement_ix == 2) {
        rro.fftPlacementMethod = FFTPlacementMethod::ThresholdBeforePeak;
    }
    else {
        std::clog << "Invalid FFT window placement " <<
            fft_window_placement_ix << " chosen" << std::endl;
        return;
    }

    if (radioReceiver) {
        radioReceiver->setReceiverOptions(rro);
    }
}

void CRadioController::setFreqSyncMethod(int fsm_ix)
{
    rro.freqsyncMethod = static_cast<FreqsyncMethod>(fsm_ix);

    if (radioReceiver) {
        radioReceiver->setReceiverOptions(rro);
    }
}

void CRadioController::setGain(int Gain)
{
    currentManualGain = Gain;
    emit gainChanged(currentManualGain);

    if (device) {
        currentManualGainValue = device->setGain(Gain);
        emit gainValueChanged(currentManualGainValue);

        int32_t gainCount_tmp = device->getGainCount();

        if(gainCount != gainCount_tmp) {
            gainCount = gainCount_tmp;
            emit gainCountChanged(gainCount);
        }
    }
}

void CRadioController::initRecorder(int size)
{
    device->initRecordBuffer(size);
}

void CRadioController::triggerRecorder(QString filename)
{
    // TODO just for testing
    filename = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/welle-io-record.iq";
    std::string filename_tmp = filename.toStdString();
    device->writeRecordBufferToFile(filename_tmp);
}

DABParams& CRadioController::getParams()
{
    static DABParams dummyParams(1);

    if(radioReceiver)
        return radioReceiver->getParams();
    else
        return dummyParams;
}

int CRadioController::getCurrentFrequency()
{
    return currentFrequency;
}

std::vector<float> CRadioController::getImpulseResponse()
{
    std::lock_guard<std::mutex> lock(impulseResponseBufferMutex);
    auto buf = std::move(impulseResponseBuffer);
    return buf;
}

std::vector<DSPCOMPLEX> CRadioController::getSignalProbe()
{
    int16_t T_u = getParams().T_u;

    if (device) {
        return device->getSpectrumSamples(T_u);
    }
    else {
        std::vector<DSPCOMPLEX> dummyBuf(static_cast<std::vector<DSPCOMPLEX>::size_type>(T_u));
        return dummyBuf;
    }
}

std::vector<DSPCOMPLEX> CRadioController::getNullSymbol()
{
    std::lock_guard<std::mutex> lock(nullSymbolBufferMutex);
    auto buf = std::move(nullSymbolBuffer);
    return buf;
}

std::vector<DSPCOMPLEX> CRadioController::getConstellationPoint()
{
    std::lock_guard<std::mutex> lock(constellationPointBufferMutex);
    auto buf = std::move(constellationPointBuffer);
    return buf;
}

/********************
 * Private methods  *
 ********************/
void CRadioController::initialise(void)
{
    for (const auto param_value : deviceParametersString) {
        device->setDeviceParam(param_value.first, param_value.second);
    }

    for (const auto param_value : deviceParametersInt) {
        device->setDeviceParam(param_value.first, param_value.second);
    }

    gainCount = device->getGainCount();
    emit gainCountChanged(gainCount);
    emit deviceReady();

    if (!isAGC) { // Manual AGC
        device->setAgc(false);
        currentManualGainValue = device->setGain(currentManualGain);
        emit gainValueChanged(currentManualGainValue);

        qDebug() << "RadioController:" << "AGC off";
    }
    else {
        device->setAgc(true);
        qDebug() << "RadioController:" << "AGC on";
    }

    audio.setVolume(currentVolume);

    deviceName = QString::fromStdString(device->getDescription());
    emit deviceNameChanged();

    deviceId = device->getID();
    emit deviceIdChanged();

    if(isAutoPlay) {
        play(autoChannel, tr("Playing last station"), autoService);
    }
}

void CRadioController::resetTechnicalData(void)
{
    currentChannel = tr("Unknown");
    emit channelChanged();

    currentEId = 0;
    currentEnsembleLabel = "";
    emit ensembleChanged();

    currentFrequency = 0;
    emit frequencyChanged();

    currentService = 0;
    emit stationChanged();

    currentStationType = "";
    emit stationTypChanged();

    currentLanguageType = "";
    emit languageTypeChanged();

    currentTitle = tr("No Station");
    emit titleChanged();

    currentText = "";
    emit textChanged();

    errorMsg = "";
    isSync = false;
    isFICCRC = false;
    isSignal = false;
    snr = 0;
    frequencyCorrection = 0;
    frequencyCorrectionPpm = NAN;
    bitRate = 0;
    audioSampleRate = 0;
    isDAB = true;
    frameErrors = 0;
    rsErrors = 0;
    aaErrors = 0;

    emit motReseted();
}

void CRadioController::deviceRestart()
{
    bool isPlay = false;

    if(device) {
        isPlay = device->restart();
    }

    if(!isPlay) {
        qDebug() << "RadioController:" << "Radio device is not ready or does not exist.";
        emit showErrorMessage(tr("Radio device is not ready or does not exist."));
        return;
    }

    labelTimer.start(40);
}

/*****************
 * Public slots *
 *****************/
void CRadioController::ensembleId(quint16 eId)
{
    qDebug() << "RadioController: ID of ensemble:" << eId;

    if (currentEId == eId)
        return;

    currentEId = eId;

    //auto label = radioReceiver->getEnsembleLabel();
    //currentEnsembleLabel = QString::fromStdString(label.utf8_label());

    //emit ensembleChanged();
}

void CRadioController::ensembleLabel(DabLabel& label)
{
    QString newLabel = QString::fromStdString(label.utf8_label());

    if (currentEnsembleLabel == newLabel)
        return;

    qDebug() << "RadioController: Label of ensemble:" << newLabel;
    currentEnsembleLabel = newLabel;

    emit ensembleChanged();
}

void CRadioController::setErrorMessage(QString Text)
{
    errorMsg = Text;
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
 * private slots *
 ********************/
void CRadioController::labelTimerTimeout()
{
    if (radioReceiver and not pendingLabels.empty()) {
        const auto sId = pendingLabels.front();
        pendingLabels.pop_front();

        std::string label;

        auto srv = radioReceiver->getService(sId);
        if (srv.serviceId != 0) {
            label = srv.serviceLabel.utf8_label();
        }

        if (not label.empty()) {
            const auto qlabel = QString::fromStdString(label);
            emit newStationNameReceived(qlabel, sId, currentChannel);
            qDebug() << "RadioController: Found service " << qPrintable(QString::number(sId, 16).toUpper()) << qlabel;

            if (currentService == sId) {
                currentTitle = qlabel;
                emit titleChanged();
            }
        }
        else {
            // Rotate pending labels to avoid getting stuck on a failing one
            pendingLabels.push_back(sId);
        }
    }
}

void CRadioController::stationTimerTimeout()
{
    if (!radioReceiver)
        return;

    const auto services = radioReceiver->getServiceList();

    for (const auto& s : services) {
        if (s.serviceId == currentService) {
            const auto comps = radioReceiver->getComponents(s);
            for (const auto& sc : comps) {
                if (sc.transportMode() == TransportMode::Audio && (
                        sc.audioType() == AudioServiceComponentType::DAB ||
                        sc.audioType() == AudioServiceComponentType::DABPlus) ) {
                    const auto& subch = radioReceiver->getSubchannel(sc);

                    if (not subch.valid()) {
                        return;
                    }

                    // We found the station inside the signal, lets stop the timer
                    stationTimer.stop();

                    std::string dumpFileName;
                    if (commandLineOptions["dumpFileName"] != "") {
                        dumpFileName = commandLineOptions["dumpFileName"].toString().toStdString();
                    }

                    bool success = radioReceiver->playSingleProgramme(*this, dumpFileName, s);
                    if (!success) {
                        qDebug() << "Selecting service failed";
                    }
                    else {
                        currentStationType = DABConstants::getProgramTypeName(s.programType);
                        emit stationTypChanged();

                        currentLanguageType = DABConstants::getLanguageName(s.language);
                        emit languageTypeChanged();

                        bitRate = subch.bitrate();
                        emit bitRateChanged(bitRate);

                        if (sc.audioType() == AudioServiceComponentType::DABPlus)
                            isDAB = false;
                        else
                            isDAB = true;
                        emit isDABChanged(isDAB);
                    }

                    return;
                }
            }
        }
    }
}

void CRadioController::channelTimerTimeout(void)
{
    channelTimer.stop();

    if(isChannelScan)
        nextChannel(false);
}

void CRadioController::displayDateTime(const dab_date_time_t& dateTime)
{
    QDate Date;
    QTime Time;

    Time.setHMS(dateTime.hour, dateTime.minutes, dateTime.seconds);
    currentDateTime.setTime(Time);

    Date.setDate(dateTime.year, dateTime.month, dateTime.day);
    currentDateTime.setDate(Date);

    int OffsetFromUtc = dateTime.hourOffset * 3600 +
                        dateTime.minuteOffset * 60;
    currentDateTime.setOffsetFromUtc(OffsetFromUtc);
    currentDateTime.setTimeSpec(Qt::OffsetFromUTC);

    emit dateTimeChanged(currentDateTime);
}

void CRadioController::nextChannel(bool isWait)
{
    if (isWait) { // It might be a channel, wait 10 seconds
        channelTimer.start(10000);
    }
    else {
        auto Channel = QString::fromStdString(channels.getNextChannel());

        if(!Channel.isEmpty()) {
            setChannel(Channel, true);

            int index = channels.getCurrentIndex() + 1;

            currentTitle = tr("Scanning") + " ... " + Channel
                    + " (" + QString::number(index * 100 / NUMBEROFCHANNELS) + "%)";
            emit titleChanged();

            emit scanProgress(index);
        }
        else {
            stopScan();
        }
    }
}

/*********************
 * Backend callbacks *
 *********************/
void CRadioController::onServiceDetected(uint32_t sId)
{
    // you may not call radioReceiver->getService() because it internally holds the FIG mutex.
    emit serviceDetected(sId);
}

void CRadioController::serviceId(quint32 sId)
{
    if (isChannelScan == true) {
        stationCount++;
        currentText = tr("Found channels") + ": " + QString::number(stationCount);
        emit textChanged();
    }

    if (sId <= 0xFFFF) {
        // Exclude data services from the list
        pendingLabels.push_back(sId);
    }
}

void CRadioController::onNewEnsemble(quint16 eId)
{
    emit ensembleIdUpdated(eId);
}

void CRadioController::onSetEnsembleLabel(DabLabel& label)
{
    emit ensembleLabelUpdated(label);
}

void CRadioController::onDateTimeUpdate(const dab_date_time_t& dateTime)
{
    emit dateTimeUpdated(dateTime);
}

void CRadioController::onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib)
{
    (void)fib;
    if (isFICCRC == crcCheckOk)
        return;
    isFICCRC = crcCheckOk;
    emit isFICCRCChanged(isFICCRC);
}

void CRadioController::onNewImpulseResponse(std::vector<float>&& data)
{
    std::lock_guard<std::mutex> lock(impulseResponseBufferMutex);
    impulseResponseBuffer = std::move(data);
}

void CRadioController::onConstellationPoints(std::vector<DSPCOMPLEX>&& data)
{
    std::lock_guard<std::mutex> lock(constellationPointBufferMutex);
    constellationPointBuffer = std::move(data);
}

void CRadioController::onNewNullSymbol(std::vector<DSPCOMPLEX>&& data)
{
    std::lock_guard<std::mutex> lock(nullSymbolBufferMutex);
    nullSymbolBuffer = std::move(data);
}

void CRadioController::onTIIMeasurement(tii_measurement_t&& m)
{
    qDebug().noquote() << "TII comb " << m.comb <<
        " pattern " << m.pattern <<
        " delay " << m.delay_samples <<
        "= " << m.getDelayKm() << " km" <<
        " with error " << m.error;
}

void CRadioController::onMessage(message_level_t level, const std::string& text, const std::string& text2)
{
    QString fullText;
    if (text2.empty())
      fullText = tr(text.c_str());
    else
      fullText = tr(text.c_str()) + QString::fromStdString(text2);
    
    switch (level) {
        case message_level_t::Information:
            emit showInfoMessage(fullText);
            break;
        case message_level_t::Error:
            emit showErrorMessage(fullText);
            break;
    }
}

void CRadioController::onSNR(int snr)
{
    if (this->snr == snr)
        return;
    this->snr = snr;
    emit snrChanged(this->snr);
}

void CRadioController::onFrequencyCorrectorChange(int fine, int coarse)
{
    if (frequencyCorrection == coarse + fine)
        return;
    frequencyCorrection = coarse + fine;
    emit frequencyCorrectionChanged(frequencyCorrection);

    if (currentFrequency != 0)
        frequencyCorrectionPpm = -1000000.0f * static_cast<float>(frequencyCorrection) / static_cast<float>(currentFrequency);
    else
        frequencyCorrectionPpm = NAN;
    emit frequencyCorrectionPpmChanged(frequencyCorrectionPpm);
}

void CRadioController::onSyncChange(char isSync)
{
    bool sync = (isSync == SYNCED) ? true : false;
    if (this->isSync == sync)
        return;
    this->isSync = sync;
    emit isSyncChanged(isSync);
}

void CRadioController::onSignalPresence(bool isSignal)
{
    if (this->isSignal != isSignal) {
        this->isSignal = isSignal;
        emit isSignalChanged(isSignal);
    }

    if (isChannelScan)
        emit switchToNextChannel(isSignal);
}

void CRadioController::onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, const std::string& mode)
{
    audioBuffer.putDataIntoBuffer(audioData.data(), static_cast<int32_t>(audioData.size()));

    if (audioSampleRate != sampleRate) {
        qDebug() << "RadioController: Audio sample rate" <<  sampleRate << "Hz, mode=" <<
            QString::fromStdString(mode);
        audioSampleRate = sampleRate;

        audio.setRate(sampleRate);
    }

    if (audioMode != QString::fromStdString(mode)) {
        audioMode = QString::fromStdString(mode);
        emit audioModeChanged(audioMode);
    }
}

void CRadioController::onFrameErrors(int frameErrors)
{
    if (this->frameErrors == frameErrors)
        return;
    this->frameErrors = frameErrors;
    emit frameErrorsChanged(this->frameErrors);
}

void CRadioController::onRsErrors(bool uncorrectedErrors, int numCorrectedErrors)
{
    (void)numCorrectedErrors;

    if (this->rsErrors == uncorrectedErrors ? 1 : 0)
        return;
    this->rsErrors = uncorrectedErrors ? 1 : 0;
    emit rsErrorsChanged(this->rsErrors);
}

void CRadioController::onAacErrors(int aacErrors)
{
    if (this->aaErrors == aacErrors)
        return;
    this->aaErrors = aacErrors;
    emit aacErrorsChanged(this->aaErrors);
}

void CRadioController::onNewDynamicLabel(const std::string& label)
{
    auto qlabel = QString::fromUtf8(label.c_str());
    if (this->currentText != qlabel) {
        this->currentText = qlabel;
        emit textChanged();
    }
}

void CRadioController::onMOT(const mot_file_t& mot_file)
{
    emit motChanged(mot_file);
}

void CRadioController::onPADLengthError(size_t announced_xpad_len, size_t xpad_len)
{
    qDebug() << "X-PAD length mismatch, expected:" << announced_xpad_len << " effective:" << xpad_len;
}

void CRadioController::onInputFailure()
{
    stop();
}
