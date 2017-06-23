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

	StationElement::StationElement (QString const stationName,
	                                QString const channelName,
	                                QObject *parent) : QObject(parent) {
	setProperty("stationName",stationName);
	setProperty("channelName",channelName);
}

QString StationElement::getStationName (void) {
	return m_stationName;
}

QString StationElement::getChannelName (void) {
	return m_channelName;
}

    CStationList::CStationList (void) {
	stationList. clear ();
}

    CStationList::~CStationList (void) {
}

void	CStationList::reset (void) {
	stationList. clear ();
}

bool	variantLessThan (const QObject* v1, const QObject* v2) {
    return (((StationElement*) v1) -> getStationName ()).compare(
                ((StationElement*) v2) -> getStationName (),
                Qt::CaseInsensitive) < 0;
}

void	CStationList::sort (void) {
	qSort (stationList. begin (), stationList. end (), variantLessThan);
}

int	CStationList::count (void) {
	return stationList. count ();
}

StationElement* CStationList::at (int i) {
	return (StationElement*) stationList. at (i);
}

QStringList CStationList::getStationAt (int i) {
QString StationName = at(i) -> getStationName ();
QString ChannelName = at(i) -> getChannelName ();
QStringList StationElement;

	StationElement. append (StationName);
	StationElement. append (ChannelName);
	return StationElement;
}

bool	CStationList::contains (QString value) {

	for (int i = 0; i < count (); i++) {
	   if (at (i) -> getStationName () == value)
	      return true;
	}

	return false;
}

void	CStationList::append (QString StationName, QString ChannelName) {
	stationList. append (new StationElement (StationName, ChannelName));
}

QList<QObject*>  CStationList::getList (void) {
	return  stationList;
}
