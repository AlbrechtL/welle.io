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
        property alias enableAutoSdr : enableAutoSdr.checked
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
        anchors.fill: parent
        spacing: Units.dp(20)

        SettingSection {
            id: settingsFrame
            isNotFirst: false
            text: qsTr("Global settings")

            Switch {
                id: enableFullScreen
                text: qsTr("Full screen mode")
                height: 24
                Layout.fillWidth: true
                checked: false
            }
        }

        SettingSection {
            text: qsTr("Global receiver settings")

//            TextStandart {
//                text: Number(radioController.deviceId)
//            }

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
                    TextStandart {
                        id: nameSliderView
                        Layout.fillWidth: true
                        text: qsTr("Manual gain")
                    }

                    TextStandart {
                        id: valueSliderView
                        text: qsTr("Value: ") + radioController.gainValue.toFixed(2)
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

            Switch {
                id: enableAutoSdr
                text: qsTr("Auto detect")
                height: 24
                Layout.fillWidth: true
                checked: true
            }

            ComboBox {
                id: manualChannelBox
                enabled: !enableAutoSdr.checked
                Layout.fillWidth: true
                font.pixelSize: TextStyle.textStandartSize
                font.family: TextStyle.textFont
                currentIndex: radioController.deviceId
                model: [ "Null", "Airspy", "RAW file", "rtl-sdr", "rtl-tcp", "SoapySDR" ];
                onCurrentIndexChanged: {
                    // Load appropriate settings
                    switch(currentIndex) {
                        case 3: sdrSpecificSettings.source = "qrc:/QML/settingpages/RTLSDRSettings.qml"; break
                        case 4: sdrSpecificSettings.source = "qrc:/QML/settingpages/RTLTCPSettings.qml"; break
                        case 5: sdrSpecificSettings.source = "qrc:/QML/settingpages/SoapySDRSettings.qml"; break
                        default: sdrSpecificSettings.source = "qrc:/QML/settingpages/NullSettings.qml"; break
                    }

                    // Load new device
                    if(!enableAutoSdr.checked
                            && currentIndex != 4) // rtl-tcp is openend in RTLTCPSettings.qml because we need the IP address and IP port
                        radioController.openDevice(currentIndex)
                }
            }
        }

        Loader {
            id: sdrSpecificSettings
            Layout.fillWidth: true
        }
    }
}
