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

#ifndef _GUI
#define _GUI

#include <QQmlContext>
#include <QTimer>
#include <QtQml/QQmlApplicationEngine>
//#include <QList>
#include <QtCharts>
using namespace QtCharts;

#include "CRadioController.h"
#include "CMOTImageProvider.h"
#include "CStationList.h"


class common_fft;


/*
 *	GThe main gui object. It inherits from
 *	QDialog and the generated form
 */
class CGUI : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant stationModel READ stationModel NOTIFY stationModelChanged)
    Q_PROPERTY(int gainCount MEMBER m_gainCount CONSTANT)
    Q_PROPERTY(float currentGainValue MEMBER m_currentGainValue NOTIFY currentGainValueChanged)
    Q_PROPERTY(QVariant licenses READ licenses CONSTANT)

public:
    CGUI(CRadioController *RadioController, CDABParams *DABParams, QObject* parent = NULL);
    ~CGUI();
    Q_INVOKABLE void channelClick(QString, QString);
    Q_INVOKABLE void startChannelScanClick(void);
    Q_INVOKABLE void stopChannelScanClick(void);
    Q_INVOKABLE void saveChannels(void);
    Q_INVOKABLE void inputEnableAGCChanged(bool checked);
    Q_INVOKABLE void inputGainChanged(double gain);
    QVariant stationModel() const
    {
        return p_stationModel;
    }
    CMOTImageProvider* MOTImage; // ToDo: Must be a getter

private:
    CRadioController *RadioController;
    CDABParams *DABParams;
    QTimer UptimeTimer;

    common_fft* spectrum_fft_handler;
    QVector<QPointF> spectrum_data;

    const QVariantMap licenses();

    CStationList stationList;
    QVariant p_stationModel;

    int m_gainCount;
    float m_currentGainValue;

public slots:
    void updateSpectrum(QAbstractSeries* series);
    void setErrorMessage(QString ErrorMessage);

private slots:
    void UpdateTimerTimeout(void);
    void MOTUpdate(QPixmap MOTImage);
    void AddToStationList(QString Station, QString CurrentChannel);

signals:   
    void channelScanStopped(void);
    void channelScanProgress(int progress);
    void foundChannelCount(int channelCount);

    void currentGainValueChanged();

    void setYAxisMax(qreal max);
    void setXAxisMinMax(qreal min, qreal max);

    void showErrorMessage(QString Text);
    void stationModelChanged();
    void motChanged(void);

    void setGUIData(QVariantMap GUIData);
};

#endif
