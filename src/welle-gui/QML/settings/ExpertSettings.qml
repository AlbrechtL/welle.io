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

// Expert mode and settings of the receiver backend
SettingSection {
    id: expertSettings
    objectName: "expertSettings"
    text: qsTr("Expert")

    Settings {
        property alias enableExpertModeState : enableExpertMode.checked
        property alias disableCoarseState: disableCoarse.checked
        property alias enableDecodeTIIState: enableDecodeTII.checked
        property alias freqSyncMethodBoxState: freqSyncMethodBox.currentIndex
        property alias fftWindowPlacement: fftPlacementBox.currentIndex
        property alias demodulatorMethod: demodulatorBox.currentIndex
    }

    property alias enableExpertModeState : enableExpertMode.checked

    SettingSwitch {
        id: enableExpertMode
        objectName: "setting.enableExpertModeState"
        text: qsTr("Expert mode")
        description: qsTr("Technical views like spectrum and constellation diagram, manual channel selection")
        checked: false
    }

    SettingSwitch {
        id: disableCoarse
        objectName: "setting.disableCoarseState"
        enabled: enableExpertMode.checked

        text: qsTr("Enable coarse corrector (for receivers with >1kHz error)")
        checked: true
        onCheckedChanged: {
            radioController.disableCoarseCorrector(!checked)
        }

        Component.onCompleted: radioController.disableCoarseCorrector(!checked)
    }

    SettingRow {
        title: qsTr("Coarse corrector algorithm")
        enabled: enableExpertMode.checked && disableCoarse.checked

        SettingComboBox {
            id: freqSyncMethodBox
            objectName: "setting.freqSyncMethodBoxState"
            model: ListModel {
                id: freqSyncMethodBoxModel
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "GetMiddle") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "CorrelatePRS") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "PatternOfZeros") }
            }
            currentIndex: 1
            onCurrentIndexChanged: {
                radioController.setFreqSyncMethod(currentIndex)
            }

            Component.onCompleted: radioController.setFreqSyncMethod(currentIndex)

            Accessible.name: qsTr("Coarse corrector algorithm")
        }
    }

    SettingSwitch {
        id: enableDecodeTII
        objectName: "setting.enableDecodeTIIState"
        enabled: enableExpertMode.checked
        text: qsTr("Enable TII decoding to console log (increases CPU usage)")
        checked: false
        onCheckedChanged: {
            radioController.enableTIIDecode(checked)
        }

        Component.onCompleted: radioController.enableTIIDecode(checked)
    }

    SettingRow {
        title: qsTr("FFT Window placement algorithm")
        enabled: enableExpertMode.checked

        SettingComboBox {
            id: fftPlacementBox
            objectName: "setting.fftWindowPlacement"
            model: ListModel {
                id: fftPlacementBoxModel
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Strongest Peak") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Earliest Peak With Binning") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Threshold Before Peak") }
            }
            currentIndex: 1
            onCurrentIndexChanged: {
                radioController.selectFFTWindowPlacement(currentIndex)
            }

            Component.onCompleted: radioController.selectFFTWindowPlacement(currentIndex)

            Accessible.name: qsTr("FFT Window placement algorithm")
        }
    }

    SettingRow {
        title: qsTr("Demodulator")
        description: qsTr("Coherent receives weak signals with up to 2 dB less SNR, but only with a stationary antenna")
        enabled: enableExpertMode.checked

        SettingComboBox {
            id: demodulatorBox
            objectName: "setting.demodulatorMethod"
            model: ListModel {
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Differential") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Coherent") }
            }
            currentIndex: 0
            onCurrentIndexChanged: {
                radioController.setDemodulationMethod(currentIndex)
            }

            Component.onCompleted: radioController.setDemodulationMethod(currentIndex)

            Accessible.name: qsTr("Demodulator")
        }
    }
}
