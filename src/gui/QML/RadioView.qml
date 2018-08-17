import QtQuick 2.2
import QtQuick.Layouts 1.1

// Import custom styles
import "texts"
import "components"

Item{
    id: item
    Layout.preferredWidth: Units.dp(320)
    Layout.minimumWidth: Units.dp(150)
    Layout.preferredHeight: Units.dp(180)

    TextRadioInfo {
        id: ensembleText
        anchors.top: parent.top
        anchors.topMargin: Units.dp(5)
        anchors.horizontalCenter: parent.horizontalCenter
        text: guiHelper.guiData.Ensemble
    }

    RowLayout{
        anchors.top: parent.top
        anchors.topMargin: Units.dp(5)
        Layout.fillWidth : true
        Layout.fillHeight: true
        width: parent.width

        RowLayout{
            Layout.fillWidth : true
            Layout.fillHeight: true
            Layout.leftMargin: Units.dp(5)
            spacing: Units.dp(2)

            Rectangle{
                id: signalBar1
                height: Units.dp(4)
                width: Units.dp(4)
                color: (radioController.snr > 2) ? "green" : "grey"
            }
            Rectangle{
                id: signalBar2
                height: Units.dp(8)
                width: Units.dp(4)
                color: (radioController.snr > 5) ? "green" : "grey"
            }
            Rectangle{
                id: signalBar3
                height: Units.dp(12)
                width: Units.dp(4)
                color: (radioController.snr > 8) ? "green" : "grey"
            }
            Rectangle{
                id: signalBar4
                height: Units.dp(16)
                width: Units.dp(4)
                color: (radioController.snr > 11) ? "green" : "grey"
            }

            Rectangle{
                id: signalBar5
                height: Units.dp(20)
                width: Units.dp(4)
                color: (radioController.snr > 15) ? "green" : "grey"
            }
        }

        /* Flags */
        RowLayout{
            id: flags
            Layout.fillWidth : true
            Layout.fillHeight: true
            Layout.rightMargin: Units.dp(5)
            Layout.alignment: Qt.AlignRight
            spacing: 2

            Rectangle{
                id: sync
                height: Units.dp(16)
                width: Units.dp(16)
                color: radioController.isSync ? "green" : "red"
            }
            Rectangle{
                id: fic
                height: Units.dp(16)
                width: Units.dp(16)
                color: radioController.isFICCRC ? "green" : "red"
            }
            Rectangle{
                id: frameSucess
                height: Units.dp(16)
                width: Units.dp(16)
                color: (radioController.frameErrors === 0
                        && radioController.isSync
                        && radioController.isFICCRC) ? "green" : "red"
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent

        /* Station Name */
        TextRadioStation {
            id: stationTitle
            Layout.alignment: Qt.AlignHCenter
            text: guiHelper.guiData.Title
        }

        /* Station Text */
        TextRadioInfo {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: Units.dp(10)
            id: stationText
            Layout.maximumWidth: parent.parent.width
            width: parent.parent.width
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            text: guiHelper.guiData.Text
        }
    }

    RowLayout{
        id: stationInfo
        visible: (guiHelper.guiData.Status === 2 /* Playing */
                  || guiHelper.guiData.Status === 3 /* Paused */)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Units.dp(5)
        Layout.fillWidth : true
        Layout.fillHeight: true
        width: parent.width

        TextRadioInfo {
            id: stationTypeText
            visible: stationInfo.visible
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: Units.dp(5)
            text: guiHelper.guiData.StationType
        }

        TextRadioInfo {
            id: stationDetails
            visible: stationInfo.visible
            Layout.alignment: Qt.AlignRight
            Layout.rightMargin: Units.dp(5)
            text: radioController.bitRate + " kbps, "
                  + (radioController.isStereo ? "Stereo" : "Mono")
                  + (radioController.isDAB ? ", DAB" : ", DAB+")
        }
    }
}
