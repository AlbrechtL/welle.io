import QtQuick 2.2
import QtQuick.Layouts 1.1

// Import custom styles
import "style"

Item {
    height: Units.dp(400)
    Layout.fillWidth: true
   // Layout.fillHeight: true
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

        RowLayout {
            SpectrumView {
                id: plot
                Layout.preferredWidth: Units.dp(200)
                Layout.preferredHeight: Units.dp(200)
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            ColumnLayout {
                anchors.topMargin: 10
                anchors.right: parent.right
                anchors.top: parent.top
                TouchButton {
                    id: buttonSpec
                    text: qsTr("Spectrum")
                    onClicked: {
                        cppGUI.setPlotType(0);
                        plot.plotType = 0;
                    }
                }
                TouchButton {
                    id: buttonCIR
                    text: qsTr("CIR")
                    onClicked: {
                        cppGUI.setPlotType(1);
                        plot.plotType = 1;
                    }
                }
                TouchButton {
                    id: buttonQPSK
                    text: qsTr("QPSK")
                    onClicked: {
                        cppGUI.setPlotType(2);
                        plot.plotType = 2;
                    }
                }
                TouchButton {
                    id: buttonNull
                    text: qsTr("Null")
                    onClicked: {
                        cppGUI.setPlotType(3);
                        plot.plotType = 3;
                    }
                }
            }
        }
    }
}
