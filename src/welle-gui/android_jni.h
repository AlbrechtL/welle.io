/*
 *    Copyright (C) 2017
 *    Matthias Benesch (twoof7@gmail.com)
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

#ifndef _ANDROID_JNI_
#define _ANDROID_JNI_

#include <QTimer>

#include "radio_controller.h"
#include "CStationList.h"

class CAndroidJNI : public QObject {
    Q_OBJECT

public:
    static CAndroidJNI& getInstance();
    ~CAndroidJNI();

    Q_INVOKABLE bool openTcpConnection(QString host, int port);
    Q_INVOKABLE void closeTcpConnection();

    Q_INVOKABLE bool isFavoriteStation(QString station, QString channel);
    Q_INVOKABLE void setFavoriteStation(QString station, QString channel,
                                        bool value);

    Q_INVOKABLE void play(QString station, QString channel);

    Q_INVOKABLE void duckPlayback(bool duck);
    Q_INVOKABLE void pausePlayback();
    Q_INVOKABLE void stopPlayback();

    Q_INVOKABLE void nextChannel();
    Q_INVOKABLE void setManualChannel(QString channel);

    Q_INVOKABLE void startChannelScan(void);
    Q_INVOKABLE void stopChannelScan(void);

    Q_INVOKABLE void setErrorMessage(QString text);

    static QString getLastStation(void);

    void setRadioController(CRadioController *radioController);

private:
    CAndroidJNI(QObject* parent = NULL);
    CRadioController *mRadioController;
    CStationList mFavoriteList;
    void addStation(QString station, QString channel);
    void addFavoriteStation(QString station, QString channel);
    void removeFavoriteStation(QString station, QString channel);
    void clearFavoriteStations();

private slots:
    void serviceReady(void);
    void deviceReady(void);
    void deviceClosed(void);
    void updateGuiData(QVariantMap GUIData);
    void updateMOT(QImage img);
    void clearStations(void);
    void foundStation(QString station, QString channel);
    void channelScanStopped(void);
    void channelScanProgress(int Progress);
    void showErrorMessage(QString Text);
    void showInfoMessage(QString Text);
};

#endif
