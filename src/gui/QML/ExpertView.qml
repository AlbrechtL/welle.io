import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "texts"
import "components"
import "expertviews"

Item {
    height: Units.dp(400)
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.preferredWidth: Units.dp(400)

    Settings {
        property alias enableStationInfoDisplayState: enableStationInfoDisplay.checked
        property alias enableSpectrumDisplayState: enableSpectrumDisplay.checked
        property alias enableImpulseResponseDisplayState: enableImpulseResponseDisplay.checked
        property alias enableConstellationDisplayState: enableConstellationDisplay.checked
        property alias enableNullSymbolDisplayState: enableNullSymbolDisplay.checked
    }

    function openSettings() {
        expertViewDrawer.open()
    }

    Drawer {
        id: expertViewDrawer
        edge: Qt.RightEdge
        y: overlayHeader.height
        height: mainWindow.height - overlayHeader.height

        ColumnLayout{
            spacing: Units.dp(20)

            Switch {
                id: enableStationInfoDisplay
                text: qsTr("Display station info")
                checked: true
            }

            Switch {
                id: enableSpectrumDisplay
                text: qsTr("Display spectrum")
                checked: true
            }

            Switch {
                id: enableImpulseResponseDisplay
                text: qsTr("Display impulse response")
                checked: false
            }

            Switch {
                id: enableConstellationDisplay
                text: qsTr("Display constellation diagram")
                checked: false
            }

            Switch {
                id: enableNullSymbolDisplay
                text: qsTr("Display null symbol")
                checked: false
            }
        }
    }

    Flow {
        id: layout
        anchors.fill: parent

        StationInformation {
            visible: enableStationInfoDisplay.checked
        }

        SpectrumGraph {
            visible: enableSpectrumDisplay.checked
        }

        ImpulseResponseGraph {
            visible: enableImpulseResponseDisplay.checked
        }

        ConstellationGraph {
            visible: enableConstellationDisplay.checked
        }

        NullSymbolGraph {
            visible: enableNullSymbolDisplay.checked
        }
    }
}
