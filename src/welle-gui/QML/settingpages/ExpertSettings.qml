import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

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
