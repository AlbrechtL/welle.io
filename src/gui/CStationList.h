/******************************************************************************\
 * Copyright (c) 2016 Albrecht Lohofener <albrechtloh@gmx.de>
 *
 * Author(s):
 * Albrecht Lohofener
 *
 * Description:
 * The classes provides a list to store the station and channel information
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#ifndef STATIONLIST_H
#define STATIONLIST_H

#include <QDataStream>
#include <QObject>
#include <QVariant>

class StationElement : public QObject {
Q_OBJECT

    Q_PROPERTY(QString stationName MEMBER mStationName NOTIFY stationNameChanged)
    Q_PROPERTY(QString channelName MEMBER mChannelName NOTIFY channelNameChanged)

public:
    explicit StationElement (QString stationName, QString channelName,
                             QObject *parent = 0);
    explicit StationElement (QObject *parent = 0);
    virtual ~StationElement ();

    QString getStationName(void);
    QString getChannelName(void);

    friend QDataStream& operator<<(QDataStream &out, StationElement* const& object);
    friend QDataStream& operator>>(QDataStream &in, StationElement*& object);

private:
    QString mStationName;
    QString mChannelName;

signals:
    void stationNameChanged();
    void channelNameChanged();
};

class CStationList
{
public:
    CStationList(QString settingsGroup = "channel");
    ~CStationList(void);

    void reset(void);
    void sort(void);
    int count(void);
    StationElement* at(int i);
    QStringList getStationAt(int i);
    StationElement* find(QString StationName, QString ChannelName);
    bool contains(QString StationName, QString ChannelName);
    void append(QString StationName, QString ChannelName);
    bool remove(QString StationName, QString ChannelName);
    QList<StationElement*> getList(void) const;
    void loadStations();
    void saveStations();

private:
    QString mSettingsGroup;
    QList<StationElement*> mStationList;
};

#endif // STATIONLIST_H
