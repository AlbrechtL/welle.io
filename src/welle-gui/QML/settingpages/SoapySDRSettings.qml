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
    text: qsTr("SoapySDR settings")
    spacing: Units.dp(20)

    Settings {
        property alias soapyAntenna: antenna.text
        property alias soapyClockSource: clockSource.text
        property alias soapyDriverArgs: driverArgs.text
    }

    ColumnLayout {
        spacing: Units.dp(20)
        RowLayout {
            TextStandart {
                text: qsTr("Antenna")
                Layout.fillWidth: true
            }

            TextField {
                id: antenna
                placeholderText: qsTr("Enter antenna")
                implicitWidth: 300
            }
        }

        RowLayout {
            TextStandart {
                text: qsTr("Clock source")
                Layout.fillWidth: true
            }

            TextField {
                id: clockSource
                placeholderText: qsTr("Enter clock source")
                implicitWidth: 300
            }
        }

        RowLayout {
            TextStandart {
                text: qsTr("Driver arguments")
                Layout.fillWidth: true
            }

            TextField {
                id: driverArgs
                placeholderText: qsTr("Enter driver arguments")
                implicitWidth: 300
            }
        }

        WButton {
            id: applyButton
            text: qsTr("Apply")
            Layout.fillWidth: true
            onClicked: {
                __setParams()
            }
        }
    }

    function initDevice(isAutoDevice) {
        __setParams()
        if(!isAutoDevice)
            guiHelper.openSoapySdr()
    }

    function __setParams() {
        guiHelper.setDriverArgsSoapySdr(driverArgs.text)
        guiHelper.setAntennaSoapySdr(antenna.text)
        guiHelper.setClockSourceSoapySdr(clockSource.text)
    }
}
