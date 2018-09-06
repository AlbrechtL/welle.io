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
            text: radioController.deviceName
        }

        TextExpert {
            name: qsTr("Current channel") + ":"
            text: radioController.channel + " (" + (radioController.frequency > 0 ? radioController.frequency/1e6 :  "N/A") + " MHz)"
        }

        GridLayout {
            columns: 3

            TextExpert {
                name: qsTr("Frame sync")
            }

            TextExpert {
                name: qsTr("FIC CRC")
            }

            TextExpert {
                name: qsTr("Frame errors")
            }

            TextExpert {
                text: radioController.isSync ? qsTr("OK") : qsTr("Not synced")
            }

            TextExpert {
                text: radioController.isFICCRC ? qsTr("OK") : qsTr("Error")
            }

            TextExpert {
                text: radioController.frameErrors
            }

            Rectangle{
                height: Units.dp(16)
                width: Units.dp(16)
                color: radioController.isSync ? "green" : "red"
            }

            Rectangle{
                height: Units.dp(16)
                width: Units.dp(16)
                color: radioController.isFICCRC ? "green" : "red"
            }

            Rectangle{
                height: Units.dp(16)
                width: Units.dp(16)
                color: (radioController.frameErrors === 0
                        && radioController.isSync
                        && radioController.isFICCRC) ? "green" : "red"
            }
        }

        TextExpert {
            name: qsTr("Frequency correction") + ":"
            text: radioController.frequencyCorrection + " Hz (" + (radioController.frequency > 0 ? radioController.frequencyCorrectionPpm.toFixed(2) : "N/A") + " ppm)"
        }

        TextExpert {
            name: qsTr("SNR") + ":"
            text: radioController.snr + " dB"
        }

        TextExpert {
            name: qsTr("RS errors") + ":"
            text: radioController.rsErrors
        }

        TextExpert {
            name: qsTr("AAC errors") + ":"
            text: radioController.aacErrors
        }

        TextExpert {
            name: qsTr("DAB date and time") + ":"
            text: radioController.dateTime.toUTCString()
        }
    }
}

