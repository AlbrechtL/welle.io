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
            id: displayCurrentChannel
            name: "Current channel:"
        }

        TextExpert {
            id: displayFreqCorr
            name: "Frequency correction:"
        }

        TextExpert {
            id: displaySNR
            name: "SNR:"
        }

        TextExpert {
            id: displayFrameErrors
            name: "Frame errors:"
        }

        TextExpert {
            id: displayRSErrors
            name: "RS errors:"
        }

        TextExpert {
            id: displayAACErrors
            name: "AAC errors:"
        }

        TextExpert {
            id: displaySync
            name: "Frame synchronization:"
        }

        TextExpert {
            id: displayFIC_CRC
            name: "FIC CRC:"
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
        onDisplayCurrentChannel:{
            displayCurrentChannel.text = Channel + " (" + Frequency/1e6 + " MHz)"
        }

        onSignalPower:{
            displaySNR.text = power + " dB"
        }

        onDisplayFreqCorr:{
            displayFreqCorr.text = Freq + " Hz"
        }

        onDisplayFrameErrors:{
            displayFrameErrors.text = Errors
        }

        onDisplayRSErrors:{
            displayRSErrors.text = Errors
        }

        onDisplayAACErrors:{
            displayAACErrors.text = Errors
        }

        onSyncFlag:{
            if(active)
                displaySync.text = "OK"
            else
                displaySync.text = "Not synced"
        }

        onFicFlag:{
            if(active)
                displayFIC_CRC.text = "OK"
            else
            {
                displayFIC_CRC.text = "Error"
            }
        }
    }
}
