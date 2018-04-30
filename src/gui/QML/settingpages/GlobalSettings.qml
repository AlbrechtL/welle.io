import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "../texts"
import "../components"

Item {
    id: settingsPage
    property alias enableFullScreenState : enableFullScreen.checked
    property alias enableLastPlayedStationState : enableLastPlayedStation.checked
    property alias enableExpertModeState : enableExpertMode.checked
    property alias enableAGCState : enableAGC.checked
    property alias enableHwAGCState : enableHwAGC.checked
    property alias manualGainState : manualGain.value

    Settings {
        property alias enableFullScreenState : settingsPage.enableFullScreenState
        property alias enableLastPlayedStationState : settingsPage.enableLastPlayedStationState
        property alias enableExpertModeState : settingsPage.enableExpertModeState
        property alias manualGainState : settingsPage.manualGainState
        property alias manualGainValue: valueSliderView.text
        property alias enableAGCState : settingsPage.enableAGCState
        property alias enableHwAGCState : settingsPage.enableHwAGCState
        property alias manualChannel: manualChannelBox.currentIndex
    }

    Component.onCompleted: {
        console.debug("Apply settings initially")
        cppGUI.inputEnableHwAGCChanged(enableHwAGCState)
        cppGUI.inputGainChanged(manualGainState)
        cppGUI.inputEnableAGCChanged(enableAGCState)
        console.debug("width: " + width)
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
            manualGain.to = cppRadioController.GainCount

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

        ColumnLayout{
            id: layout
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

                            Button {
                                id: startChannelScanButton
                                text: qsTr("Start")
                                Layout.alignment: Qt.AlignCenter
                                onClicked: {
                                    startChannelScanButton.enabled = false
                                    stopChannelScanButton.enabled = true
                                    cppGUI.startChannelScanClick()
                                }
                            }

                            Button {
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

                        ProgressBar{
                            id: channelScanProgressBar
                            from: 0
                            to: 54 // 54 channels
                            width: parent.width

                            property alias text: textView.text

                            Text {
                                id: textView
                                font.pixelSize: TextStyle.textStandartSize
                                font.family: TextStyle.textFont
                                color: TextStyle.textColor
                                anchors.centerIn: parent
                                text: qsTr("Found stations") + ": 0"
                            }
                        }

                        RowLayout {
                            Layout.preferredWidth: parent.width

                            Button {
                                id: clearListButton
                                text: qsTr("Clear station list")
                                Layout.preferredWidth: Units.dp(150)
                                Layout.alignment: Qt.AlignLeft
                                onClicked: cppGUI.clearStationList()
                            }

                            ComboBox {
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

                    Switch {
                        id: enableHwAGC
                        text: qsTr("Hardware RF gain")
                        height: 24
                        Layout.fillHeight: true
                        objectName: "enableHwAGC"
                        //visible: (cppRadioController.isHwAGCSupported && enableExpertMode.checked) ? true : false
                        visible: false // This switch is only for debug purposes and not tested, so disable it by default
                        checked: enableHwAGCState
                            onClicked: {
                            cppGUI.inputEnableHwAGCChanged(checked)
                        }
                    }

                    Switch {
                        id: enableAGC
                        text: qsTr("Automatic RF gain")
                        height: 24
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        objectName: "enableAGC"
                        checked: true
                        onClicked: {
                            cppGUI.inputEnableAGCChanged(checked)

                            if(checked == false)
                                cppGUI.inputGainChanged(manualGain.value)
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: parent.width
                        spacing: Units.dp(10)
                        opacity: enabled ? 1 : 0.5
                        enabled: !enableAGC.checked

                        RowLayout {
                            Layout.preferredWidth: parent.width

                            Text {
                                id: nameSliderView
                                font.pixelSize: TextStyle.textStandartSize
                                font.family: TextStyle.textFont
                                color: TextStyle.textColor
                                Layout.alignment: Qt.AlignLeft
                                text: qsTr("Manual gain")
                            }

                            Text {
                                id: valueSliderView
                                text: qsTr("Value: ") + cppGUI.currentGainValue.toFixed(2)
                                font.pixelSize: TextStyle.textStandartSize
                                font.family: TextStyle.textFont
                                color: TextStyle.textColor
                                Layout.alignment: Qt.AlignRight
                            }
                        }

                        Slider {
                            id: manualGain
                            from: 0
                            to: 100
                            value: manualGainState
                            stepSize: 1

                            Layout.preferredWidth: parent.width
                            onValueChanged: {
                                if(enableAGC.checked == false)
                                    cppGUI.inputGainChanged(value)
                            }
                        }
                    }

//                    TouchSlider {
//                        id: manualGain
//                        enabled: !enableAGC.checked
//                        name: qsTr("Manual gain")
//                        showCurrentValue: qsTr("Value: ") + cppGUI.currentGainValue.toFixed(2)
//                        Layout.fillHeight: true
//                        currentValue: manualGainState
//                        onValueChanged: {
//                            if(enableAGC.checked == false)
//                                cppGUI.inputGainChanged(valueGain)
//                        }
//                    }
                }
            }

            SettingsFrame {
                id: settingsFrame
                Layout.fillWidth: true
                ColumnLayout{
                    anchors.fill: parent
                    spacing: Units.dp(20)

                    Switch {
                        id: enableFullScreen
                        text: qsTr("Full screen mode")
                        height: 24
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        objectName: "enableFullScreen"
                        checked: false
                    }

                    Switch {
                        id: enableLastPlayedStation
                        text: qsTr("Last played")
                        height: 24
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        objectName: "enableLastPlayedStation"
                        checked: false
                    }

                    Switch {
                        id: enableExpertMode
                        text: qsTr("Expert mode")
                        height: 24
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        objectName: "enableExpertMode"
                        checked: false
                    }
                }
            }
        }

        ScrollBar.vertical: ScrollBar { }
    }
}
