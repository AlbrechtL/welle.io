/*
 *    This file is part of the welle.io.
 *    Settings of the Linux kernel SDR input (V4L2, /dev/swradioN).
 *    This file was generated with the help of an AI assistant (Claude Code).
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

// The device list is read again whenever the settings are shown, so that
// devices plugged in later appear
SettingSection {
    id: v4l2Settings
    objectName: "v4l2Settings"
    text: qsTr("Linux SDR settings")

    // Saved path of the device, empty: the first device that can be used
    property string devicePath: ""

    Settings {
        property alias v4l2DevicePath: v4l2Settings.devicePath
    }

    function initDevice(isAutoDevice) {
        if(!isAutoDevice)
            guiHelper.openV4l2(devicePath)
    }

    function __updateDevices() {
        deviceModel.clear()
        deviceModel.append({ label: QT_TRANSLATE_NOOP("Settings", "First found"), path: "" })
        let found = false
        for (const device of guiHelper.v4l2Devices()) {
            deviceModel.append({ label: device.label, path: device.path })
            found = found || device.path === devicePath
        }
        // A saved device that isn't there (now) stays chosen
        if (devicePath !== "" && !found)
            deviceModel.append({ label: devicePath, path: devicePath })
        deviceBox.currentIndex = __indexOf(devicePath)
    }

    function __indexOf(path) {
        for (let i = 0; i < deviceModel.count; i++) {
            if (deviceModel.get(i).path === path)
                return i
        }
        return 0
    }

    Component.onCompleted: __updateDevices()
    onVisibleChanged: if (visible) __updateDevices()

    SettingRow {
        title: qsTr("Device")
        description: qsTr("Receivers with a Linux kernel driver, e.g. rtl2832_sdr or airspy")

        SettingComboBox {
            id: deviceBox
            objectName: "setting.v4l2Device"
            model: ListModel { id: deviceModel }

            onActivated: (index) => {
                v4l2Settings.devicePath = deviceModel.get(index).path
                guiHelper.openV4l2(v4l2Settings.devicePath)
            }

            Accessible.name: qsTr("Device")
        }
    }
}
