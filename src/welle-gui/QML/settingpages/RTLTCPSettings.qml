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
import QtQuick.Layouts
import QtQuick.Controls
import QtCore

// Import custom styles
import "../texts"
import "../components"

SettingSection {
    text: qsTr("rtl-tcp settings")

    property bool isLoaded : false

    Settings {
        property alias ipByte1: ipAddress1.currentIndex
        property alias ipByte2: ipAddress2.currentIndex
        property alias ipByte3: ipAddress3.currentIndex
        property alias ipByte4: ipAddress4.currentIndex
        property alias ipPort: ipPort.currentIndex
        property alias rtlTcpEnableHostName: enableHostName.checked
        property alias rtlTcpHostName: hostName.text
    }

    WSwitch {
        id: enableHostName
        text: qsTr("Use host name")
        Layout.fillWidth: true
        checked: false
    }

    RowLayout {
        spacing: Units.dp(20)

        ColumnLayout {
            TextStandart {
                text: qsTr("IP address")
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                id:layout
                height: Units.dp(120)
                spacing: Units.dp(5)
                enabled: !enableHostName.checked

                WTumbler {
                    id: ipAddress1
                    currentIndex: 192
                }
                WTumbler {
                    id: ipAddress2
                    currentIndex: 168
                }
                WTumbler {
                    id: ipAddress3
                    currentIndex: 1
                }
                WTumbler {
                    id: ipAddress4
                    currentIndex: 10
                }
            }
        }

        ColumnLayout {
            TextStandart {
                text: qsTr("IP port")
            }

            WTumbler {
                id: ipPort
                model: 65536
                currentIndex: 1234
            }
        }
    }

    RowLayout {
        enabled: enableHostName.checked

        TextField {
            id: hostName
            placeholderText: qsTr("Enter host name")
            implicitWidth: 200
        }

        TextStandart {
            text: qsTr("Host name")
            Layout.fillWidth: true
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

        if(!enableHostName.checked)
            serverAddress =
                ipAddress1.currentIndex + "."
                + ipAddress2.currentIndex + "."
                + ipAddress3.currentIndex + "."
                + ipAddress4.currentIndex
        else
            serverAddress = hostName.text

        guiHelper.openRtlTcp(serverAddress, ipPort.currentIndex, true)
    }
}

