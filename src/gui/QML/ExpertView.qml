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

    property bool enableStationInfoDisplay: true
    property bool enableSpectrumDisplay: true
    property bool enableImpulseResponseDisplay: false
    property bool enableConstellationDisplay: false
    property bool enableNullSymbolDisplay: false
    property bool enableConsoleOutput: false


    GridLayout {
        id: layout
        anchors.fill: parent
        flow: GridLayout.LeftToRight
        columns: (parent.width / Units.dp(400)).toFixed(0)

        StationInformation {
            Layout.alignment: Qt.AlignTop
            visible: enableStationInfoDisplay
        }

        SpectrumGraph {
            visible: enableSpectrumDisplay
        }

        ImpulseResponseGraph {
            visible: enableImpulseResponseDisplay
        }

        ConstellationGraph {
            visible: enableConstellationDisplay
        }

        NullSymbolGraph {
            visible: enableNullSymbolDisplay
        }

        TextOutputView {
            visible: enableConsoleOutput
        }
    }
}
