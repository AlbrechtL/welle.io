import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "texts"
import "components"
import "expertviews"

Item {
    implicitHeight: layout.implicitHeight
    Layout.preferredHeight: Units.dp(400)
    Layout.preferredWidth: Units.dp(400)
    Layout.fillWidth: true
    Layout.fillHeight: true

    Settings {
        property alias enableStationInfoDisplayState: enableStationInfoDisplay.checked
        property alias enableSpectrumDisplayState: enableSpectrumDisplay.checked
        property alias enableImpulseResponseDisplayState: enableImpulseResponseDisplay.checked
        property alias enableConstellationDisplayState: enableConstellationDisplay.checked
        property alias enableNullSymbolDisplayState: enableNullSymbolDisplay.checked
    }

    function openSettings() {        
        // Workaround for touch displays. (Discovered with Windows 10)
        // For some reason the dawer will be closed before it is openend
        // Enable the closing again
        expertViewDrawer.closePolicy = Popup.NoAutoClose

        // Open drawer
        expertViewDrawer.open()
    }

    Drawer {
        id: expertViewDrawer
        edge: Qt.RightEdge
        y: overlayHeader.height
        height: mainWindow.height - overlayHeader.height

        // Workaround for touch displays. (Discovered with Windows 10)
        // For some reason the dawer will be closed before it is openend
        // Enable the closing again
        onOpened: closePolicy = Popup.CloseOnEscape | Popup.CloseOnPressOutside

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

    GridLayout {
        id: layout
        anchors.fill: parent
        flow: GridLayout.LeftToRight
        columns: (parent.width / Units.dp(400)).toFixed(0)

        StationInformation {
            Layout.alignment: Qt.AlignTop
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
