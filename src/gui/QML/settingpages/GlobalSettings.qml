import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "../texts"
import "../components"

Item {
    id: settingsPage

    // Necessary for Flickable
    implicitHeight: layout.implicitHeight

    property alias enableFullScreenState : enableFullScreen.checked   
    property alias enableExpertModeState : enableExpertMode.checked
    property alias enableAGCState : enableAGC.checked 
    property alias manualGainState : manualGain.value

    Settings {
        property alias enableFullScreenState : settingsPage.enableFullScreenState
        property alias enableExpertModeState : settingsPage.enableExpertModeState
        property alias manualGainState : settingsPage.manualGainState
        property alias manualGainValue: valueSliderView.text
        property alias enableAGCState : settingsPage.enableAGCState     
    }

    Component.onCompleted: {
        console.debug("Apply settings initially")
        cppGUI.inputGainChanged(manualGainState)
        cppGUI.inputEnableAGCChanged(enableAGCState)
        console.debug("width: " + width)
    }

    Connections{
        target: cppGUI

        onGuiDataChanged:{
            manualGain.to = cppRadioController.GainCount
        }
    }

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

            Switch {
                id: enableFullScreen
                text: qsTr("Full screen mode")
                height: 24
                Layout.fillWidth: true
                checked: false
            }

            Switch {
                id: enableExpertMode
                text: qsTr("Expert mode")
                height: 24
                Layout.fillWidth: true
                checked: false
            }
        }


        SettingSection {
            text: qsTr("Backend")

            Switch {
                id: disableCoarse
                text: qsTr("Disable Coarse Corrector (for receivers with <1kHz error)")
                height: 24
                Layout.fillWidth: true
                checked: true
                onClicked: {
                    cppGUI.inputDisableCoarseCorrector(checked)
                }
            }

            Switch {
                id: decodeTII
                text: qsTr("Enable TII decoding to console log (increases CPU usage)")
                height: 24
                Layout.fillWidth: true
                checked: false
                onClicked: {
                    cppGUI.inputEnableTIIDecode(checked)
                }
            }

            Switch {
                id: oldFFTWindow
                text: qsTr("Select old FFT placement algorithm (experimental)")
                height: 24
                Layout.fillWidth: true
                checked: false
                onClicked: {
                    cppGUI.inputEnableOldFFTWindowPlacement(checked)
                }
            }

            ComboBox {
                id: freqSyncMethodBox
                enabled: true
                model: [ "GetMiddle", "CorrelatePRS", "PatternOfZeros" ];
                Layout.preferredHeight: Units.dp(25)
                Layout.preferredWidth: Units.dp(330)
                onActivated: {
                    cppGUI.inputSetFreqSyncMethod(index)
                }
            }
        }

        SettingSection {
            text: qsTr("Gain")

            Switch {
                id: enableAGC
                text: qsTr("Automatic RF gain")
                height: 24
                Layout.fillWidth: true
                checked: true
                onClicked: {
                    cppGUI.inputEnableAGCChanged(checked)

                    if(checked == false)
                        cppGUI.inputGainChanged(manualGain.value)
                }
            }

            ColumnLayout {
                spacing: Units.dp(10)
                opacity: enabled ? 1 : 0.5
                enabled: !enableAGC.checked

                RowLayout {
                    anchors.fill: parent
                    Text {
                        id: nameSliderView
                        font.pixelSize: TextStyle.textStandartSize
                        font.family: TextStyle.textFont
                        color: TextStyle.textColor
                        anchors.left: parent.left
                        text: qsTr("Manual gain")
                    }

                    Text {
                        id: valueSliderView
                        text: qsTr("Value: ") + cppGUI.currentGainValue.toFixed(2)
                        font.pixelSize: TextStyle.textStandartSize
                        font.family: TextStyle.textFont
                        color: TextStyle.textColor
                        anchors.right: parent.right
                    }
                }

                Slider {
                    id: manualGain
                    from: 0
                    to: 100
                    value: manualGainState
                    stepSize: 1
                    Layout.fillWidth: true

                    onValueChanged: {
                        if(enableAGC.checked == false)
                            cppGUI.inputGainChanged(value)
                    }
                }
            }
        }
    }
}
