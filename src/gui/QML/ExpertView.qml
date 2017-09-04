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
            text: cppRadioController.FrequencyCorrection + " Hz"
        }

        TextExpert {
            id: displaySNR
            name: qsTr("SNR") + ":"
            text: cppRadioController.SNR + " dB"
        }

        TextExpert {
            id: displayFrameErrors
            name: qsTr("Frame errors") + ":"
            text: cppRadioController.FrameErrors
        }

        TextExpert {
            id: displayRSErrors
            name: qsTr("RS errors") + ":"
            text: cppRadioController.RSErrors
        }

        TextExpert {
            id: displayAACErrors
            name: qsTr("AAC errors") + ":"
            text: cppRadioController.AACErrors
        }

        TextExpert {
            id: displaySync
            name: qsTr("Frame synchronization") + ":"
            text: cppRadioController.isSync ? displaySync.text = qsTr("OK")
                                            : displaySync.text = qsTr("Not synced")
        }

        TextExpert {
            id: displayFIC_CRC
            name: qsTr("FIC CRC") + ":"
            text: cppRadioController.isFICCRC ? qsTr("OK") : qsTr("Error")
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

            // Device name
            displayDeviceName.text = GUIData.DeviceName
        }
    }
}
