import QtQuick 2.2
import QtQuick.Layouts 1.1

// Import custom styles
import "style"

Item {
    width: Units.dp(400)
    height: Units.dp(400)
    Layout.fillWidth: true
    Layout.fillHeight: true

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Units.dp(5)
        anchors.leftMargin: Units.dp(5)

        TextExpert {
            id: displayDeviceName
            name: qsTr("Device") + ":"
        }

        TextExpert {
            id: displayCurrentChannel
            name: qsTr("Current channel") + ":"
        }

        TextExpert {
            id: displayFreqCorr
            name: qsTr("Frequency correction") + ":"
        }

        TextExpert {
            id: displaySNR
            name: qsTr("SNR") + ":"
        }

        TextExpert {
            id: displayFrameErrors
            name: qsTr("Frame errors") + ":"
        }

        TextExpert {
            id: displayRSErrors
            name: qsTr("RS errors") + ":"
        }

        TextExpert {
            id: displayAACErrors
            name: qsTr("AAC errors") + ":"
        }

        TextExpert {
            id: displaySync
            name: qsTr("Frame synchronization") + ":"
        }

        TextExpert {
            id: displayFIC_CRC
            name: qsTr("FIC CRC") + ":"
        }

        SpectrumView {
            Layout.preferredWidth: Units.dp(200)
            Layout.preferredHeight: Units.dp(200)
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    Connections{
        target: cppGUI

        onSetGUIData:{
            // Channel
            displayCurrentChannel.text = GUIData.Channel + " (" + GUIData.Frequency/1e6 + " MHz)"

            // SNR
            displaySNR.text = GUIData.SNR + " dB"

            // Frequency correction
            displayFreqCorr.text = GUIData.FrequencyCorrection + " Hz"

            // Frame errors
            displayFrameErrors.text = GUIData.FrameErrors

            // RS errors
            displayRSErrors.text = GUIData.RSErrors

            // AAC errors
            displayAACErrors.text = GUIData.AACErrors

            // Sync flag
            if(GUIData.isSync)
                displaySync.text = qsTr("OK")
            else
                displaySync.text = qsTr("Not synced")

            // FIC flag
            if(GUIData.isFICCRC)
                displayFIC_CRC.text = qsTr("OK")
            else
                displayFIC_CRC.text = qsTr("Error")

            // Device name
            displayDeviceName.text = GUIData.DeviceName
        }
    }
}
