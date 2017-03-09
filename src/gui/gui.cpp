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

#include <QSettings>

#include "CInputFactory.h"
#include "gui.h"
#include "CAudio.h"
#include "DabConstants.h"

/**
  *	We use the creation function merely to set up the
  *	user interface and make the connections between the
  *	gui elements and the handling agents. All real action
  *	is embedded in actions, initiated by gui buttons
  */
RadioInterface::RadioInterface(CVirtualInput *Device, CDABParams& DABParams, QObject *parent): QObject(parent)
{
    QSettings Settings;

    //	Before printing anything, we set
    setlocale(LC_ALL, "");

    coarseCorrector = 0;
    running = false;
    scanMode = false;
    isSynced = UNSYNCED;
    isFICCRC = false;
    LastCurrentManualGain = 0;

    inputDevice = Device;
    dabModeParameters = DABParams;

    // Read channels from the settings
    Settings.beginGroup("channels");
    int channelcount = Settings.value("channelcout", 0).toInt();
    for (int i = 1; i <= channelcount; i++) {
        QStringList SaveChannel = Settings.value("channel/" + QString::number(i)).toStringList();
        stationList.append(SaveChannel.first(), SaveChannel.last());
    }
    Settings.endGroup();
    stationList.sort();

    p_stationModel = QVariant::fromValue(stationList.getList());
    emit stationModelChanged();

    // Add image provider for the MOT slide show
    MOTImage = new MOTImageProvider;

    //	the name of the device is passed on from the main program
    m_deviceName = inputDevice->getName();
    m_gainCount = inputDevice->getGainCount();

    /**
    *	With this GUI there is no choice for the output channel,
    *	It is the soundcard, so just allocate it
    */
    AudioBuffer = new RingBuffer<int16_t>(2 * 32768);
    Audio = new CAudio(AudioBuffer);

    /**
    *	The actual work is done elsewhere: in ofdmProcessor
    *	and ofdmDecoder for the ofdm related part, ficHandler
    *	for the FIC's and mscHandler for the MSC.
    *	The ficHandler shares information with the mscHandler
    *	but the handlers do not change each others modes.
    */
    my_mscHandler = new mscHandler(this,
        &dabModeParameters,
        AudioBuffer,
        false);
    my_ficHandler = new ficHandler(this);

    /**
    *	The default for the ofdmProcessor depends on
    *	the input device, note that in this setup the
    *	device is selected on start up and cannot be changed.
    */

    my_ofdmProcessor = new ofdmProcessor(inputDevice,
        &dabModeParameters,
        this,
        my_mscHandler,
        my_ficHandler,
        3,
        3);

    //	Set timer to check the FIC CRC
    connect(&CheckFICTimer, SIGNAL(timeout(void)), this, SLOT(CheckFICTimerTimeout(void)));
    connect(&StationTimer, SIGNAL(timeout(void)), this, SLOT(StationTimerTimeout(void)));
    CurrentFrameErrors = -1;

    spectrum_fft_handler = new common_fft(dabModeParameters.T_u);
}

RadioInterface::~RadioInterface()
{
    qDebug() << "GUI:" <<  "deleting radioInterface";
}
/**
 * \brief returns the licenses for all the relative libraries plus application version information
 */
const QVariantMap RadioInterface::licenses()
{
    QVariantMap ret;
    // Set application version
    QString InfoText;
    InfoText += "welle.io version: " + QString(CURRENT_VERSION);
    InfoText += "Build on: " + QString(__TIMESTAMP__);
    ret.insert("version", InfoText);

    // Read graph license
    QFile File(":/NOTICE.txt");
    File.open(QFile::ReadOnly);
    QByteArray FileContent = File.readAll();

    // Set graph license content
    ret.insert("graphLicense", FileContent);

    // Read license
    QFile File2(":/LICENSE.txt");
    File2.open(QFile::ReadOnly);
    QByteArray FileContent2 = File2.readAll();

    // Set license content
    ret.insert("license", FileContent2);
    return ret;
}

/**
  *	\brief At the end, we might save some GUI values
  *	The QSettings could have been the class variable as well
  *	as the parameter
  */
void RadioInterface::saveSettings()
{
    QSettings Settings;

    //	Remove channels from previous invocation ...
    Settings.beginGroup("channels");
    int ChannelCount = Settings.value("channelcout").toInt();
    for (int i = 1; i <= ChannelCount; i++)
        Settings.remove("channel/" + QString::number(i));

    //	... and save the current set
    ChannelCount = stationList.count();
    Settings.setValue("channelcout",
        QString::number(ChannelCount));

    for (int i = 1; i <= ChannelCount; i++)
        Settings.setValue("channel/" + QString::number(i),
            stationList.getStationAt(i - 1));
    Settings.endGroup();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//	The public slots are called from other places within the dab software
//	so please provide some implementation, perhaps an empty one
//
//	a slot called by the ofdmprocessor
void RadioInterface::set_fineCorrectorDisplay(int v)
{
    fineCorrector = v;
    emit displayFreqCorr(coarseCorrector + v);
}

//	a slot called by the ofdmprocessor
void RadioInterface::set_coarseCorrectorDisplay(int v)
{
    coarseCorrector = v * kHz(1);
    emit displayFreqCorr(coarseCorrector + fineCorrector);
}
/**
  *	clearEnsemble
  *	on changing settings, we clear all things in the gui
  *	related to the ensemble.
  *	The function is called from "deep" within the handling code
  *	Potentially a dangerous approach, since the fic handler
  *	might run in a separate thread and generate data to be displayed
  */
void RadioInterface::clearEnsemble(void)
{
    //
    //	it obviously means: stop processing
    my_mscHandler->stopProcessing();
    my_ficHandler->clearEnsemble();
    my_ofdmProcessor->coarseCorrectorOn();
    my_ofdmProcessor->reset();
}

//
//	a slot, called by the fic/fib handlers
void RadioInterface::addtoEnsemble(const QString& s)
{
    //	Add new station into list
    if (!s.contains("data") && !stationList.contains(s)) {
        stationList.append(s, currentChannel);

        //fprintf (stderr,"Found station %s\n", s.toStdString().c_str());
        emit foundChannelCount(stationList.count());
    }
}

//
///	a slot, called by the fib processor
void RadioInterface::nameofEnsemble(int id, const QString& v)
{
    (void)id;
    (void)v;
    my_ofdmProcessor->coarseCorrectorOff();
}

void RadioInterface::show_frameErrors(int s)
{
    CurrentFrameErrors = s;

    // Activate a timer to reset the frequency sychronisation if the FIC CRC is constant false
    if ((CurrentFrameErrors != 0) && (!StationTimer.isActive()))
        StationTimer.start(10000); // 10 s

    emit displayFrameErrors(s);
}

void RadioInterface::show_rsErrors(int s)
{
    emit displayRSErrors(s);
}

void RadioInterface::show_aacErrors(int s)
{
    emit displayAACErrors(s);
}

///	called from the ofdmDecoder, which computes this for each frame
void RadioInterface::show_snr(int s)
{
    emit signalPower(s);
}

///	just switch a color, obviously GUI dependent, but called
//	from the ofdmprocessor
void RadioInterface::setSynced(char b)
{
    isSynced = b;
    switch (isSynced) {
    case SYNCED:
        emit syncFlag(true);
        break;

    default:
        emit syncFlag(false);
        break;
    }
}

//	showLabel is triggered by the message handler
//	the GUI may decide to ignore this
void RadioInterface::showLabel(QString s)
{
    emit stationText("");
    //	The horizontal text alignment is not working if
    //	the text is not reset. Maybe this is a bug in QT
    emit stationText(s);
}
//
//	showMOT is triggered by the MOT handler,
void RadioInterface::showMOT(QByteArray data, int subtype, QString s)
{
    (void)data;
    (void)subtype;
    (void)s;

    QPixmap p(320, 240);
    p.loadFromData(data, subtype == 0 ? "GIF" : subtype == 1 ? "JPEG" : subtype == 2 ? "BMP" : "PNG");

    MOTImage->setPixmap(p);
    emit motChanged();
}

//
//	sendDatagram is triggered by the ip handler, just ignore
void RadioInterface::sendDatagram(char* data, int length)
{
    (void)data;
    (void)length;
}

/**
  *	\brief changeinConfiguration
  *	No idea yet what to do, so just give up
  *	with what we were doing. The user will -eventually -
  *	see the new configuration from which he can select
  */
void RadioInterface::changeinConfiguration(void)
{
    if (running) {
        Audio->stop();
        inputDevice->stop();
        inputDevice->reset();
        running = false;
    }
}
//
//	The audio is sent back from the audio decoder to the GUI
//	The gui will sent it to the appropriate soundhandler,
//	which for this GUI is the soundcard
//	Note the - when shutting down - some signals might
//	still wait for handling
void RadioInterface::newAudio(int rate)
{
    if (running)
        Audio->setRate(rate);
}

//	if so configured, the function might be triggered
//	from the message decoding software. The GUI
//	might decide to ignore the data sent
void RadioInterface::show_mscErrors(int er)
{
    emit displayMSCErrors(er);
    qDebug() << "GUI:" <<  "displayMSCErrors:" << er;
}
//
//	a slot, called by the iphandler
void RadioInterface::show_ipErrors(int er)
{
    (void)er;
}
//
//	These are signals, not appearing in the other GUI's
void RadioInterface::setStereo(bool isStereo)
{
    if (isStereo)
        emit audioType("Stereo");
    else
        emit audioType("Mono");
}
//
//
void RadioInterface::startChannelScanClick(void)
{
    //
    //	if running: stop the input
    inputDevice->stop();
    //	Clear old channels
    stationList.reset();
    //	start the radio
    CurrentChannelScanIndex = 0;
    currentChannel = CDABConstants::getChannelNameAtIndex(CurrentChannelScanIndex);
    set_channelSelect(currentChannel);
    emit currentStation(currentChannel);
    emit foundChannelCount(0);
    setStart();
    my_ofdmProcessor->reset();
    my_ofdmProcessor->set_scanMode(true, currentChannel);
    scanMode = true;
}

void RadioInterface::stopChannelScanClick(void)
{
    //	Stop channel scan
    my_ofdmProcessor->set_scanMode(false, currentChannel);
    ScanChannelTimer.stop();
    scanMode = false;

    emit currentStation("No Station");

    //	Sort stations
    stationList.sort();
    p_stationModel = QVariant::fromValue(stationList.getList());
    emit stationModelChanged();
}

QString RadioInterface::nextChannel(QString currentChannel)
{
    CurrentChannelScanIndex++;

    return currentChannel = CDABConstants::getChannelNameAtIndex(CurrentChannelScanIndex);
}

//
//	The ofdm processor is "conditioned" to send one signal
//	per "scanning tour". This signal is either "false"
//	if we are pretty certain that the channel does not contain
//	a signal, or "true" if there is a fair chance that the
//	channel contains useful data
void RadioInterface::setSignalPresent(bool isSignal)
{
    if (isSignal) // may be a channel, give it time
    {
        connect(&ScanChannelTimer, SIGNAL(timeout(void)),
            this, SLOT(end_of_waiting_for_stations(void)));
        ScanChannelTimer.start(10000);
        return;
    }
    currentChannel = nextChannel(currentChannel);
    if (currentChannel == QString("")) {
        emit channelScanStopped();
        emit currentStation("No Station");
        //	Sort stations
        stationList.sort();

        p_stationModel = QVariant::fromValue(stationList.getList());
        emit stationModelChanged();
        return;
    }
    set_channelSelect(currentChannel);
    emit currentStation(currentChannel);
    my_ofdmProcessor->reset();
    my_ofdmProcessor->set_scanMode(true, currentChannel);
}

void RadioInterface::show_ficSuccess(bool b)
{
    isFICCRC = b;

    emit ficFlag(b);
}

void RadioInterface::end_of_waiting_for_stations(void)
{
    disconnect(&ScanChannelTimer, SIGNAL(timeout(void)),
        this, SLOT(end_of_waiting_for_stations(void)));
    ScanChannelTimer.stop();
    currentChannel = nextChannel(currentChannel);
    if (currentChannel == QString("")) {
        emit channelScanStopped();
        emit currentStation("No Station");
        //	Sort stations
        stationList.sort();
        p_stationModel = QVariant::fromValue(stationList.getList());
        emit stationModelChanged();
        return;
    }
    set_channelSelect(currentChannel);
    emit currentStation(currentChannel);
    my_ofdmProcessor->reset();
    my_ofdmProcessor->set_scanMode(true, currentChannel);
}

void RadioInterface::displayDateTime(int* DateTime)
{
    int Year = DateTime[0];
    int Month = DateTime[1];
    int Day = DateTime[2];
    int Hour = DateTime[3];
    int Minute = DateTime[4];
    //int Seconds	= DateTime [5];
    int HourOffset = DateTime[6];
    int MinuteOffset = DateTime[7];

    emit newDateTime(Year, Month, Day,
        Hour + HourOffset, Minute + MinuteOffset);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//	Private slots relate to the modeling of the GUI
//
/**
  *	\brief setStart is a function that is called after pushing
  *	the start button.
  *	if "autoStart" == true, then the initializer will start
  *
  */
void RadioInterface::setStart(void)
{
    bool r = 0;
    if (running) // only listen when not running yet
        return;
    //
    r = inputDevice->restart();
    qDebug() << "GUI:" << "Starting" << r;
    if (!r) {
        qDebug() << "GUI:" << "Opening input stream failed";
        return;
    }
    //
    //	Of course, starting the machine will generate a new instance
    //	of the ensemble, so the listing - if any - should be cleared
    clearEnsemble(); // the display

    running = true;
}

/**
  *	\brief terminateProcess
  *	Pretty critical, since there are many threads involved
  *	A clean termination is what is needed, regardless of the GUI
  */
void RadioInterface::terminateProcess(void)
{
    running = false;
    inputDevice->stop(); // might be concurrent
    my_mscHandler->stopHandler(); // might be concurrent
    my_ofdmProcessor->stop(); // definitely concurrent
    Audio->stop();
    //
    //	everything should be halted by now
    saveSettings();
    delete my_ofdmProcessor;
    delete my_ficHandler;
    delete my_mscHandler;
    delete Audio;
    Audio = NULL; // signals may be pending, so careful
    qDebug() << "GUI:" <<  "Termination started";
    delete inputDevice;
    QApplication::quit();
}

//
/**
  *	\brief set_channelSelect
  *	Depending on the GUI the user might select a channel
  *	or some magic will cause a channel to be selected
  */
void RadioInterface::set_channelSelect(QString s)
{
    bool localRunning = running;

    // Reset timeout to reset the tuner
    StationTimer.start(10000);
    CurrentFrameErrors = -1;

    if (localRunning) {
        clearEnsemble();
        Audio->stop();
        inputDevice->stop();
        inputDevice->reset();
    }

    tunedFrequency = KHz(CDABConstants::getFrequency(s));

    if (tunedFrequency == 0)
        return;

    inputDevice->setFrequency(tunedFrequency);

    if (localRunning) {
        inputDevice->restart();
        my_ofdmProcessor->reset();
        running = true;
    }
    qDebug() << "GUI:" <<  s << "->" << tunedFrequency;
    emit displayCurrentChannel(s, tunedFrequency);
}

void RadioInterface::updateTimeDisplay(void)
{
}

void RadioInterface::autoCorrector_on(void)
{
    //	first the real stuff
    my_ficHandler->clearEnsemble();
    my_ofdmProcessor->coarseCorrectorOn();
    my_ofdmProcessor->reset();
}

void RadioInterface::CheckFICTimerTimeout(void)
{
    if (!isFICCRC)
        return;
    //
    //	for now: we handle only audio services
    if (my_ficHandler->kindofService(CurrentStation) != AUDIO_SERVICE)
        return;

    //	Tune to station
    audiodata d;
    CheckFICTimer.stop(); // stop timer
    emit currentStation(CurrentStation.simplified());
    my_ficHandler->dataforAudioService(CurrentStation, &d);
    my_mscHandler->set_audioChannel(&d);
    showLabel(QString(" "));
    stationType(CDABConstants::getProgramTypeName(d.programType));
    languageType(CDABConstants::getLanguageName(d.language));
    bitrate(d.bitRate);
    if (d.ASCTy == 077)
        emit dabType("DAB+");
    else
        emit dabType("DAB");
}

void RadioInterface::StationTimerTimeout(void)
{
    StationTimer.stop();

    // Reset if frame success rate is below 50 %
    if (CurrentFrameErrors > 3 && !scanMode) {
        qDebug() << "GUI:" <<  "Resetting tuner ...";

        // Reset current channel to force channelClick to do a new turn
        // This is very ugly but its working
        QString tmpCurrentChannel = currentChannel;
        currentChannel = "";

        // Tune to channel
        channelClick(CurrentStation, tmpCurrentChannel);
    }
}

void RadioInterface::channelClick(QString StationName,
    QString ChannelName)
{
    setStart();
    if (ChannelName != currentChannel) {
        set_channelSelect(ChannelName);
        currentChannel = ChannelName;
        emit syncFlag(false); // Clear flags
    }

    CurrentStation = StationName;

    //	Start the checking of the FIC CRC.
    //	If the FIC CRC is ok we can tune to the channel
    CheckFICTimer.start(1000);
    emit currentStation("Tuning ...");
    emit stationText("");

    // Clear MOT slide show
    QPixmap p(320, 240);
    p.fill(Qt::transparent);
    MOTImage->setPixmap(p);
    emit motChanged();

    // Clear flags
    emit displayFrameErrors(0);
    emit ficFlag(false);
}

void RadioInterface::inputEnableAGCChanged(bool checked)
{
    if (inputDevice)
    {
        inputDevice->setAgc(checked);

        if (!checked)
        {
            inputDevice->setGain(LastCurrentManualGain);
            qDebug() << "GUI:" << "AGC off";
        }
        else
        {
            qDebug() << "GUI:" <<  "AGC on";
        }

    }
}

void RadioInterface::inputGainChanged(double gain)
{
    if (inputDevice)
    {
        LastCurrentManualGain = (int)gain;
        m_currentGainValue = inputDevice->setGain(LastCurrentManualGain);
        currentGainValueChanged();
    }
}

// This function is called by the QML GUI
void RadioInterface::updateSpectrum(QAbstractSeries* series)
{
    int Samples = 0;

    if (series == NULL)
        return;

    QXYSeries* xySeries = static_cast<QXYSeries*>(series);

    //	Delete old data
    spectrum_data.clear();

    qreal tunedFrequency_MHz = tunedFrequency / 1e6;
    qreal sampleFrequency_MHz = 2048000 / 1e6;
    qreal dip_MHz = sampleFrequency_MHz / dabModeParameters.T_u;

    qreal x(0);
    qreal y(0);
    qreal y_max(0);

    // Get FFT buffer
    DSPCOMPLEX* spectrumBuffer = spectrum_fft_handler->getVector();

    // Get samples
    if (inputDevice)
        Samples = inputDevice->getSpectrumSamples(spectrumBuffer, dabModeParameters.T_u);

    // Continue only if we got data
    if (Samples <= 0)
        return;

    // Do FFT to get the spectrum
    spectrum_fft_handler->do_FFT();

    //	Process samples one by one
    for (int i = 0; i < dabModeParameters.T_u; i++) {
        int half_Tu = dabModeParameters.T_u / 2;

        //	Shift FFT samples
        if (i < half_Tu)
            y = abs(spectrumBuffer[i + half_Tu]);
        else
            y = abs(spectrumBuffer[i - half_Tu]);

        //	Find maximum value to scale the plotter
        if (y > y_max)
            y_max = y;

        x = (i * dip_MHz) + (tunedFrequency_MHz - (sampleFrequency_MHz / 2));
        spectrum_data.append(QPointF(x, y));
    }

    //	Set maximum of y-axis
    y_max = round(y_max) + 1;
    if (y_max > 0.0001)
        emit setYAxisMax(y_max);

    // Set x-axis min and max
    emit setXAxisMinMax(tunedFrequency_MHz - (sampleFrequency_MHz / 2), tunedFrequency_MHz + (sampleFrequency_MHz / 2));

    //	Set new data
    xySeries->replace(spectrum_data);
}

void RadioInterface::setErrorMessage(QString ErrorMessage)
{
    // Print only if we tune into a channel
    if (currentChannel != QString(""))
        emit showErrorMessage(ErrorMessage);
}
