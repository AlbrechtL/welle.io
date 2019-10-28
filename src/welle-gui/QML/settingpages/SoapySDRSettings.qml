import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2
import Qt.labs.settings 1.0

// Import custom styles
import "../texts"
import "../components"

SettingSection {
    text: qsTr("SoapySDR settings")
    spacing: Units.dp(20)

    Settings {
        property alias soapyAntenna: antenna.text
        property alias soapyClockSource: clockSource.text
        property alias soapyDriverArgs: driverArgs.text
    }

    ColumnLayout {
        spacing: Units.dp(20)
        RowLayout {
            TextStandart {
                text: qsTr("Antenna")
                Layout.fillWidth: true
            }

            TextField {
                id: antenna
                placeholderText: qsTr("Enter antenna")
            }
        }

        RowLayout {
            TextStandart {
                text: qsTr("Clock source")
                Layout.fillWidth: true
            }

            TextField {
                id: clockSource
                placeholderText: qsTr("Enter clock source")
            }
        }

        RowLayout {
            TextStandart {
                text: qsTr("Driver arguments")
                Layout.fillWidth: true
            }

            TextField {
                id: driverArgs
                placeholderText: qsTr("Enter driver arguments")
            }
        }

        WButton {
            id: applyButton
            text: qsTr("Apply")
            Layout.fillWidth: true
            onClicked: {
                __setParams()
            }
        }
    }

    function initDevice(isAutoDevice) {
        __setParams()
        if(!isAutoDevice)
            guiHelper.openSoapySdr()
    }

    function __setParams() {
        guiHelper.setDriverArgsSoapySdr(driverArgs.text)
        guiHelper.setAntennaSoapySdr(antenna.text)
        guiHelper.setClockSourceSoapySdr(clockSource.text)
    }
}
