/*
 *    Copyright (C) 2017 - 2021
 *    Albrecht Lohofener (albrechtloh@gmx.de)
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
 
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

import "../texts"
import "../components"

Item {
    id: channelSettingsPage
    implicitHeight: layout.implicitHeight

    property alias addStationNameToWindowTitleState : addStationNameToWindowTitle.checked

    Settings {
        property alias enableLastPlayedStationState : enableLastPlayedStation.checked
        property alias addStationNameToWindowTitleState : channelSettingsPage.addStationNameToWindowTitleState
    }

    ColumnLayout{
        id: layout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Units.dp(20)

        WSwitch {
            id: enableLastPlayedStation
            text: qsTr("Automatic start playing last station")
            checked: false
            Layout.fillWidth: true
            onClicked: radioController.setAutoPlay(checked, radioController.lastChannel[1], radioController.lastChannel[0])
        }

        WSwitch {
            id: addStationNameToWindowTitle
            text: qsTr("Display station name in the window title")
            checked: false
            Layout.fillWidth: true
        }
    }
}
