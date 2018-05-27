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
        property alias enableSpectrumState: enableSpectrum.checked
        property alias enableStationInfoState: enableStationInfo.checked
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
                id: enableSpectrum
                text: qsTr("Display spectrum")
            }

            Switch {
                id: enableStationInfo
                text: qsTr("Display station info")
            }
        }
    }

    Flow {
        id: layout
        anchors.fill: parent

        StationInformation {
            visible: enableStationInfo.checked
        }

        Spectrum {
            visible: enableSpectrum.checked
        }

        ImpulseResponseGraph {
            visible: true
        }

        ConstellationGraph {
            visible: true
        }

        NullSymbolGraph {
            visible: true
        }
    }
}
