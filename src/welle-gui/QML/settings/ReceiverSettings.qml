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

// Receiver settings: input device and RF gain, followed by the settings of
// the selected device (e.g. the RAW file). They are applied when the page
// is created at startup.
ColumnLayout {
    id: receiverSettings
    objectName: "receiverSettings"

    // The chosen input device, saved as its position in the full list of
    // devices below. The device box shows only the devices of this build.
    property int device: 0
    property bool isLoaded: false

    // All input devices. "setting" is the saved value and must not change,
    // "deviceId" is the CDeviceID (src/input/virtual_input.h), "page" the
    // device settings
    readonly property var allDevices: [
        { label: QT_TRANSLATE_NOOP("Settings", "None"), setting: 0, deviceId: 1, page: "NullSettings.qml" },
        { label: QT_TRANSLATE_NOOP("Settings", "Airspy"), setting: 1, deviceId: 2, page: "AirspySettings.qml" },
        { label: QT_TRANSLATE_NOOP("Settings", "rtl-sdr"), setting: 2, deviceId: 4, page: "RTLSDRSettings.qml" },
        { label: QT_TRANSLATE_NOOP("Settings", "SoapySDR"), setting: 3, deviceId: 6, page: "SoapySDRSettings.qml" },
        { label: QT_TRANSLATE_NOOP("Settings", "rtl-tcp"), setting: 4, deviceId: 5, page: "RTLTCPSettings.qml" },
        { label: QT_TRANSLATE_NOOP("Settings", "RAW file"), setting: 5, deviceId: 3, page: "RawFileSettings.qml" }
    ]

    // The saved device, or "None" if this build doesn't support it
    readonly property var shownDevice: isAvailable(allDevices[device]) ? allDevices[device] : allDevices[0]

    function isAvailable(entry) {
        if (!entry)
            return false
        const available = guiHelper.availableDevices
        // rtl-sdr is ANDROID_RTL_SDR (7) on Android
        return available.indexOf(entry.deviceId) >= 0
                || (entry.deviceId === 4 && available.indexOf(7) >= 0)
    }

    // Saved apart from the slider, whose range depends on the device and
    // would cut the value
    property int manualGainState: 0

    Layout.fillWidth: true
    spacing: Theme.spacingXL

    Settings {
        property alias device: receiverSettings.device
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
            case 0: receiverSettings.device = 0; break; // UNKNOWN
            case 1: receiverSettings.device = 0; break; // NULLDEVICE
            case 2: receiverSettings.device = 1; break; // AIRSPY
            case 3: receiverSettings.device = 5; break; // RAWFILE
            case 4: receiverSettings.device = 2; break; // RTL_SDR
            case 5: receiverSettings.device = 4; break; // RTL_TCP
            case 6: receiverSettings.device = 3; break; // SOAPYSDR
            default: receiverSettings.device = 0;
            }
        }
    }

    SettingSection {
        text: qsTr("Receiver")

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
                // Only the devices of this build
                model: ListModel {
                    id: deviceBoxModel
                    Component.onCompleted: {
                        for (const entry of receiverSettings.allDevices) {
                            if (receiverSettings.isAvailable(entry))
                                append({ label: entry.label, setting: entry.setting })
                        }
                    }
                }

                currentIndex: {
                    for (let i = 0; i < deviceBoxModel.count; i++) {
                        if (deviceBoxModel.get(i).setting === receiverSettings.shownDevice.setting)
                            return i
                    }
                    return -1
                }

                onActivated: (index) => {
                    receiverSettings.device = deviceBoxModel.get(index).setting
                    console.debug("Used input device: " + receiverSettings.device)
                }

                Accessible.name: qsTr("Input device")
            }
        }

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
        source: "qrc:/QML/settings/devices/" + receiverSettings.shownDevice.page
        visible: item && item.implicitHeight > 0

        onLoaded: {
            if(receiverSettings.isLoaded)
                item.initDevice(enableAutoSdr.checked)
        }
    }
}
