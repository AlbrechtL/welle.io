import QtQuick 2.2
import QtQuick.Layouts 1.1

// Import custom styles
import "style"

Item{
    height: Units.dp(180)
    width: Units.dp(320)
    Layout.minimumWidth: Units.dp(150)

    RowLayout{
        anchors.top: parent.top
        anchors.topMargin: Units.dp(5)
        Layout.fillWidth : true
        Layout.fillHeight: true
        width: parent.width

        RowLayout{
            Layout.fillWidth : true
            Layout.fillHeight: true
            anchors.left: parent.left
            anchors.leftMargin: Units.dp(5)
            spacing: Units.dp(2)

            Rectangle{
                id: signalBar1
                height: Units.dp(4)
                width: Units.dp(4)
                color: (cppRadioController.SNR > 2) ? "green" : "grey"
            }
            Rectangle{
                id: signalBar2
                height: Units.dp(8)
                width: Units.dp(4)
                color: (cppRadioController.SNR > 5) ? "green" : "grey"
            }
            Rectangle{
                id: signalBar3
                height: Units.dp(12)
                width: Units.dp(4)
                color: (cppRadioController.SNR > 8) ? "green" : "grey"
            }
            Rectangle{
                id: signalBar4
                height: Units.dp(16)
                width: Units.dp(4)
                color: (cppRadioController.SNR > 11) ? "green" : "grey"
            }

            Rectangle{
                id: signalBar5
                height: Units.dp(20)
                width: Units.dp(4)
                color: (cppRadioController.SNR > 15) ? "green" : "grey"
            }
        }

        TextRadioInfo {
            id: ensembleText
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.leftMargin: Units.dp(5)
            anchors.rightMargin: Units.dp(5)
        }

        /* Flags */
        RowLayout{
            id: flags
            Layout.fillWidth : true
            Layout.fillHeight: true
            anchors.right: parent.right
            anchors.rightMargin: Units.dp(5)
            spacing: 2

            Rectangle{
                id: sync
                height: Units.dp(16)
                width: Units.dp(16)
                color: cppRadioController.isSync ? "green" : "red"
            }
            Rectangle{
                id: fic
                height: Units.dp(16)
                width: Units.dp(16)
                color: cppRadioController.isFICCRC ? "green" : "red"
            }
            Rectangle{
                id: frameSucess
                height: Units.dp(16)
                width: Units.dp(16)
                color: (cppRadioController.FrameErrors === 0
                        && cppRadioController.isSync
                        && cppRadioController.isFICCRC) ? "green" : "red"
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent

        /* Station Name */
        TextRadioStation {
            id: stationTitle
            anchors.horizontalCenter: parent.horizontalCenter
        }

        /* Station Text */
        TextRadioInfo {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.margins: Units.dp(10)
            id: stationText
            Layout.maximumWidth: parent.parent.width
            width: parent.parent.width
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Connections{
        target: cppGUI

        onSetGUIData:{
            // Title
            stationTitle.text = GUIData.Title

            // Text
            stationText.text = GUIData.Text

            // Ensemble
            ensembleText.text = GUIData.Ensemble

            // Station info
            stationInfo.visible = (GUIData.Status === 2 || GUIData.Status === 3)

            // Station type
            stationTypeText.text = GUIData.StationType

            // Language type
            languageTypeText.text = GUIData.LanguageType
        }
    }

    RowLayout{
        id: stationInfo
        visible: false
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Units.dp(5)
        Layout.fillWidth : true
        Layout.fillHeight: true
        width: parent.width

        TextRadioInfo {
            id: stationTypeText
            visible: stationInfo.visible
            anchors.left: parent.left
            anchors.leftMargin: Units.dp(5)
        }

        TextRadioInfo {
            id: languageTypeText
            visible: stationInfo.visible
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.leftMargin: Units.dp(5)
            anchors.rightMargin: Units.dp(5)
        }

        TextRadioInfo {
            id: stationDetails
            visible: stationInfo.visible
            anchors.right: parent.right
            anchors.rightMargin: Units.dp(5)
            text: cppRadioController.BitRate + " kbps, "
                  + (cppRadioController.isStereo ? "Stereo" : "Mono")
                  + (cppRadioController.isDAB ? ", DAB" : ", DAB+")
        }
    }
}
