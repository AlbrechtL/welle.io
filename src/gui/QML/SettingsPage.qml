import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "style"

Item {
    id: settingsPage
    property alias enableFullScreenState : enableFullScreen.checked
    property alias enableLastPlayedStationState : enableLastPlayedStation.checked
    property alias enableExpertModeState : enableExpertMode.checked
    property alias enableAGCState : enableAGC.checked
    property alias enableHwAGCState : enableHwAGC.checked
    property alias manualGainState : manualGain.currentValue
    property alias is3D : enable3D.checked

    Settings {
        property alias enableFullScreenState : settingsPage.enableFullScreenState
        property alias enableLastPlayedStationState : settingsPage.enableLastPlayedStationState
        property alias enableExpertModeState : settingsPage.enableExpertModeState
        property alias manualGainState : settingsPage.manualGainState
        property alias manualGainValue: manualGain.showCurrentValue
        property alias enableAGCState : settingsPage.enableAGCState
        property alias enableHwAGCState : settingsPage.enableHwAGCState
        property alias manualChannel: manualChannelBox.currentIndex
        property alias is3D : settingsPage.is3D
    }

    Component.onCompleted: {
        console.debug("Apply settings initially")
        cppGUI.inputEnableHwAGCChanged(enableHwAGCState)
        cppGUI.inputGainChanged(manualGainState)
        cppGUI.inputEnableAGCChanged(enableAGCState)
    }

    Connections{
        target: cppGUI
        onChannelScanStopped:{
            startChannelScanButton.enabled = true
            stopChannelScanButton.enabled = false
        }

        onChannelScanProgress:{
            startChannelScanButton.enabled = false
            stopChannelScanButton.enabled = true
            channelScanProgressBar.value = progress
        }

        onFoundChannelCount:{
            channelScanProgressBar.text = qsTr("Found stations") + ": " + channelCount;
        }

        onGuiDataChanged:{
            manualGain.maximumValue = cppRadioController.GainCount

            // Channel
            var channelIndex = manualChannelBox.find(guiData.Channel)
            if (channelIndex !== -1)
                manualChannelBox.currentIndex = channelIndex
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
                                text: qsTr("Found stations") + ": 0"
                            }

                            RowLayout {
                                Layout.preferredWidth: parent.width

                                TouchButton {
                                    id: clearListButton
                                    text: qsTr("Clear station list")
                                    Layout.preferredWidth: Units.dp(150)
                                    Layout.alignment: Qt.AlignLeft
                                    onClicked: cppGUI.clearStationList()
                                }

                                TouchComboBox {
                                    id: manualChannelBox
                                    enabled: true
                                    model: ["5A", "5B", "5C", "5D",
                                        "6A", "6B", "6C", "6D",
                                        "7A", "7B", "7C", "7D",
                                        "8A", "8B", "8C", "8D",
                                        "9A", "9B", "9C", "9D",
                                        "10A", "10B", "10C", "10D",
                                        "11A", "11B", "11C", "11D",
                                        "12A", "12B", "12C", "12D",
                                        "13A", "13B", "13C", "13D", "13E", "13F",
                                        "LA", "LB", "LC", "LD",
                                        "LE", "LF", "LG", "LH",
                                        "LI", "LJ", "LK", "LL",
                                        "LM", "LN", "LO", "LP"]

                                    Layout.preferredHeight: Units.dp(25)
                                    Layout.preferredWidth: Units.dp(130)
                                    Layout.alignment: Qt.AlignRight
                                    onActivated: {
                                        cppGUI.setManualChannel(model[index])
                                    }
                                }
                            }
                        }
                    }
                }

                SettingsFrame {
                    Layout.fillWidth: true
                    ColumnLayout{
                        anchors.fill: parent
                        spacing: Units.dp(20)

                        TouchSwitch {
                            id: enableHwAGC
                            name: qsTr("Hardware RF gain")
                            height: 24
                            Layout.fillHeight: true
                            objectName: "enableHwAGC"
                            //visible: (cppRadioController.isHwAGCSupported && enableExpertMode.checked) ? true : false
                            visible: false // This switch is only for debug purposes and not tested, so disable it by default
                            checked: enableHwAGCState
                            onChanged: {
                                cppGUI.inputEnableHwAGCChanged(valueChecked)
                            }
                        }

                        TouchSwitch {
                            id: enableAGC
                            name: qsTr("Automatic RF gain")
                            height: 24
                            Layout.fillWidth: true
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
                            showCurrentValue: qsTr("Value: ") + cppGUI.currentGainValue.toFixed(2)
                            Layout.fillHeight: true
                            currentValue: manualGainState
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
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            objectName: "enableFullScreen"
                            checked: false
                        }

                        TouchSwitch {
                            id: enableLastPlayedStation
                            name: qsTr("Last played")
                            height: 24
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            objectName: "enableLastPlayedStation"
                            checked: false
                        }

                        TouchSwitch {
                            id: enable3D
                            name: qsTr("Channel list layout (experimental)")
                            height: 24
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            objectName: "enable3D"
                            checked: false
                            onText: "3D"
                            offText: "2D"
                            visible: false // Deactivated because 3D view is not ready
                        }

                        TouchSwitch {
                            id: enableExpertMode
                            name: qsTr("Expert mode")
                            height: 24
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            objectName: "enableExpertMode"
                            checked: false
                        }
                    }
                }
            }

            TouchButton {
                id: exitAppButton
                text: qsTr("EXIT")
                onClicked: cppGUI.close()
                Layout.preferredWidth: parent.width
                Layout.alignment: Qt.AlignBottom
            }
        }

        ScrollBar.vertical: ScrollBar { }
    }
}
