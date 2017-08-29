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
#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#include <QtAndroid>
#include <QAndroidJniEnvironment>
#include <QDesktopServices>

CGUI::CGUI(CRadioControllerReplica *RadioController, QObject *parent)
#else
CGUI::CGUI(CRadioController *RadioController, QObject *parent)
#endif
    : QObject(parent)
    , spectrum_series(NULL)
{
    this->RadioController = RadioController;

    QList<StationElement*> stations = RadioController->Stations();
    QList<QObject*> *stationList = reinterpret_cast<QList<QObject*>*>(&stations);
    p_stationModel = QVariant::fromValue(*stationList);
    emit stationModelChanged();

    // Add image provider for the MOT slide show
    MOTImage = new CMOTImageProvider;

#ifdef Q_OS_ANDROID
    connect(RadioController, &CRadioControllerReplica::GUIDataChanged, this, &CGUI::GUIDataUpdate);
    connect(RadioController, &CRadioControllerReplica::MOTChanged, this, &CGUI::MOTUpdate);
    connect(RadioController, &CRadioControllerReplica::SpectrumUpdated, this, &CGUI::SpectrumUpdate);
    connect(RadioController, &CRadioControllerReplica::StationsChanged, this, &CGUI::StationsChange);
    connect(RadioController, &CRadioControllerReplica::ScanStopped, this, &CGUI::channelScanStopped);
    connect(RadioController, &CRadioControllerReplica::ScanProgress, this, &CGUI::channelScanProgress);

    connect(this, &CGUI::onSuccess, RadioController, &CRadioControllerReplica::onEventLoopStarted);
    connect(this, &CGUI::onError, RadioController, &CRadioControllerReplica::setErrorMessage);
#else
    connect(RadioController, &CRadioController::GUIDataChanged, this, &CGUI::GUIDataUpdate);
    connect(RadioController, &CRadioController::MOTChanged, this, &CGUI::MOTUpdate);
    connect(RadioController, &CRadioController::SpectrumUpdated, this, &CGUI::SpectrumUpdate);
    connect(RadioController, &CRadioController::StationsChanged, this, &CGUI::StationsChange);
    connect(RadioController, &CRadioController::ScanStopped, this, &CGUI::channelScanStopped);
    connect(RadioController, &CRadioController::ScanProgress, this, &CGUI::channelScanProgress);
#endif

    QVariantMap GUIData = RadioController->GUIData();
    bool ok;
    int dabStatus = GUIData.value("Status").toInt(&ok);
    if (!ok || dabStatus <= 0) {
        qDebug() << "CGUI:" <<  "Open default device";
        openDefaultDevice();
    } else {
        qDebug() << "CGUI:" <<  "Device already open";
    }
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
    InfoContent.append(tr("QT version") + ": " + qVersion() + "\n");
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

void CGUI::GUIDataUpdate(QVariantMap GUIData)
{
    //qDebug() << "CGUI:" <<  "GUIDataUpdate";
    emit setGUIData(GUIData);
}

void CGUI::MOTUpdate(QImage MOTImage)
{
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

void CGUI::channelClick(QString StationName, QString ChannelName)
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
        RadioController->setAGC(checked);
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
        m_currentGainValue = RadioController->GainValue();
        if(m_currentGainValue >= 0)
            emit currentGainValueChanged();
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
    spectrum_series = static_cast<QXYSeries*>(series);
}

// This function is called by the QML GUI
void CGUI::updateSpectrum()
{
    if (RadioController && spectrum_series)
        RadioController->UpdateSpectrum();
}

void CGUI::SpectrumUpdate(qreal Ymax, qreal Xmin, qreal Xmax, QVector<QPointF> Data)
{
    // Set maximum of y-axis
    if (Ymax > 0.0001)
        emit setYAxisMax(Ymax);

    // Set x-axis min and max
    emit setXAxisMinMax(Xmin, Xmax);

    // Set new data
    if (spectrum_series)
        spectrum_series->replace(Data);
}

#ifndef Q_OS_ANDROID

void CGUI::openDefaultDevice()
{
    // The timer is used to signal if the QT event lopp is running
    QTimer::singleShot(0, RadioController, SLOT(onEventLoopStarted()));
}

#else

void CGUI::openDefaultDevice()
{
    // Start Android rtl_tcp
    QAndroidJniObject path = QAndroidJniObject::fromString("iqsrc://-a 127.0.0.1 -p 1234 -s 2048000");

    QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
                "android/net/Uri",
                "parse",
                "(Ljava/lang/String;)Landroid/net/Uri;", path.object<jstring>());

    QAndroidJniObject ACTION_VIEW = QAndroidJniObject::getStaticObjectField<jstring>(
                "android/content/Intent",
                "ACTION_VIEW");

    QAndroidJniObject intent(
                "android/content/Intent",
                "(Ljava/lang/String;)V",
                ACTION_VIEW.object<jstring>());

    QAndroidJniObject result = intent.callObjectMethod(
                "setData",
                "(Landroid/net/Uri;)Landroid/content/Intent;",
                uri.object<jobject>());

    QtAndroid::startActivity(intent, 1234, this);

    // Catch exception
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
}

#define RESULT_OK -1

void CGUI::handleActivityResult(int receiverRequestCode, int resultCode,
                                const QAndroidJniObject &data)
{
    if(receiverRequestCode == 1234)
    {
        if(resultCode == RESULT_OK)
        {
            qDebug() << "Android RTL_SDR: Successfully opened";
            emit onSuccess();
        }
        else
        {
            QAndroidJniObject MessageType = QAndroidJniObject::fromString("detailed_exception_message");
            QString Message;

            if(data.isValid())
            {
                QAndroidJniObject result = data.callObjectMethod(
                            "getStringExtra",
                            "(Ljava/lang/String;)Ljava/lang/String;",
                            MessageType.object<jstring>());

                Message = result.toString();
            }
            else
            {
                // We assume here that the Android RTL-SDR driver is not installed
                Message = CGUI::tr("Android RTL-SDR driver is not installed");

                emit showAndroidInstallDialog(Message, CGUI::tr("Do you would like to install it? After install start welle.io again."));
            }

            qDebug().noquote() << "Android RTL_SDR:" << Message;
            emit onError(Message);
        }
    }
}

#endif
