import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

import "../texts"
import "../components"

Item {
    id: expertSettings
    implicitHeight: layout.implicitHeight
    implicitWidth:  layout.implicitWidth

    Settings {
        property alias enableExpertModeState : enableExpertMode.checked
        property alias disableCoarseState: disableCoarse.checked
        property alias enableDecodeTIIState: enableDecodeTII.checked
        property alias enableOldFFTWindowState: enableOldFFTWindow.checked
        property alias freqSyncMethodBoxState: freqSyncMethodBox.currentIndex
    }

    property alias enableExpertModeState : enableExpertMode.checked

    ColumnLayout{
        id: layout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Units.dp(20)

        SettingSection {
            id: settingsFrame
            isNotFirst: false
            text: qsTr("Global")

            SettingSwitch {
                id: enableExpertMode
                text: qsTr("Expert mode")
                checked: false
            }
        }

        SettingSection {
            text: qsTr("Backend")
            enabled: enableExpertMode.checked

            SettingSwitch {
                id: disableCoarse
                text: qsTr("Disable coarse corrector (for receivers with <1kHz error)")
                checked: true
                onCheckedChanged: {
                    radioController.disableCoarseCorrector(checked)
                }

                Component.onCompleted: radioController.disableCoarseCorrector(checked)
            }

            RowLayout {
                enabled: !disableCoarse.checked
                WComboBox {
                    id: freqSyncMethodBox
                    model: [ "GetMiddle", "CorrelatePRS", "PatternOfZeros" ];
                    onCurrentIndexChanged: {
                        radioController.setFreqSyncMethod(currentIndex)
                    }

                    Component.onCompleted: radioController.setFreqSyncMethod(currentIndex)
                }

                TextStandart {
                    text: qsTr("Coarse corrector algorithm")
                }
            }

            SettingSwitch {
                id: enableDecodeTII
                text: qsTr("Enable TII decoding to console log (increases CPU usage)")
                checked: false
                onCheckedChanged: {
                    radioController.enableTIIDecode(checked)
                }

                Component.onCompleted: radioController.enableTIIDecode(checked)
            }

            SettingSwitch {
                id: enableOldFFTWindow
                text: qsTr("Select old FFT placement algorithm (experimental)")
                checked: false
                onCheckedChanged: {
                    radioController.enableOldFFTWindowPlacement(checked)
                }

                Component.onCompleted: radioController.enableOldFFTWindowPlacement(checked)
            }
        }
    }
}
