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

import "../controls"

// Receiver settings: RF gain and input device, followed by the settings of
// the selected device (e.g. the RAW file). They are applied when the page
// is created at startup.
ColumnLayout {
    id: receiverSettings
    objectName: "receiverSettings"

    property alias device: deviceBox.currentIndex
    property bool isLoaded: false

    // Saved apart from the slider, whose range depends on the device and
    // would cut the value
    property int manualGainState: 0

    Layout.fillWidth: true
    spacing: Theme.spacingXL

    Settings {
        property alias device: deviceBox.currentIndex
        property alias enableAGCState : enableAGC.checked
        property alias manualGainState : receiverSettings.manualGainState
        property alias enableAutoSdr : enableAutoSdr.checked
    }

    Component.onCompleted: {
        console.debug("Apply settings initially")
        radioController.setGain(manualGainState)
        radioController.setAGC(enableAGC.checked)
    }

    Connections{
        target: radioController

        function onGainCountChanged() {manualGain.to = radioController.gainCount}
        // Changed elsewhere, e.g. in the gain card of the expert view: saved here
        function onAgcChanged(agc) {enableAGC.checked = agc}
        function onGainChanged(gain) {receiverSettings.manualGainState = gain}
    }

    Connections {
        target: guiHelper

        function onNewDeviceId(deviceId) {
            switch(deviceId) {
            case 0: deviceBox.currentIndex = 0; break; // UNKNOWN
            case 1: deviceBox.currentIndex = 0; break; // NULLDEVICE
            case 2: deviceBox.currentIndex = 1; break; // AIRSPY
            case 3: deviceBox.currentIndex = 5; break; // RAWFILE
            case 4: deviceBox.currentIndex = 2; break; // RTL_SDR
            case 5: deviceBox.currentIndex = 4; break; // RTL_TCP
            case 6: deviceBox.currentIndex = 3; break; // SOAPYSDR
            default: deviceBox.currentIndex = 0;
            }
        }
    }

    SettingSection {
        text: qsTr("Receiver")

        SettingSwitch {
            id: enableAGC
            objectName: "setting.enableAGCState"
            text: qsTr("Automatic RF gain")
            checked: true
            onClicked: {
                radioController.setAGC(checked)

                if(checked == false)
                    radioController.setGain(receiverSettings.manualGainState)
            }
        }

        SettingRow {
            title: qsTr("Manual gain")
            description: qsTr("Value: ") + radioController.gainValue.toFixed(2)
            wideControl: true
            enabled: !enableAGC.checked

            Slider {
                id: manualGain
                objectName: "setting.manualGainState"
                Layout.fillWidth: true
                from: 0
                to: 100
                stepSize: 1
                value: receiverSettings.manualGainState

                onMoved: {
                    receiverSettings.manualGainState = value
                    if(enableAGC.checked == false)
                        radioController.setGain(value)
                }

                Accessible.name: qsTr("Manual gain")
            }
        }

        SettingSwitch {
            id: enableAutoSdr
            objectName: "setting.enableAutoSdr"
            text: qsTr("Auto detect")
            description: qsTr("Use the first receiver that is found")
            checked: true
            onCheckedChanged: {
                if(enableAutoSdr.checked)
                    guiHelper.openAutoDevice()
            }
        }

        SettingRow {
            title: qsTr("Input device")
            enabled: !enableAutoSdr.checked

            SettingComboBox {
                id: deviceBox
                objectName: "setting.device"
                model: ListModel {
                    id: deviceBoxModel
                    ListElement { label: QT_TRANSLATE_NOOP("Settings", "None") }
                    ListElement { label: QT_TRANSLATE_NOOP("Settings", "Airspy") }
                    ListElement { label: QT_TRANSLATE_NOOP("Settings", "rtl-sdr") }
                    ListElement { label: QT_TRANSLATE_NOOP("Settings", "SoapySDR") }
                    ListElement { label: QT_TRANSLATE_NOOP("Settings", "rtl-tcp") }
                    ListElement { label: QT_TRANSLATE_NOOP("Settings", "RAW file") }
                }

                onCurrentIndexChanged: {
                    // Load appropriate settings
                    switch(currentIndex) {
                    case 1: sdrSpecificSettings.source = "qrc:/QML/settings/devices/AirspySettings.qml"; break
                    case 2: sdrSpecificSettings.source = "qrc:/QML/settings/devices/RTLSDRSettings.qml"; break
                    case 3: sdrSpecificSettings.source = "qrc:/QML/settings/devices/SoapySDRSettings.qml"; break
                    case 4: sdrSpecificSettings.source = "qrc:/QML/settings/devices/RTLTCPSettings.qml"; break
                    case 5: sdrSpecificSettings.source = "qrc:/QML/settings/devices/RawFileSettings.qml"; break
                    default: sdrSpecificSettings.source = "qrc:/QML/settings/devices/NullSettings.qml"; break
                    }

                    console.debug("Used input device: " + currentIndex)
                }

                Accessible.name: qsTr("Input device")
            }
        }

        Component.onCompleted: {
            if(enableAutoSdr.checked)
                guiHelper.openAutoDevice()

            sdrSpecificSettings.item.initDevice(enableAutoSdr.checked)
            receiverSettings.isLoaded = true
        }
    }

    Loader {
        id: sdrSpecificSettings
        objectName: "deviceSettings"
        Layout.fillWidth: true
        visible: item && item.implicitHeight > 0

        onLoaded: {
            if(receiverSettings.isLoaded)
                item.initDevice(enableAutoSdr.checked)
        }
    }
}
