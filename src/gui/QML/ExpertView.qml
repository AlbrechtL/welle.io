import QtQuick 2.2
import QtQuick.Layouts 1.1

// Import custom styles
import "style"

Item {
    height: Units.dp(400)
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.preferredWidth: Units.dp(400)

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Units.dp(5)

        TextExpert {
            id: displayDeviceName
            name: qsTr("Device") + ":"
            text: cppGUI.guiData.DeviceName
        }

        TextExpert {
            id: displayCurrentChannel
            name: qsTr("Current channel") + ":"
            text: cppGUI.guiData.Channel + " (" + cppGUI.guiData.Frequency/1e6 + " MHz)"
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
            text: cppRadioController.isSync ? qsTr("OK") : qsTr("Not synced")
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
}
