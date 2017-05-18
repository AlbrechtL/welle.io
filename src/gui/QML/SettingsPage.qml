import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "style"

Item {
    id: settingsPage
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
            channelScanProgressBar.text = qsTr("Found channels") + ": " + channelCount;
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: layout.implicitHeight > parent.height ? layout.implicitHeight : parent.height
        contentWidth: parent.width

        ColumnLayout {
            id: layout
            anchors.fill: parent
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
                                    text: qsTr("Channel scan")
                                    Layout.alignment: Qt.AlignLeft
                                }

                                TouchButton {
                                    id: startChannelScanButton
                                    text: qsTr("Start")
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
                                    text: qsTr("Stop")
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
                                text: qsTr("Found channels") + ": 0"
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
                            name: qsTr("Automatic RF gain")
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
                            name: qsTr("Manual gain")
                            maximumValue: cppGUI.gainCount
                            showCurrentValue: qsTr("Value: ") + cppGUI.currentGainValue.toFixed(2)
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
                            name: qsTr("Full screen mode")
                            height: 24
                            Layout.fillHeight: true
                            objectName: "enableFullScreen"
                            checked: false
                        }

                        TouchSwitch {
                            id: enableExpertMode
                            name: qsTr("Expert mode")
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
                text: qsTr("Exit welle.io")
                onClicked: Qt.quit()
                Layout.preferredWidth: parent.width
                Layout.alignment: Qt.AlignBottom
            }
        }
    }
}
