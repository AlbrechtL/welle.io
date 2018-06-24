import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "../texts"
import "../components"

Item {
     Layout.preferredHeight: layout.height
     Layout.preferredWidth: layout.width

    ColumnLayout {
        id: layout

        TextExpert {
            id: displayDeviceName
            name: qsTr("Device") + ":"
            text: cppGUI.guiData.DeviceName
        }

        TextExpert {
            id: displayCurrentChannel
            name: qsTr("Current channel") + ":"
            text: cppGUI.guiData.Channel + " (" + (cppGUI.guiData.Frequency > 0 ? cppGUI.guiData.Frequency/1e6 :  "N/A") + " MHz)"
        }

        TextExpert {
            id: displayFreqCorr
            name: qsTr("Frequency correction") + ":"
            text: cppRadioController.FrequencyCorrection + " Hz (" + (cppGUI.guiData.Frequency > 0 ? cppRadioController.FrequencyCorrectionPpm.toFixed(2) : "N/A") + " ppm)"
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
    }
}

