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
#include "CGUI.h"
#include "CAudio.h"
#include "DabConstants.h"
#include "msc-handler.h"

// Fallback if git hash macro is not defined
#ifndef GITHASH
#pragma message "Git hash is not defined! Set it to \"unknown\""
#define GITHASH "unknown"
#endif

/**
  *	We use the creation function merely to set up the
  *	user interface and make the connections between the
  *	gui elements and the handling agents. All real action
  *	is embedded in actions, initiated by gui buttons
  */
CGUI::CGUI(CRadioController *RadioController, CDABParams *DABParams, QObject *parent): QObject(parent)
{
    this->RadioController = RadioController;
    this->DABParams = DABParams;

    // Read channels from the settings
    stationList.loadStations();
    stationList.sort();

    if(stationList.count() == 0)
        stationList.append("", tr("Station list is empty"), "");

    p_stationModel = QVariant::fromValue(stationList.getList());
    emit stationModelChanged();

    // Add image provider for the MOT slide show
    MOTImage = new CMOTImageProvider;

    spectrum_fft_handler = new common_fft(DABParams->T_u);

    connect(RadioController, SIGNAL(MOTChanged(QPixmap)), this, SLOT(MOTUpdate(QPixmap)));
    connect(RadioController, SIGNAL(FoundStation(QString, QString, QString)), this, SLOT(AddToStationList(QString, QString, QString)));
    connect(RadioController, SIGNAL(ScanStopped()), this, SIGNAL(channelScanStopped()));
    connect(RadioController, SIGNAL(ScanProgress(int)), this, SIGNAL(channelScanProgress(int)));

    connect(&UptimeTimer, SIGNAL(timeout(void)), this, SLOT(UpdateTimerTimeout(void)));
    UptimeTimer.start(250); // 250 ms
}

CGUI::~CGUI()
{
    qDebug() << "GUI:" <<  "deleting radioInterface";
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
    InfoContent.append("COPYING\n");
    InfoContent.append("-------\n");
    File = new QFile(":/COPYING");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Set graph license content
    ret.insert("FileContent", InfoContent);

    return ret;
}

/**
  *	\brief At the end, we might save some GUI values
  *	The QSettings could have been the class variable as well
  *	as the parameter
  */
void CGUI::saveChannels()
{
    stationList.saveStations();
}

void CGUI::startChannelScanClick(void)
{
    if(RadioController)
        RadioController->StartScan();

    clearStationList();
}

void CGUI::stopChannelScanClick(void)
{
    if(RadioController)
        RadioController->StopScan();
}

void CGUI::UpdateTimerTimeout()
{
    if(RadioController)
    {
        QVariantMap GUIData = RadioController->GetGUIData();

        emit setGUIData(GUIData);
    }
}

void CGUI::MOTUpdate(QPixmap MOTImage)
{
    this->MOTImage->setPixmap(MOTImage);
    emit motChanged();
}

void CGUI::AddToStationList(QString SId, QString Station, QString CurrentChannel)
{
    //	Add new station into list
    if (!stationList.contains(SId, CurrentChannel)) {
        stationList.append(SId, Station, CurrentChannel);

        //	Sort stations
        stationList.sort();
        p_stationModel = QVariant::fromValue(stationList.getList());
        emit stationModelChanged();

        //fprintf (stderr,"Found station %s\n", s.toStdString().c_str());
        emit foundChannelCount(stationList.count());

        // Save the channels
        saveChannels();
    }
}

void CGUI::channelClick(QString StationName,
    QString ChannelName)
{
    if(RadioController && ChannelName != "")
        RadioController->Play(ChannelName, StationName);
}

void CGUI::setManualChannel(QString ChannelName)
{
    if(RadioController)
        RadioController->SetChannel(ChannelName, false);
}

void CGUI::inputEnableAGCChanged(bool checked)
{
    if(RadioController)
        RadioController->SetAGC(checked);
}

void CGUI::inputEnableHwAGCChanged(bool checked)
{
    if(RadioController)
        RadioController->SetHwAGC(checked);
}

void CGUI::inputGainChanged(double gain)
{
    if(RadioController)
    {
        m_currentGainValue = RadioController->SetGain((int) gain);
        if(m_currentGainValue >= 0)
            emit currentGainValueChanged();
    }
}

void CGUI::clearStationList()
{
    //	Clear old channels
    stationList.reset();
    saveChannels();

    p_stationModel = QVariant::fromValue(stationList.getList());
    emit stationModelChanged();
}

// This function is called by the QML GUI
void CGUI::updateSpectrum(QAbstractSeries* series)
{
    int Samples = 0;
    int16_t T_u = DABParams->T_u;

    if (series == NULL)
        return;

    QXYSeries* xySeries = static_cast<QXYSeries*>(series);

    //	Delete old data
    spectrum_data.resize(T_u);

    qreal tunedFrequency_MHz = 0;
    qreal sampleFrequency_MHz = 2048000 / 1e6;
    qreal dip_MHz = sampleFrequency_MHz / T_u;

    qreal x(0);
    qreal y(0);
    qreal y_max(0);

    // Get FFT buffer
    DSPCOMPLEX* spectrumBuffer = spectrum_fft_handler->getVector();

    // Get samples
    if (RadioController)
    {
        tunedFrequency_MHz = RadioController->GetCurrentFrequency() / 1e6;
        Samples = RadioController->GetSpectrumSamples(spectrumBuffer, T_u);
    }

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

    //	Set maximum of y-axis
    y_max = round(y_max) + 1;
    if (y_max > 0.0001)
        emit setYAxisMax(y_max);

    // Set x-axis min and max
    emit setXAxisMinMax(tunedFrequency_MHz - (sampleFrequency_MHz / 2), tunedFrequency_MHz + (sampleFrequency_MHz / 2));

    //	Set new data
    xySeries->replace(spectrum_data);
}
