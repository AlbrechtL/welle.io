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
                color: "grey"
            }
            Rectangle{
                id: signalBar2
                height: Units.dp(8)
                width: Units.dp(4)
                color: "grey"
            }
            Rectangle{
                id: signalBar3
                height: Units.dp(12)
                width: Units.dp(4)
                color: "grey"
            }
            Rectangle{
                id: signalBar4
                height: Units.dp(16)
                width: Units.dp(4)
                color: "grey"
            }

            Rectangle{
                id: signalBar5
                height: Units.dp(20)
                width: Units.dp(4)
                color: "grey"
            }
        }

        TextRadioInfo {
            id: bitrateText
            text: "96 kbps"
        }

        TextRadioInfo {
            id: dabTypeText
            text: "DAB+"
        }

        TextRadioInfo {
            id: audioTypeText
            text: "Stereo"
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
                id: signal
                height: Units.dp(16)
                width: Units.dp(16)
                color: "red"
            }
            Rectangle{
                id: sync
                height: Units.dp(16)
                width: Units.dp(16)
                color: "red"
            }
            Rectangle{
                id: fic
                height: Units.dp(16)
                width: Units.dp(16)
                color: "red"
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent

        /* Station Name */
        TextRadioStation {
            id: currentStation
            anchors.horizontalCenter: parent.horizontalCenter
            text: "No Station"
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
        onCurrentStation:{
            currentStation.text = text
        }

        onStationText:{
            stationText.text = text
        }

        onSignalFlag:{
            if(active)
                signal.color = "green"
            else
                signal.color = "red"
        }

        onSyncFlag:{
            if(active)
                sync.color = "green"
            else
                sync.color = "red"
        }

        onFicFlag:{
            if(active)
                fic.color = "green"
            else
                fic.color = "red"
        }

        onBitrate: {
            bitrateText.text = bitrate +  " kbps"
        }

        onDabType: {
            dabTypeText.text = text
        }

        onAudioType: {
            if(isStereo)
                audioTypeText.text = "Stereo"
            else
                audioTypeText.text = "Mono"
        }

        onStationType: {
            stationTypeText.text = text
        }

        onLanguageType: {
            languageTypeText.text = text
        }

        onSignalPower: {
            if(power > 15) signalBar5.color = "green"; else signalBar5.color = "grey"
            if(power > 11) signalBar4.color = "green"; else signalBar4.color = "grey"
            if(power > 8) signalBar3.color = "green"; else signalBar3.color = "grey"
            if(power > 5) signalBar2.color = "green"; else signalBar2.color = "grey"
            if(power > 2) signalBar1.color = "green"; else signalBar1.color = "grey"
        }
    }

    RowLayout{
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Units.dp(5)
        Layout.fillWidth : true
        Layout.fillHeight: true
        width: parent.width

        TextRadioInfo {
            id: languageTypeText
            anchors.left: parent.left
            anchors.leftMargin: Units.dp(5)
            text: "German"
        }

        TextRadioInfo {
            id: stationTypeText
            anchors.right: parent.right
            anchors.rightMargin: Units.dp(5)
            text: "Information"
        }

    }

}
