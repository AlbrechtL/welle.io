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

// Import custom styles
import "../texts"
import "../components"

SettingSection {
    spacing: Units.dp(20)
    text: qsTr("Airspy settings")

    Settings {
        property alias airspyEnableBiasTeeState: enableBiasTee.checked
    }

    function initDevice(isAutoDevice) {
        if(!isAutoDevice)
            guiHelper.openAirspy()

        guiHelper.setBiasTeeAirspy(enableBiasTee.checked)
    }

    Switch {
        id: enableBiasTee
        text: qsTr("Enable bias tee")
        onClicked: {
            guiHelper.setBiasTeeAirspy(checked)
        }
    }
}
