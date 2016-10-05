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

#include <QObject>
#include <QVariant>

class StationElement : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString stationName MEMBER m_stationName NOTIFY stationNameChanged)
    Q_PROPERTY(QString channelName MEMBER m_channelName NOTIFY channelNameChanged)

public:
    explicit StationElement(QString stationName, QString channelName, QObject *parent = 0);
    QString getStationName(void);
    QString getChannelName(void);

private:
    QString m_stationName;
    QString m_channelName;

signals:
    void stationNameChanged();
    void channelNameChanged();
};

class StationList
{
public:
    StationList(void);
    ~StationList(void);

    void reset(void);
    void sort(void);
    int count(void);
    StationElement* at(int i);
    QStringList getStationAt(int i);
    bool contains(QString value);
    void append(QString StationName, QString ChannelName);
    QList<QObject*>  getList(void);

private:
    QList<QObject*> stationList;
};

#endif // STATIONLIST_H
