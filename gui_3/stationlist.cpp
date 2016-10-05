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

#include "stationlist.h"

StationElement::StationElement(QString const stationName, QString const channelName, QObject *parent) : QObject(parent)
{
    setProperty("stationName",stationName);
    setProperty("channelName",channelName);
}

QString StationElement::getStationName(void)
{
    return m_stationName;
}

QString StationElement::getChannelName(void)
{
    return m_channelName;
}

StationList::StationList(void)
{
    stationList.clear();
}

StationList::~StationList(void)
{
}

void StationList::reset(void)
{
    stationList.clear();
}

bool variantLessThan(const QObject* v1, const QObject* v2)
 {
     return ((StationElement*) v1)->getStationName() < ((StationElement*) v2)->getStationName();
 }

void StationList::sort(void)
{
    qSort(stationList.begin(), stationList.end(), variantLessThan);
}

int StationList::count(void)
{
    return stationList.count();
}

StationElement* StationList::at(int i)
{
    return (StationElement*) stationList.at(i);
}

QStringList StationList::getStationAt(int i)
{
    QString StationName = at(i)->getStationName();
    QString ChannelName = at(i)->getChannelName();

    QStringList StationElement;

    StationElement.append(StationName);
    StationElement.append(ChannelName);

    return StationElement;
}

bool StationList::contains(QString value)
{
    bool result = false;

    for(int i=0;i<count();i++)
    {
        if(at(i)->getStationName() == value)
            result = true;
    }

    return result;
}

void StationList::append(QString StationName, QString ChannelName)
{
    stationList.append(new StationElement(StationName, ChannelName));
}

QList<QObject*>  StationList::getList(void)
{
    return  stationList;
}
