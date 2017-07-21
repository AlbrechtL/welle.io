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

#include "CStationList.h"

#include <QSettings>

StationElement::StationElement(QString const stationName,
                               QString const channelName,
                               QObject *parent)
    : QObject(parent)
{
    setProperty("stationName",stationName);
    setProperty("channelName",channelName);
}

QString StationElement::getStationName(void)
{
    return mStationName;
}

QString StationElement::getChannelName(void)
{
    return mChannelName;
}

CStationList::CStationList(QString settingsGroup)
    : mSettingsGroup(settingsGroup)
{
    mStationList.clear();
}

CStationList::~CStationList (void)
{
}

void CStationList::reset(void) {
    mStationList.clear();
}

bool variantLessThan(const QObject* v1, const QObject* v2)
{
    int result = (((StationElement*) v1)->getStationName()).compare(
                ((StationElement*) v2)->getStationName(), Qt::CaseInsensitive);
    if (result < 0) {
        return true;
    } else if (result > 0) {
        return false;
    } else {
        return (((StationElement*) v1)->getChannelName()).compare(
                    ((StationElement*) v2)->getChannelName(), Qt::CaseInsensitive) < 0;
    }
}

void CStationList::sort(void)
{
    qSort(mStationList.begin(), mStationList.end(), variantLessThan);
}

int CStationList::count(void)
{
    return mStationList.count();
}

StationElement* CStationList::at(int i)
{
    return (StationElement*) mStationList.at(i);
}

QStringList CStationList::getStationAt(int i)
{
    QString StationName = at(i)->getStationName();
    QString ChannelName = at(i)->getChannelName();
    QStringList StationElement;

    StationElement.append(StationName);
    StationElement.append(ChannelName);
    return StationElement;
}

StationElement* CStationList::find(QString StationName, QString ChannelName)
{
    if (StationName.isNull())
        return 0;

    foreach (QObject *obj, mStationList) {
        StationElement *station = (StationElement*)obj;
        if (station->getStationName() == StationName &&
                station->getChannelName() == ChannelName) {
            return station;
        }
    }

    return 0;
}

bool CStationList::contains(QString StationName, QString ChannelName)
{
    return (find(StationName, ChannelName) != 0);
}

void CStationList::append(QString StationName, QString ChannelName)
{
    mStationList.append(new StationElement(StationName, ChannelName));
}

bool CStationList::remove(QString StationName, QString ChannelName)
{
    for (QList<QObject*>::iterator it = mStationList.begin(); it != mStationList.end(); it++) {
        StationElement *station = (StationElement*)*it;
        if (station->getStationName() == StationName
                && station->getChannelName() == ChannelName) {
            mStationList.erase(it);
            return true;
        }
    }
    return false;
}

QList<QObject*> CStationList::getList(void)
{
    return  mStationList;
}

void CStationList::loadStations()
{
    QSettings Settings;
    Settings.beginGroup(mSettingsGroup + "s");
    int channelcount = Settings.value(mSettingsGroup + "cout", 0).toInt();
    for (int i = 1; i <= channelcount; i++) {
        QStringList SaveChannel = Settings.value(mSettingsGroup + "/" + QString::number(i)).toStringList();
        append(SaveChannel.first(), SaveChannel.last());
    }
    Settings.endGroup();
}

void CStationList::saveStations()
{
    QSettings Settings;

    // Remove channels from previous invocation ...
    Settings.beginGroup(mSettingsGroup + "s");
    int ChannelCount = Settings.value("channelcout").toInt();

    for (int i = 1; i <= ChannelCount; i++)
        Settings.remove(mSettingsGroup + "/" + QString::number(i));

    // ... and save the current set
    ChannelCount = mStationList.count();
    Settings.setValue(mSettingsGroup + "cout", QString::number(ChannelCount));

    for (int i = 1; i <= ChannelCount; i++)
        Settings.setValue(mSettingsGroup + "/" + QString::number(i), getStationAt(i - 1));

    Settings.endGroup();
}
