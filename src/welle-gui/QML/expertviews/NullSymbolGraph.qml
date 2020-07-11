import QtQuick 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "../texts"
import "../components"

ViewBaseFrame {
    labelText: qsTr("Null Symbol")

    Settings {
        property alias isNullSymbolWaterfall: spectrum.isWaterfall
    }

    content: WSpectrum {
        id: spectrum
    }

    Connections{
        target: guiHelper

        onSetNullSymbolAxis: {
            spectrum.yMax = Ymax
            spectrum.freqMin = Xmin
            spectrum.freqMax = Xmax
        }
    }

    Connections {
        target: spectrum

        onIsWaterfallChanged: {
            __registerSeries();
        }
    }

    Timer {
        id: refreshTimer
        interval: 1 / 10 * 1000 // 10 Hz
        running: parent.visible ? true : false // Trigger new data only if spectrum is showed
        repeat: true
        onTriggered: {
           guiHelper.updateNullSymbol();
        }
    }

    Component.onCompleted: {
        __registerSeries();
    }

    function __registerSeries() {
       if(spectrum.isWaterfall)
           guiHelper.registerNullSymbolWaterfall(spectrum.waterfallObject);
       else
           guiHelper.registerNullSymbolSeries(spectrum.spectrumObject.series(0))
    }
}
