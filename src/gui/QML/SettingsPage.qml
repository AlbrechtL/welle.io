import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "style"

Item {
    id: settingsPage
    property alias showChannelState : enableExpertMode.checked
    property alias enableFullScreenState : enableFullScreen.checked
    property alias enableExpertModeState : enableExpertMode.checked
    property alias enableAGCState : enableAGC.checked
    property alias manualGainState : manualGain.currentValue

    Settings {
        property alias enableFullScreenState : settingsPage.enableFullScreenState
        property alias enableExpertModeState : settingsPage.enableExpertModeState
        property alias manualGainState : settingsPage.manualGainState
        property alias enableAGCState : settingsPage.enableAGCState
    }

    Connections{
        target: cppGUI
        onChannelScanStopped:{
            startChannelScanButton.enabled = true
            stopChannelScanButton.enabled = false
        }

        onChannelScanProgress:{
            channelScanProgressBar.value = progress
        }

        onFoundChannelCount:{
            channelScanProgressBar.text = "Found channels: " + channelCount;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Units.dp(20)
        spacing: Units.dp(30)

        ColumnLayout{
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: Units.dp(20)

            SettingsFrame{
                ColumnLayout{
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: Units.dp(20)

                    ColumnLayout{
                        Layout.preferredWidth: parent.width
                        spacing: parent.spacing / 2

                        RowLayout {
                            Layout.preferredWidth: parent.width

                            TextStandart {
                                text: "Channel scan"
                                Layout.alignment: Qt.AlignLeft
                            }

                            TouchButton {
                                id: startChannelScanButton
                                text: "Start"
                                Layout.preferredWidth: Units.dp(80)
                                Layout.alignment: Qt.AlignCenter
                                onClicked: {
                                    startChannelScanButton.enabled = false
                                    stopChannelScanButton.enabled = true
                                    cppGUI.startChannelScanClick()
                                }
                            }

                            TouchButton {
                                id: stopChannelScanButton
                                text: "Stop"
                                Layout.alignment: Qt.AlignRight
                                Layout.preferredWidth: Units.dp(80)
                                enabled: false
                                onClicked: {
                                    startChannelScanButton.enabled = true
                                    stopChannelScanButton.enabled = false
                                    cppGUI.stopChannelScanClick()
                                }
                            }
                        }

                        TouchProgressBar{
                            id: channelScanProgressBar
                            minimumValue: 0
                            maximumValue: 54 // 54 channels
                            width: parent.width
                            text: "Found channels: 0"
                        }
                    }


                    /*RowLayout {
                        Layout.preferredWidth: parent.width

                        ComboBox {
                            id: styleBox
                            model: ["5A", "5B", "5C"]
                            Component.onCompleted: { }
                        }

                        TouchButton {
                            id: resetChannelListButton
                            text: "Reset station list"
                            Layout.alignment: Qt.AlignRight
                            Layout.preferredWidth: Units.dp(110)
                            onClicked: { }
                        }
                    }*/
                }
            }

            SettingsFrame {
                Layout.fillWidth: true
                ColumnLayout{
                    anchors.fill: parent
                    spacing: Units.dp(20)

                    TouchSwitch {
                        id: enableAGC
                        name: "Automatic RF gain"
                        height: 24
                        Layout.fillHeight: true
                        objectName: "enableAGC"
                        checked: true
                        onChanged: {
                            cppGUI.inputEnableAGCChanged(valueChecked)

                            if(valueChecked == false)
                                cppGUI.inputGainChanged(manualGain.currentValue)
                        }
                    }

                    TouchSlider {
                        id: manualGain
                        enabled: !enableAGC.checked
                        name: "Manual gain"
                        maximumValue: cppGUI.gainCount
                        showCurrentValue: "Value: " + cppGUI.currentGainValue.toFixed(2)
                        Layout.fillHeight: true
                        onValueChanged: {
                            if(enableAGC.checked == false)
                                cppGUI.inputGainChanged(valueGain)
                        }
                    }
                }
            }

            SettingsFrame {
                id: settingsFrame
                Layout.fillWidth: true
                ColumnLayout{
                    anchors.fill: parent
                    spacing: Units.dp(20)

                    TouchSwitch {
                        id: enableFullScreen
                        name: "Full screen mode"
                        height: 24
                        Layout.fillHeight: true
                        objectName: "enableFullScreen"
                        checked: false
                    }

                    TouchSwitch {
                        id: enableExpertMode
                        name: "Expert mode"
                        height: 24
                        Layout.fillHeight: true
                        objectName: "enableExpertMode"
                        checked: false
                    }
                }
            }

            /*TouchButton {
                id: inputSettingsButton
                text: "Input settings"
                width: parent.width
            }*/
        }

        TouchButton {
            id: exitAppButton
            text: "Exit welle.io"
            onClicked: Qt.quit()
            Layout.preferredWidth: parent.width
            Layout.alignment: Qt.AlignBottom
        }
    }
}
