import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3

// Import custom styles
import "texts"
import "components"

ViewBaseFrame {
    id: frame
    labelText: qsTr("Service Overview")

//    Layout.minimumHeight: Units.dp(150)
    Layout.maximumHeight: Units.dp(230)

    TextRadioInfo {
        anchors.top: parent.top
        anchors.topMargin: Units.dp(5)
        anchors.horizontalCenter: parent.horizontalCenter
        text: radioController.ensemble.trim()
    }

    // Use a button to display a icon
    Button {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: Units.dp(-20) // ToDo Hack!
        anchors.rightMargin: Units.dp(-10) // ToDo Hack!

        background: Rectangle { opacity: 0} // Hack to disable the pressed color
        checkable: true
        flat: true

        icon.name: checked ? "speaker_mute" : "speaker"
        icon.width: Units.dp(30)
        icon.height: Units.dp(30)
        icon.color: checked ? "red" : "transparent"

        implicitWidth: contentItem.implicitWidth + Units.dp(30)
        implicitHeight: implicitWidth

        onCheckedChanged: checked ? radioController.setVolume(0) : radioController.setVolume(100)
    }

    RowLayout{
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: Units.dp(5)
        spacing: Units.dp(2)

        Rectangle{
            height: Units.dp(4)
            width: Units.dp(4)
            color: (radioController.snr > 2) ? "green" : "grey"
        }
        Rectangle{
            height: Units.dp(8)
            width: Units.dp(4)
            color: (radioController.snr > 5) ? "green" : "grey"
        }
        Rectangle{
            height: Units.dp(12)
            width: Units.dp(4)
            color: (radioController.snr > 8) ? "green" : "grey"
        }
        Rectangle{
            height: Units.dp(16)
            width: Units.dp(4)
            color: (radioController.snr > 11) ? "green" : "grey"
        }

        Rectangle{
            height: Units.dp(20)
            width: Units.dp(4)
            color: (radioController.snr > 15) ? "green" : "grey"
        }
    }

    ColumnLayout {
        anchors.centerIn: parent

        /* Station Name */
        RowLayout {
            Layout.alignment: Qt.AlignHCenter

            TextRadioStation {
                text: radioController.title.trim()

                // Use a button to display a icon
                Button  {
                    property bool isSignal: false
                    id: antennaSymbol
                    x: parent.width + Units.dp(5)
                    y: (parent.height / 2) - (height / 2)

                    visible: opacity == 0 ? false : true
                    opacity: 100
                    icon.name: isSignal ? "antenna" : "antenna_no_signal"
                    icon.width: Units.dp(30)
                    icon.height: Units.dp(30)
                    icon.color: isSignal ? "transparent" : "red"
                    background: Rectangle { opacity: 0 } // Hack to disable the pressed color
                    implicitWidth: contentItem.implicitWidth + Units.dp(20)
                    implicitHeight: implicitWidth

                    NumberAnimation on opacity {
                        id: effect
                        to: 0;
                        duration: 6000;
                        running: false
                    }

                    Connections {
                        target: radioController
                        onIsFICCRCChanged: {
                            if(radioController.isFICCRC) {
                                antennaSymbol.isSignal = true
                                effect.restart()
                            }
                            else {
                                antennaSymbol.isSignal = false
                                antennaSymbol.opacity = 100
                                effect.stop()
                            }
                        }

                        onIsSyncChanged: {
                            if(radioController.isSync) {
                                antennaSymbol.isSignal = true
                                effect.restart()
                            }
                            else {
                                antennaSymbol.isSignal = false
                                antennaSymbol.opacity = 100
                                effect.stop()
                            }
                        }
                    }
                }
            }
        }

        /* Station Text */
        TextRadioInfo {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: Units.dp(10)
            Layout.maximumWidth: parent.parent.width
            width: frame.width
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            text: radioController.text.trim()
        }
    }

    RowLayout{
        id: stationInfo
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Units.dp(5)
        width: parent.width

        TextRadioInfo {
            visible: stationInfo.visible
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: Units.dp(5)
            text: radioController.stationType
        }

        TextRadioInfo {
            visible: stationInfo.visible
            Layout.alignment: Qt.AlignRight
            Layout.rightMargin: Units.dp(5)
            text: (radioController.isDAB ? "DAB" : "DAB+")
                + " " + radioController.audioMode
        }
    }
}
