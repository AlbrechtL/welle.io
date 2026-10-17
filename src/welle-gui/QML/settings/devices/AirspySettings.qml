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

SettingSection {
    objectName: "airspySettings"
    text: qsTr("Airspy settings")

    Settings {
        property alias airspyEnableBiasTeeState: enableBiasTee.checked
        property alias airspyGainModeState: gainMode.currentIndex
    }

    function initDevice(isAutoDevice) {
        if(!isAutoDevice)
            guiHelper.openAirspy()

        guiHelper.setBiasTeeAirspy(enableBiasTee.checked)
        guiHelper.setAirspyGainMode(gainMode.currentIndex)
    }

    SettingSwitch {
        id: enableBiasTee
        text: qsTr("Enable bias tee")
        onClicked: {
            guiHelper.setBiasTeeAirspy(checked)
        }
    }

    SettingRow {
        title: qsTr("Gain mode")
        description: qsTr("Which gain of the Airspy the automatic and the manual gain set")

        SettingComboBox {
            id: gainMode
            objectName: "setting.airspyGainMode"
            // The order is the value of DeviceParam::AirspyGainMode
            model: ListModel {
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Linearity") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Sensitivity") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Hybrid (tuner AGC)") }
            }
            currentIndex: 0
            onActivated: guiHelper.setAirspyGainMode(currentIndex)

            Accessible.name: qsTr("Gain mode")
        }
    }
}
