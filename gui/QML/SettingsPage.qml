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

    Settings {
        property alias enableFullScreenState : settingsPage.enableFullScreenState
        property alias enableExpertModeState : settingsPage.enableExpertModeState
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
                            maximumValue: 38
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
                        name: "Enable AGC"
                        height: 24
                        Layout.fillHeight: true
                        objectName: "enableAGC"
                        checked: true
                        onChanged: cppGUI.inputEnableAGCChange(valueChecked)
                    }

                    TouchSlider {
                        id: gain
                        enabled: !enableAGC.checked
                        name: "Manual gain"
                        Layout.fillHeight: true
                        onValueChanged: cppGUI.inputGainChange(valueGain)
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
                        name: "Enable full screen mode"
                        height: 24
                        Layout.fillHeight: true
                        objectName: "enableFullScreen"
                        checked: false
                    }

                    TouchSwitch {
                        id: enableExpertMode
                        name: "Enable expert mode"
                        height: 24
                        Layout.fillHeight: true
                        objectName: "enableExpertMode"
                        checked: false
                        onChanged: {
                            if(valueChecked == false)
                                mainWindow.width = Units.dp(350) + radioInformationView.width
                        }
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
            onClicked:  cppGUI.terminateProcess()
            Layout.preferredWidth: parent.width
            Layout.alignment: Qt.AlignBottom
        }
    }
}
