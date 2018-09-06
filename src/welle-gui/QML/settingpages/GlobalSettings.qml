import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "../texts"
import "../components"

Item {
    id: settingsPage

    implicitHeight: layout.implicitHeight
    implicitWidth:  layout.implicitWidth

    property alias enableFullScreenState : enableFullScreen.checked   
    property alias enableAGCState : enableAGC.checked 
    property alias manualGainState : manualGain.value

    Settings {
        property alias enableFullScreenState : settingsPage.enableFullScreenState
        property alias manualGainState : settingsPage.manualGainState
        property alias manualGainValue: valueSliderView.text
        property alias enableAGCState : settingsPage.enableAGCState     
    }

    Component.onCompleted: {
        console.debug("Apply settings initially")
        radioController.setGain(manualGainState)
        radioController.setAGC(enableAGCState)
    }

    Connections{
        target: radioController

        onGainCountChanged:{
            manualGain.to = radioController.gainCount
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
        }

        SettingSection {
            text: qsTr("SDR receiver")

            ComboBox {
                id: manualChannelBox
                Layout.fillWidth: true
                font.pixelSize: TextStyle.textStandartSize
                font.family: TextStyle.textFont
                model: [ "rtl-sdr", "rtl-tcp", "SoapySDR" ];
                onActivated: {
                    switch(index) {
                        case 0: sdrSpecificSettings.source = "qrc:/QML/settingpages/RTLSDRSettings.qml"; break
                        case 1: sdrSpecificSettings.source = "qrc:/QML/settingpages/RTLTCPSettings.qml"; break
                        case 2: sdrSpecificSettings.source = "qrc:/QML/settingpages/SoapySDRSettings.qml"; break
                    }
                }
            }

            Switch {
                id: enableAGC
                text: qsTr("Automatic RF gain")
                height: 24
                Layout.fillWidth: true
                checked: true
                onClicked: {
                    radioController.setAGC(checked)

                    if(checked == false)
                        radioController.setGain(manualGain.value)
                }
            }

            ColumnLayout {
                spacing: Units.dp(10)
                opacity: enabled ? 1 : 0.5
                enabled: !enableAGC.checked

                RowLayout {
                    Text {
                        id: nameSliderView
                        font.pixelSize: TextStyle.textStandartSize
                        font.family: TextStyle.textFont
                        color: TextStyle.textColor
                        Layout.fillWidth: true
                        text: qsTr("Manual gain")
                    }

                    Text {
                        id: valueSliderView
                        text: qsTr("Value: ") + radioController.gainValue.toFixed(2)
                        font.pixelSize: TextStyle.textStandartSize
                        font.family: TextStyle.textFont
                        color: TextStyle.textColor
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
                            radioController.setGain(value)
                    }
                }
            }
        }

        Loader {
            id: sdrSpecificSettings
            source : "qrc:/QML/settingpages/RTLSDRSettings.qml"
        }
    }
}
