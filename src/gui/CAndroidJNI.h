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

#include "CRadioController.h"
#include "CStationList.h"

class CAndroidJNI : public QObject {
    Q_OBJECT

public:
    static CAndroidJNI& getInstance();
    ~CAndroidJNI();

    Q_INVOKABLE bool openUsbDevice(int fd, QString path);

    Q_INVOKABLE bool isFavoriteStation(QString stationId);
    Q_INVOKABLE void addFavoriteStation(QString stationId, QString stationName);
    Q_INVOKABLE void removeFavoriteStation(QString stationId);
    Q_INVOKABLE void saveFavoriteStations();

    Q_INVOKABLE void playStationById(QString stationId);

    Q_INVOKABLE void duckPlayback(bool duck);
    Q_INVOKABLE void pausePlayback();
    Q_INVOKABLE void stopPlayback();

    Q_INVOKABLE void nextChannel();
    Q_INVOKABLE void setManualChannel(QString channelName);

    Q_INVOKABLE void startChannelScan(void);
    Q_INVOKABLE void stopChannelScan(void);

    Q_INVOKABLE void saveStations(void);
    Q_INVOKABLE void clearStations(void);

    static QString getLastStation(void);

    void setRadioController(CRadioController *radioController);

private:
    CAndroidJNI(QObject* parent = NULL);
    CRadioController *mRadioController;
    CStationList mStationList;
    CStationList mFavoriteList;
    void addStation(QString stationId, QString stationName, QString channelName);
    StationElement* findStation(QString stationId);

private slots:
    void serviceReady(void);
    void deviceReady(void);
    void updateGuiData(QVariantMap GUIData);
    void foundStation(QString stationId, QString stationName, QString channelName);
    void channelScanStopped(void);
    void channelScanProgress(int Progress);
    void showErrorMessage(QString Text);
    void showInfoMessage(QString Text);
};

#endif
