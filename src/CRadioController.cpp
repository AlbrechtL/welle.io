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

#include "CRadioController.h"

#define AUDIOBUFFERSIZE 32768

CRadioController::CRadioController(CVirtualInput *Device, CDABParams& DABParams, QObject *parent): QObject(parent)
{
    this->Device = Device;
    this->DABParams = DABParams;

    /**
    *	With this GUI there is no choice for the output channel,
    *	It is the soundcard, so just allocate it
    */
    AudioBuffer = new RingBuffer<int16_t>(2 * AUDIOBUFFERSIZE);
    Audio = new CAudio(AudioBuffer);

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

    MOTImage = new QPixmap(320, 240);
    isChannelScan = false;

    // Init the technical data
    CurrentChannel = "Unknown";
    CurrentFrequency = 0;
    CurrentStation = "";
    CurrentDisplayStation = "No Station";
    CurrentStationType = "";
    CurrentLanguageType = "";
    Label = "";
    isSync = false;
    isFICCRC = false;
    isSignal = false;
    SNR = 0;
    FrequencyCorrection = 0;
    BitRate = 0;
    isStereo = true;
    isDAB = true;
    FrameErrors = 0;
    RSErrors = 0;
    AACErrors = 0;
    CurrentManualGain = 0;
    CurrentManualGainValue = 0.0;

    connect(&StationTimer, SIGNAL(timeout(void)), this, SLOT(StationTimerTimeout(void)));
    connect(&ChannelTimer, SIGNAL(timeout(void)), this, SLOT(ChannelTimerTimeout(void)));
}

void CRadioController::Play(QString Channel, QString Station)
{
    qDebug() << "RadioController:" << "Start or restart device";

    DeviceRestart();
    SetChannel(Channel, false);
    SetStation(Station);
}

void CRadioController::StartScan(void)
{
    qDebug() << "RadioController:" << "Start channel scan";

    DeviceRestart();

    if(Device && Device->getID() == CDeviceID::RAWFILE)
    {
        CurrentDisplayStation = "RAW File";
        SetChannel(CChannels::FirstChannel, false); // Just a dummy
        emit ScanStopped();
    }
    else
    {
        // Start with lowest frequency
        SetChannel(CChannels::FirstChannel, true);

        isChannelScan = true;
        CurrentDisplayStation = "Scanning ...";
        Label = CChannels::FirstChannel;
    }
}

void CRadioController::StopScan(void)
{
    qDebug() << "RadioController:" << "Stop channel scan";

    isChannelScan = false;
    CurrentDisplayStation = "No Station";
    Label = "";

    emit ScanStopped();
}

QVariantMap CRadioController::GetGUIData(void)
{
    // Init the GUI data map
    GUIData["Channel"] = CurrentChannel;
    GUIData["Frequency"] = CurrentFrequency;
    GUIData["Station"] = CurrentDisplayStation.simplified();
    GUIData["DateTime"] = CurrentDateTime.toString("dd.MM.yyyy hh:mm");
    GUIData["isSync"] = isSync;
    GUIData["isFICCRC"] = isFICCRC;
    GUIData["isSignal"] = isSignal;
    GUIData["SNR"] = SNR;
    GUIData["FrequencyCorrection"] = FrequencyCorrection;
    GUIData["BitRate"] = BitRate;
    GUIData["isStereo"] = isStereo;
    GUIData["FrameErrors"] = FrameErrors;
    GUIData["RSErrors"] = RSErrors;
    GUIData["AACErrors"] = AACErrors;
    GUIData["Label"] = Label;
    GUIData["StationType"] = CurrentStationType;
    GUIData["LanguageType"] = CurrentLanguageType;
    GUIData["isDAB"] = isDAB;

    if(Device) GUIData["DeviceName"] = Device->getName();

    return GUIData;
}

QPixmap CRadioController::GetMOTImage()
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

int CRadioController::GetGainCount()
{
    int GainCount = 0;

    if(Device)
        GainCount = Device->getGainCount();

    qDebug() << "RadioController:" << "Number of gain steps:" << GainCount;

    return GainCount;
}

void CRadioController::SetAGC(bool isAGC)
{
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
}

float CRadioController::SetGain(int Gain)
{
    if (Device)
    {
        CurrentManualGainValue = Device->setGain(Gain);
        CurrentManualGain = Gain;
    }

    return CurrentManualGainValue;
}


/********************
 * Private methods  *
 ********************/

void CRadioController::DeviceRestart()
{
    bool isPlay = false;

    if(Device)
        isPlay = Device->restart();

    if (!isPlay)
    {
        qDebug() << "RadioController:" << "Start or restart of device failed!";
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
    my_ofdmProcessor->set_scanMode(isScan);

    my_mscHandler->stopProcessing();
    my_ficHandler->clearEnsemble();
    my_ofdmProcessor->coarseCorrectorOn();
    my_ofdmProcessor->reset();
}

void CRadioController::SetChannel(QString Channel, bool isScan)
{
    if(CurrentChannel != Channel)
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

            if(CurrentFrequency != 0)
            {
                qDebug() << "RadioController: Tune to channel" <<  Channel << "->" << CurrentFrequency/1e6 << "MHz";
                Device->setFrequency(CurrentFrequency);
            }
        }

        DecoderRestart(isScan);

        StationList.clear();
    }
}

void CRadioController::SetStation(QString Station)
{
    if(CurrentStation != Station)
    {
        CurrentStation = Station;

        qDebug() << "RadioController: Tune to station" <<  Station;

        CurrentDisplayStation = "Tuning";

        // Wait if we found the station inside the signal
        StationTimer.start(1000);

        // Clear old data
        CurrentStationType = "";
        CurrentLanguageType = "";
        Label = "";

        // Clear MOT
        QPixmap MOT(320, 240);
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
        // We found the station inside the signal, lets stop the timer
        StationTimer.stop();

        // Set station
        audiodata AudioData;
        my_ficHandler->dataforAudioService(CurrentStation, &AudioData);
        my_mscHandler->set_audioChannel(&AudioData);

        CurrentDisplayStation = CurrentStation;
        CurrentStationType = CDABConstants::getProgramTypeName(AudioData.programType);
        CurrentLanguageType = CDABConstants::getLanguageName(AudioData.language);
        BitRate = AudioData.bitRate;

        if (AudioData.ASCTy == 077)
            isDAB = false;
        else
            isDAB = true;
    }
}

void CRadioController::ChannelTimerTimeout(void)
{
    ChannelTimer.stop();

    if(isChannelScan)
        NextChannel(false);
}


/*****************
 * Backend slots *
 *****************/

void CRadioController::addtoEnsemble(const QString &Station)
{
    qDebug() << "RadioController: Found station" <<  Station;

    StationList.append(Station);

    emit FoundStation(Station, CurrentChannel);
}

void CRadioController::nameofEnsemble(int id, const QString &v)
{
    (void)id;
    (void)v;
    // Unknown use case
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

    Time.setHMS(Hour + HourOffset, Minute + MinuteOffset, Seconds);
    Date.setDate(Year, Month, Day);

    CurrentDateTime.setDate(Date);
    CurrentDateTime.setTime(Time);
}

void CRadioController::show_ficSuccess(bool isFICCRC)
{
    this->isFICCRC = isFICCRC;
}

void CRadioController::show_snr(int SNR)
{
    this->SNR = SNR;
}

void CRadioController::set_fineCorrectorDisplay(int FineFrequencyCorr)
{
    int CoarseFrequencyCorr = (FrequencyCorrection / 1000);
    FrequencyCorrection = (CoarseFrequencyCorr * 1000) + FineFrequencyCorr;
}

void CRadioController::set_coarseCorrectorDisplay(int CoarseFreuqencyCorr)
{
    int OldCoareFrequencyCorrr = (FrequencyCorrection / 1000);
    int FineFrequencyCorr = FrequencyCorrection - (OldCoareFrequencyCorrr * 1000);
    FrequencyCorrection = (CoarseFreuqencyCorr * 1000) + FineFrequencyCorr;
}

void CRadioController::setSynced(char isSync)
{
    if(isSync == SYNCED)
        this->isSync = true;
    else
        this->isSync = false;
}

void CRadioController::setSignalPresent(bool isSignal)
{
    this->isSignal = isSignal;

    if(isChannelScan)
        NextChannel(isSignal);
}

void CRadioController::setErrorMessage(QString ErrorMessage)
{
    (void)ErrorMessage; // ToDo
}

void CRadioController::newAudio(int BitRate)
{
    (void) BitRate; // Not used
}

void CRadioController::setStereo(bool isStereo)
{
    this->isStereo = isStereo;
}

void CRadioController::show_frameErrors(int FrameErrors)
{
    this->FrameErrors = FrameErrors;
}

void CRadioController::show_rsErrors(int RSErrors)
{
    this->RSErrors = RSErrors;
}

void CRadioController::show_aacErrors(int AACErrors)
{
    this->AACErrors = AACErrors;
}

void CRadioController::showLabel(QString Label)
{
    this->Label = Label;
}

void CRadioController::showMOT(QByteArray Data, int Subtype, QString s)
{
    (void)s; // Not used, can be removed

    MOTImage->loadFromData(Data, Subtype == 0 ? "GIF" : Subtype == 1 ? "JPEG" : Subtype == 2 ? "BMP" : "PNG");

    emit MOTChanged(*MOTImage);
}
