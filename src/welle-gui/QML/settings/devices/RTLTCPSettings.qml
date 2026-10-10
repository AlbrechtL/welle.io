/*
 *    Copyright (C) 2017 - 2026
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
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore

import "../../controls"

// The server is connected when the settings are closed
SettingSection {
    text: qsTr("rtl-tcp settings")
    id: rtltcpsettings

    property bool isLoaded : false

    Settings {
        id: settings

        // Deprecated: Necessary for migration from version <=2.5, can be removed in future versions
        property int ipByte1
        property int ipByte2
        property int ipByte3
        property int ipByte4

        property alias ipPort: hostPort.text
        property alias rtlTcpHostName: hostName.text
    }

    SettingRow {
        title: qsTr("Host name or IP-address")

        TextField {
            id: hostName
            placeholderText: qsTr("Enter host name or IP address")
            Layout.preferredWidth: 240
            Accessible.name: qsTr("Host name or IP-address")
        }
    }

    SettingRow {
        title: qsTr("IP-Port")

        TextField {
            id: hostPort
            placeholderText: qsTr("Enter IP-port")
            Layout.preferredWidth: 240
            text: "1234"
            Accessible.name: qsTr("IP-Port")
        }
    }

    onVisibleChanged: {
        if(!visible && isLoaded)
            __openDevice()
    }

    function initDevice(isAutoDevice) {
        if(!isAutoDevice)
            __openDevice()

        isLoaded = true
    }

    function __openDevice() {
        var serverAddress = "";

        if(settings.ipByte1) {
            // Deprecated: Necessary for migration from version <=2.5, can be removed in future versions
            serverAddress =
                settings.ipByte1 + "."
                + settings.ipByte2 + "."
                + settings.ipByte3 + "."
                + settings.ipByte4
            hostName.text = serverAddress

            // Clear IP address to avoid further usage
            settings.ipByte1 = 0
            settings.ipByte2 = 0
            settings.ipByte3 = 0
            settings.ipByte4 = 0
        }

        guiHelper.openRtlTcp(hostName.text, hostPort.text, true)
    }
}

