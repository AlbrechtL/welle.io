import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "style"

Item {
    id: settingsPage

    property alias showChannelState : enableExpertMode.checked
    property alias enableFullScreenState : enableFullScreen.checked
    property alias enableExpertModeState : enableExpertMode.checked

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
                ColumnLayout{
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: Units.dp(20)

                    TouchSwitch {
                        id: enableAGC
                        name: "Enable AGC"
                        objectName: "enableAGC"
                        checked: true
                        onChanged: cppGUI.inputEnableAGCChanged(valueChecked)
                    }

                    TouchSlider {
                        id: gain
                        enabled: !enableAGC.checked
                        name: "Manual gain"
                        onValueChanged: cppGUI.inputGainChanged(valueGain)
                    }
                }
            }

            SettingsFrame {
                ColumnLayout{
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: Units.dp(20)

                    TouchSwitch {
                        id: enableFullScreen
                        name: "Enable full screen mode"
                        objectName: "enableFullScreen"
                        checked: false
                    }

                    TouchSwitch {
                        id: enableExpertMode
                        name: "Enable expert mode"
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
