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

#include "CDebugOutput.h"
#include "CInputFactory.h"
#include "CGUI.h"
#include "CAudio.h"
#include "dab-constants.h"
#include "msc-handler.h"
#include "version.h"

/**
  *	We use the creation function merely to set up the
  *	user interface and make the connections between the
  *	gui elements and the handling agents. All real action
  *	is embedded in actions, initiated by gui buttons
  */
#ifdef Q_OS_ANDROID
CGUI::CGUI(CRadioControllerReplica *RadioController, QObject *parent)
#else
CGUI::CGUI(CRadioController *RadioController, QObject *parent)
#endif
    : QObject(parent)
    , RadioController(RadioController)
    , spectrumSeries(NULL)
    , impulseResponseSeries(NULL)
{
    m_currentGainValue = 0;

    StationsChange(RadioController->Stations());

    // Add image provider for the MOT slide show
    MOTImage = new CMOTImageProvider;

#ifdef Q_OS_ANDROID
    connect(RadioController, &CRadioControllerReplica::stateChanged, this, &CGUI::stateChanged);
    connect(RadioController, &CRadioControllerReplica::DeviceClosed, this, &CGUI::DeviceClosed);
    connect(RadioController, &CRadioControllerReplica::GUIDataChanged, this, &CGUI::guiDataChanged);
    connect(RadioController, &CRadioControllerReplica::MOTChanged, this, &CGUI::MOTUpdate);
    connect(RadioController, &CRadioControllerReplica::SpectrumUpdated, this, &CGUI::SpectrumUpdate);
    connect(RadioController, &CRadioControllerReplica::StationsChanged, this, &CGUI::StationsChange);
    connect(RadioController, &CRadioControllerReplica::ScanStopped, this, &CGUI::channelScanStopped);
    connect(RadioController, &CRadioControllerReplica::ScanProgress, this, &CGUI::channelScanProgress);
#else
    connect(RadioController, &CRadioController::GUIDataChanged, this, &CGUI::guiDataChanged);
    connect(RadioController, &CRadioController::MOTChanged, this, &CGUI::MOTUpdate);
    connect(RadioController, &CRadioController::StationsChanged, this, &CGUI::StationsChange);
    connect(RadioController, &CRadioController::ScanStopped, this, &CGUI::channelScanStopped);
    connect(RadioController, &CRadioController::ScanProgress, this, &CGUI::channelScanProgress);
    connect(RadioController, &CRadioController::showErrorMessage, this, &CGUI::showErrorMessage);
    connect(RadioController, &CRadioController::showInfoMessage, this, &CGUI::showInfoMessage);
#endif

#ifndef QT_NO_SYSTEMTRAYICON
    minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SIGNAL(minimizeWindow()));

    maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(maximizeAction, SIGNAL(triggered()), this, SIGNAL(maximizeWindow()));

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SIGNAL(restoreWindow()));

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    trayIconMenu = new QMenu();
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon();
    trayIcon->setContextMenu(trayIconMenu);

    trayIcon->setIcon(QIcon(":/icon.png"));
    trayIcon->show();
#endif

    CDebugOutput::setCGUI(this);
}

CGUI::~CGUI()
{
    qDebug() << "GUI:" <<  "deleting radioInterface";
}

void CGUI::close()
{
#ifdef Q_OS_ANDROID
    if (RadioController) {
        qDebug() << "GUI:" <<  "close device";
        disconnect(RadioController, &CRadioControllerReplica::DeviceClosed, this, &CGUI::close);
        RadioController->closeDevice();
        return;
    }
#endif
    qDebug() << "GUI:" <<  "close application";
    QApplication::quit();
}

#ifdef Q_OS_ANDROID
void CGUI::stateChanged(QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState)
{
    qDebug() << "GUI:" <<  "state changed from:" << oldState << "" << state;
    if (state == QRemoteObjectReplica::Suspect) {
        qDebug() << "GUI:" <<  "closing application";
        QApplication::quit();
    }
}
#endif

void CGUI::DeviceClosed()
{
#ifdef Q_OS_ANDROID
    qDebug() << "GUI:" <<  "device closed => closing application";
    QApplication::quit();
#endif
}

/**
 * \brief returns the licenses for all the relative libraries plus application version information
 */
const QVariantMap CGUI::licenses()
{
    QVariantMap ret;
    QFile *File;
    QByteArray InfoContent;

    // Set application version
    InfoContent.append("welle.io " + tr("version") + ": " + QString(CURRENT_VERSION) + "\n");
    InfoContent.append(tr("Git revision") + ": " + QString(GITHASH) + "\n");
    InfoContent.append(tr("Build on") + ": " + QString(__TIMESTAMP__) + "\n");
    InfoContent.append(tr("QT version") + ": " + qVersion() + "\n");
    InfoContent.append("\n");

    InfoContent.append("For legal information scroll down, please.\n");
    InfoContent.append("\n");

    // Read AUTHORS
    InfoContent.append("AUTHORS\n");
    InfoContent.append("-------\n");
    File = new QFile(":/AUTHORS");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Read THANKS
    InfoContent.append("THANKS\n");
    InfoContent.append("------\n");
    File = new QFile(":/THANKS");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Read COPYING
    InfoContent.append("COPYING (GPLv2)\n");
    InfoContent.append("-------\n");
    File = new QFile(":/COPYING");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Read COPYING
    InfoContent.append("QT COPYING (LGPL-2.1)\n");
    InfoContent.append("-------\n");
    File = new QFile(":/src/libs/COPYING.QT.LGPL-2.1");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Read COPYING
    InfoContent.append("kjmp2 COPYING (zlib)\n");
    InfoContent.append("-------\n");
    File = new QFile(":/src/libs/COPYING.kjmp2.zlib");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Read COPYING
    InfoContent.append("kiss_fft COPYING (BSD 3-clause)\n");
    InfoContent.append("-------\n");
    File = new QFile(":/src/libs/kiss_fft/COPYING");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Set graph license content
    ret.insert("FileContent", InfoContent);

    return ret;
}

void CGUI::startChannelScanClick(void)
{
    if(RadioController)
        RadioController->StartScan();
}

void CGUI::stopChannelScanClick(void)
{
    if(RadioController)
        RadioController->StopScan();
}

void CGUI::MOTUpdate(QImage MOTImage)
{
    if (MOTImage.isNull()) {
        MOTImage = QImage(320, 240, QImage::Format_Alpha8);
        MOTImage.fill(Qt::transparent);
    }
    this->MOTImage->setPixmap(QPixmap::fromImage(MOTImage));
    emit motChanged();
}

void CGUI::StationsChange(QList<StationElement*> Stations)
{
    //qDebug() << "CGUI:" <<  "StationsChange";
    QList<QObject*> *stationList = reinterpret_cast<QList<QObject*>*>(&Stations);
    p_stationModel = QVariant::fromValue(*stationList);

    emit stationModelChanged();
    emit foundChannelCount(Stations.count());
}

void CGUI::showErrorMessage(QString Text)
{
#ifndef QT_NO_SYSTEMTRAYICON
    trayIcon->showMessage(QCoreApplication::applicationName(), Text, QIcon(":/icon.png"), 5000);
#else
    (void)Text;
#endif
}

void CGUI::showInfoMessage(QString Text)
{
#ifndef QT_NO_SYSTEMTRAYICON
    trayIcon->showMessage(QCoreApplication::applicationName(), Text, QIcon(":/icon.png"), 5000);
#else
    (void)Text;
#endif
}

void CGUI::channelClick(QString StationName, QString ChannelName)
{
    if(RadioController && ChannelName != "")
        RadioController->Play(ChannelName, StationName);
}

void CGUI::setManualChannel(QString ChannelName)
{
    if(RadioController)
        RadioController->SetManualChannel(ChannelName);
}

void CGUI::inputEnableAGCChanged(bool checked)
{
    if(RadioController)
        RadioController->setAGC(checked);
}

void CGUI::inputDisableCoarseCorrector(bool checked)
{
    if(RadioController)
        RadioController->disableCoarseCorrector(checked);
}

void CGUI::inputEnableTIIDecode(bool checked)
{
    if(RadioController)
        RadioController->enableTIIDecode(checked);
}

void CGUI::inputEnableOldFFTWindowPlacement(bool checked)
{
    if(RadioController)
        RadioController->enableOldFFTWindowPlacement(checked);
}

void CGUI::inputSetFreqSyncMethod(int fsm_ix)
{
    if(RadioController)
        RadioController->setFreqSyncMethod(fsm_ix);
}

void CGUI::inputEnableHwAGCChanged(bool checked)
{
    if(RadioController)
        RadioController->setHwAGC(checked);
}

void CGUI::inputGainChanged(double gain)
{
    if(RadioController)
    {
        RadioController->setGain((int) gain);
        float currentGainValue = RadioController->GainValue();
        if(currentGainValue > std::numeric_limits<float>::lowest())
        {
            m_currentGainValue = currentGainValue;
            emit currentGainValueChanged();
        }
    }
}

void CGUI::clearStationList()
{
    if(RadioController)
    {
        RadioController->ClearStations();
    }
}

void CGUI::registerSpectrumSeries(QAbstractSeries* series)
{
    spectrumSeries = static_cast<QXYSeries*>(series);
}

void CGUI::registerImpulseResonseSeries(QAbstractSeries* series)
{
    impulseResponseSeries = static_cast<QXYSeries*>(series);
}

void CGUI::registerNullSymbolSeries(QAbstractSeries *series)
{
    nullSymbolSeries = static_cast<QXYSeries*>(series);
}

void CGUI::registerConstellationSeries(QAbstractSeries *series)
{
    constellationSeries = static_cast<QXYSeries*>(series);
}

void CGUI::tryHideWindow()
{
#ifndef QT_NO_SYSTEMTRAYICON
    trayIcon->showMessage(QCoreApplication::applicationName(), tr("The program will keep running in the "
                                       "system tray. To terminate the program, "
                                       "choose <b>Quit</b> in the context menu "
                                       "of the system tray entry."), QIcon(":/icon.png"), 5000);
    emit minimizeWindow();
#endif
}

// This function is called by the QML GUI
void CGUI::updateSpectrum()
{
    std::vector<DSPCOMPLEX> signalProbeBuffer;
    int T_u = RadioController->getDABParams().T_u;

    qreal y = 0;
    qreal x = 0;
    qreal y_max = 0;
    qreal x_min = 0;
    qreal x_max = 0;

    qreal tunedFrequency_MHz = 0;
    qreal CurrentFrequency = RadioController->getCurrentFrequency();
    qreal sampleFrequency_MHz = INPUT_RATE / 1e6;
    qreal dip_MHz = sampleFrequency_MHz / T_u;

    signalProbeBuffer = RadioController->getSignalProbe();

    if (signalProbeBuffer.size() == (size_t)T_u) {
        spectrumSeriesData.resize(T_u);

        fft::Forward FFT(T_u);
        DSPCOMPLEX* spectrumBuffer = FFT.getVector();

        std::copy(signalProbeBuffer.begin(), signalProbeBuffer.begin() + T_u,
                spectrumBuffer);

        // Do FFT to get the spectrum
        FFT.do_FFT();

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
            qreal CMA = spectrumSeriesData[i].y();
            y = (CMA * avg + y) / (avg + 1);

            // Find maximum value to scale the plotter
            if (y > y_max)
                y_max = y;

            // Calc x frequency
            x = (i * dip_MHz) + (tunedFrequency_MHz - (sampleFrequency_MHz / 2));

            spectrumSeriesData[i]= QPointF(x, y);
        }

        x_min = tunedFrequency_MHz - (sampleFrequency_MHz / 2);
        x_max = tunedFrequency_MHz + (sampleFrequency_MHz / 2);

        emit setSpectrumAxis(y_max, x_min, x_max);

        if(spectrumSeries)
            spectrumSeries->replace(spectrumSeriesData);
    }
}

void CGUI::updateImpulseResponse()
{
    std::vector<float> impulseResponseBuffer;
    int T_u = RadioController->getDABParams().T_u;

    qreal y_max = 0;
    qreal x_min = 0;
    qreal x_max = 0;

    impulseResponseBuffer = RadioController->getImpulseResponse();

    if (impulseResponseBuffer.size() == (size_t)T_u) {
        impulseResponseSeriesData.resize(T_u);
        for (int i = 0; i < T_u; i++) {
            qreal y = 10.0f * std::log10(impulseResponseBuffer[i]);
            qreal x = i;

            // Find maximum value to scale the plotter
            if (y > y_max)
                y_max = y;
            impulseResponseSeriesData[i] = QPointF(x, y);
        }

        x_min = 0;
        x_max = T_u;

        emit setImpulseResponseAxis(y_max, x_min, x_max);

        if(impulseResponseSeries)
            impulseResponseSeries->replace(impulseResponseSeriesData);
    }
}

void CGUI::updateNullSymbol()
{
    std::vector<DSPCOMPLEX> nullSymbolBuffer;
    int T_u = RadioController->getDABParams().T_u;
    int T_null = RadioController->getDABParams().T_null;

    qreal y = 0;
    qreal x = 0;
    qreal y_max = 0;
    qreal x_min = 0;
    qreal x_max = 0;

    qreal tunedFrequency_MHz = 0;
    qreal CurrentFrequency = RadioController->getCurrentFrequency();
    qreal sampleFrequency_MHz = INPUT_RATE / 1e6;
    qreal dip_MHz = sampleFrequency_MHz / T_u;

    nullSymbolBuffer = RadioController->getNullSymbol();

    if (nullSymbolBuffer.size() == (size_t)T_null) {
        nullSymbolSeriesData.resize(T_u);

        fft::Forward FFT(T_u);
        DSPCOMPLEX* spectrumBuffer = FFT.getVector();

        std::copy(nullSymbolBuffer.begin(), nullSymbolBuffer.begin() + T_u,
                spectrumBuffer);

        // Do FFT to get the spectrum
        FFT.do_FFT();

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
            qreal CMA = nullSymbolSeriesData[i].y();
            y = (CMA * avg + y) / (avg + 1);

            // Find maximum value to scale the plotter
            if (y > y_max)
                y_max = y;

            // Calc x frequency
            x = (i * dip_MHz) + (tunedFrequency_MHz - (sampleFrequency_MHz / 2));

            nullSymbolSeriesData[i]= QPointF(x, y);
        }

        x_min = tunedFrequency_MHz - (sampleFrequency_MHz / 2);
        x_max = tunedFrequency_MHz + (sampleFrequency_MHz / 2);

        emit setNullSymbolAxis(y_max, x_min, x_max);

        if(nullSymbolSeries)
            nullSymbolSeries->replace(nullSymbolSeriesData);
    }
}

void CGUI::updateConstellation()
{
    std::vector<DSPCOMPLEX> constellationPointBuffer;

    qreal x_min = 0;
    qreal x_max = 0;

    constellationPointBuffer = RadioController->getConstellationPoint();

    const size_t decim = OfdmDecoder::constellationDecimation;
    const size_t num_iqpoints = (RadioController->getDABParams().L-1) * RadioController->getDABParams().K / decim;
    if (constellationPointBuffer.size() == num_iqpoints) {
        constellationSeriesData.resize(num_iqpoints);
        for (size_t i = 0; i < num_iqpoints; i++) {
            qreal y = 180.0f / (float)M_PI * std::arg(constellationPointBuffer[i]);
            constellationSeriesData[i] = QPointF(i, y);
        }

        x_min = 0;
        x_max = num_iqpoints;

        emit setConstellationAxis(x_min, x_max);

        if(constellationSeries)
            constellationSeries->replace(constellationSeriesData);
    }
//    else {
//        qDebug() << "IQ" << constellationPointBuffer.size() << num_iqpoints;
    //    }
}

void CGUI::setNewDebugOutput(QString text)
{
    emit newDebugOutput(text);
}

QTranslator* CGUI::AddTranslator(QString Language, QTranslator *OldTranslator)
{
    if(OldTranslator)
        QCoreApplication::removeTranslator(OldTranslator);

    QTranslator *Translator = new QTranslator;

    // Special handling for German
    if(Language == "de_AT" || Language ==  "de_CH" || Language ==  "de_BE" || Language ==  "de_IT" || Language ==  "de_LU")
    {
        qDebug() << "main:" <<  "Use de_DE instead of" << Language;
        Language = "de_DE";
    }

    // Special handling for French
    if(Language == "fr_BE" || Language == "fr_CA" || Language == "fr_CH" || Language == "fr_LU")
    {
        qDebug() << "main:" <<  "Use fr_FR instead of" << Language;
        Language = "fr_FR";
    }

    bool isTranslation = Translator->load(QString(":/i18n/") + Language);

    qDebug() << "main:" <<  "Set language" << Language;
    QCoreApplication::installTranslator(Translator);

    if(!isTranslation)
    {
        qDebug() << "main:" <<  "Error while loading language" << Language << "use English \"en_GB\" instead";
        Language = "en_GB";
    }

    QLocale curLocale(QLocale((const QString&)Language));
    QLocale::setDefault(curLocale);

    return Translator;
}
