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
            name: qsTr("Device") + ":"
            text: guiHelper.guiData.DeviceName
        }

        TextExpert {
            name: qsTr("Current channel") + ":"
            text: guiHelper.guiData.Channel + " (" + (guiHelper.guiData.Frequency > 0 ? guiHelper.guiData.Frequency/1e6 :  "N/A") + " MHz)"
        }

        TextExpert {
            name: qsTr("Frequency correction") + ":"
            text: radioController.FrequencyCorrection + " Hz (" + (guiHelper.guiData.Frequency > 0 ? radioController.FrequencyCorrectionPpm.toFixed(2) : "N/A") + " ppm)"
        }

        TextExpert {
            name: qsTr("SNR") + ":"
            text: radioController.SNR + " dB"
        }

        TextExpert {
            name: qsTr("Frame errors") + ":"
            text: radioController.FrameErrors
        }

        TextExpert {
            name: qsTr("RS errors") + ":"
            text: radioController.RSErrors
        }

        TextExpert {
            name: qsTr("AAC errors") + ":"
            text: radioController.AACErrors
        }

        TextExpert {
            name: qsTr("Frame synchronization") + ":"
            text: radioController.isSync ? qsTr("OK") : qsTr("Not synced")
        }

        TextExpert {
            name: qsTr("FIC CRC") + ":"
            text: radioController.isFICCRC ? qsTr("OK") : qsTr("Error")
        }

        TextExpert {
            name: qsTr("DAB date and time") + ":"
            text: radioController.DateTime
        }
    }
}

