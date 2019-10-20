/*
 *    Copyright (C) 2018
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

#ifndef GUIHELPER_H
#define GUIHELPER_H

#include <QQmlContext>
#include <QTimer>
#include <QQmlApplicationEngine>
#include <QtCharts>

#ifndef QT_NO_SYSTEMTRAYICON
    #include <QSystemTrayIcon>
#endif

#ifdef __ANDROID__
    #include <QAndroidJniEnvironment>
    #include <QtAndroidExtras/QAndroidJniObject>
    #include <QtAndroidExtras/QtAndroid>
    #include <QtAndroidExtras/QAndroidActivityResultReceiver>
#endif

#include "mot_image_provider.h"
#include "dab-constants.h"
#include "radio_controller.h"

#ifdef __ANDROID__
    class FileActivityResultReceiver;
#endif

/*
 *	GThe main gui object. It inherits from
 *	QDialog and the generated form
 */
class CGUIHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant licenses READ licenses CONSTANT)

public:
    Q_INVOKABLE void addTranslator(QString Language, QObject *obj);

    CGUIHelper(CRadioController *radioController, QObject* parent = nullptr);
    ~CGUIHelper();
    Q_INVOKABLE void registerSpectrumSeries(QAbstractSeries* series);
    Q_INVOKABLE void registerSpectrumWaterfall(QObject * obj);
    Q_INVOKABLE void registerImpulseResonseSeries(QAbstractSeries* series);
    Q_INVOKABLE void registerImpulseResonseWaterfall(QObject * obj);
    Q_INVOKABLE void registerNullSymbolSeries(QAbstractSeries* series);
    Q_INVOKABLE void registerNullSymbolWaterfall(QObject * obj);
    Q_INVOKABLE void registerConstellationSeries(QAbstractSeries* series);
    Q_INVOKABLE void tryHideWindow(void);
    Q_INVOKABLE void updateSpectrum();
    Q_INVOKABLE void updateImpulseResponse();
    Q_INVOKABLE void updateNullSymbol();
    Q_INVOKABLE void updateConstellation();

    Q_INVOKABLE void openAutoDevice();
    Q_INVOKABLE void openAirspy();
    Q_INVOKABLE void setBiasTeeAirspy(bool isOn);
    Q_INVOKABLE void openRtlSdr();
    Q_INVOKABLE void setBiasTeeRtlSdr(bool isOn);
    Q_INVOKABLE void openSoapySdr(QString driverArgs);
    Q_INVOKABLE void setAntennaSoapySdr(QString text);
    Q_INVOKABLE void setDriverArgsSoapySdr(QString text);
    Q_INVOKABLE void setClockSourceSoapySdr(QString text);
    Q_INVOKABLE void openRtlTcp(QString IpAddress, int IpPort, bool force);
    Q_INVOKABLE void openRawFile(QString fileFormat);
    Q_INVOKABLE void openRawFile(QString filename, QString fileFormat);    

    void setNewDebugOutput(QString text);

    CMOTImageProvider* motImage; // ToDo: Must be a getter

private:
    QTranslator *translator = nullptr;

    CRadioController *radioController;

    QXYSeries* spectrumSeries;
    QVector<QPointF> spectrumSeriesData;

    QXYSeries* impulseResponseSeries;
    QVector<QPointF> impulseResponseSeriesData;

    QXYSeries* nullSymbolSeries;
    QVector<QPointF> nullSymbolSeriesData;

    QXYSeries* constellationSeries;
    QVector<QPointF> constellationSeriesData;

    const QVariantMap licenses();

#ifndef QT_NO_SYSTEMTRAYICON
    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *quitAction;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
#endif

#ifdef __ANDROID__
    FileActivityResultReceiver *activityResultReceiver;
#endif
public slots:
    void close();

private slots:
    void deviceClosed();
    void motUpdate(QImage motImage);
    void showErrorMessage(QString Text);
    void showInfoMessage(QString Text);
    void showWindow(QSystemTrayIcon::ActivationReason r);

signals:
    void foundChannelCount(int channelCount);
    void setSpectrumAxis(qreal Ymax, qreal Xmin, qreal Xmax);
    void setImpulseResponseAxis(qreal Ymax, qreal Xmin, qreal Xmax);
    void setNullSymbolAxis(qreal Ymax, qreal Xmin, qreal Xmax);
    void setConstellationAxis(qreal Xmin, qreal Xmax);
    void motChanged(void);
    void newDebugOutput(QString text);
    void newDeviceId(int deviceId);

#ifndef QT_NO_SYSTEMTRAYICON
    void minimizeWindow(void);
    void maximizeWindow(void);
    void restoreWindow(void);
#endif
};

#ifdef __ANDROID__
class FileActivityResultReceiver : public QAndroidActivityResultReceiver
{
public:
    FileActivityResultReceiver(CGUIHelper *Client, QString fileFormat): guiHelper(Client), fileFormat(fileFormat) {}

    virtual void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &intent);

private:
    CGUIHelper *guiHelper;
    QString fileFormat;
};
#endif

#endif // GUIHELPER_H
