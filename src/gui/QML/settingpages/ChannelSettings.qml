import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

import "../texts"
import "../components"

Item {
    // Necessary for Flickable
    implicitHeight: layout.implicitHeight

    Settings {
        property alias manualChannel: manualChannelBox.currentIndex
        property alias enableLastPlayedStationState : enableLastPlayedStation.checked
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
            // Channel
            var channelIndex = manualChannelBox.find(guiData.Channel)
            if (channelIndex !== -1)
                manualChannelBox.currentIndex = channelIndex
        }
    }

    ColumnLayout{
        id: layout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Units.dp(20)

        SettingSection {
            isNotFirst: false
            text: qsTr("Channel scan")

            RowLayout {
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

        SettingSection {
            text: qsTr("Misc")

            Switch {
                id: enableLastPlayedStation
                text: qsTr("Last played")
                height: 24
                Layout.fillWidth: true
                Layout.fillHeight: true
                objectName: "enableLastPlayedStation"
                checked: false
            }
        }
    }
}
