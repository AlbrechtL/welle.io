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
    id: expertSettings

    anchors.fill: parent
    implicitHeight: layout.implicitHeight

    Settings {
        property alias enableExpertModeState : enableExpertMode.checked
        property alias disableCoarseState: disableCoarse.checked
        property alias enableDecodeTIIState: enableDecodeTII.checked
        property alias freqSyncMethodBoxState: freqSyncMethodBox.currentIndex
        property alias fftWindowPlacement: fftPlacementBox.currentIndex
    }

    property alias enableExpertModeState : enableExpertMode.checked

    ColumnLayout{
        id: layout
        anchors.fill: parent
        spacing: Units.dp(20)

        SettingSection {
            id: settingsFrame
            isNotFirst: false
            text: qsTr("Global")

            WSwitch {
                id: enableExpertMode
                text: qsTr("Expert mode")
                checked: false
            }
        }

        SettingSection {
            text: qsTr("Backend")
            enabled: enableExpertMode.checked

            WSwitch {
                id: disableCoarse
                Layout.fillWidth: true

                text: qsTr("Enable coarse corrector (for receivers with >1kHz error)")
                checked: true
                onCheckedChanged: {
                    radioController.disableCoarseCorrector(!checked)
                }

                Component.onCompleted: radioController.disableCoarseCorrector(!checked)
            }

            RowLayout {
                enabled: disableCoarse.checked

                WComboBoxList {
                    id: freqSyncMethodBox
                    textRole: 'trLabel'
                    model: ListModel {
                        id: freqSyncMethodBoxModel
                        ListElement { label: "GetMiddle"; trLabel: qsTr("GetMiddle"); trContext: "ExpertSettings" }
                        ListElement { label: "CorrelatePRS"; trLabel: qsTr("CorrelatePRS"); trContext: "ExpertSettings" }
                        ListElement { label: "PatternOfZeros"; trLabel: qsTr("PatternOfZeros"); trContext: "ExpertSettings" }
                    }
                    sizeToContents: true
                    currentIndex: 1
                    onCurrentIndexChanged: {
                        radioController.setFreqSyncMethod(currentIndex)
                    }

                    Component.onCompleted: radioController.setFreqSyncMethod(currentIndex)
                }

                TextStandart {
                    text: qsTr("Coarse corrector algorithm")
                    Layout.fillWidth: true
                }
            }

            WSwitch {
                id: enableDecodeTII
                Layout.fillWidth: true
                text: qsTr("Enable TII decoding to console log (increases CPU usage)")
                checked: false
                onCheckedChanged: {
                    radioController.enableTIIDecode(checked)
                }

                Component.onCompleted: radioController.enableTIIDecode(checked)
            }

            RowLayout {
                Layout.fillWidth: true
                WComboBoxList {
                    id: fftPlacementBox
                    textRole: 'trLabel'
                    model: ListModel {
                        id: fftPlacementBoxModel
                        ListElement { label: "Strongest Peak"; trLabel: qsTr("Strongest Peak"); trContext: "ExpertSettings" }
                        ListElement { label: "Earliest Peak With Binning"; trLabel: qsTr("Earliest Peak With Binning"); trContext: "ExpertSettings" }
                        ListElement { label: "Threshold Before Peak"; trLabel: qsTr("Threshold Before Peak"); trContext: "ExpertSettings" }
                    }
                    sizeToContents: true
                    currentIndex: 1
                    onCurrentIndexChanged: {
                        radioController.selectFFTWindowPlacement(currentIndex)
                    }

                    Component.onCompleted: radioController.selectFFTWindowPlacement(currentIndex)
                }

                TextStandart {
                    text: qsTr("FFT Window placement algorithm")
                    Layout.fillWidth: true
                }
            }
        }
    }
}
