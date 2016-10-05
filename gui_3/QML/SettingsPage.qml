import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1

// Import custom styles
import "style"

Item {
    id: settingsPage

    property alias showChannelState : showChannel.checked
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
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        height: parent.height

        ColumnLayout{
            spacing: Units.dp(30)

            ColumnLayout{
                spacing: Units.dp(10)
                RowLayout {
                    spacing: Units.dp(20)
                    TextStandart {
                        Behavior on x { NumberAnimation{ easing.type: Easing.OutCubic} }
                        text: "Channel scan"
                    }

                    TouchButton {
                        id: startChannelScanButton
                        text: "Start"
                        implicitWidth: Units.dp(80)
                        onClicked: {
                            startChannelScanButton.enabled = false
                            stopChannelScanButton.enabled = true
                            mainWindow.startChannelScanClicked()
                        }
                    }

                    TouchButton {
                        id: stopChannelScanButton
                        text: "Stop"
                        implicitWidth: Units.dp(80)
                        enabled: false
                        onClicked: {
                            startChannelScanButton.enabled = true
                            stopChannelScanButton.enabled = false
                            mainWindow.stopChannelScanClicked()
                        }
                    }
                }

                TouchProgressBar{
                    id: channelScanProgressBar
                    minimumValue: 0
                    maximumValue: 38
                    text: "Found channels: 0"
                }
            }

            TouchSwitch {
                id: showChannel
                name: "Show channel in station list"
                objectName: "showChannel"
                checked: false
            }

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
            }
        }

        TouchButton {
            id: exitAppButton
            text: "Exit dab-rpi"
            implicitWidth: parent.width
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Units.dp(10)
            onClicked:  mainWindow.exitApplicationClicked()
        }
    }
}
