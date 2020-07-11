import QtQuick 2.0
import QtCharts 2.1
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0


// Import custom styles
import "../texts"
import "../components"

ViewBaseFrame {
    labelText: qsTr("Impulse Response")

    Settings {
        property alias isImpulseResponseWaterfall: spectrum.isWaterfall
    }

    content: Item {
        anchors.fill: parent

        WSpectrum {
            id: spectrum
            xAxisText: qsTr("Samples")
            yMin: -20
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            hoverEnabled: true
            onPositionChanged: {
                // Move text to mouse position
                xConversation.x = mouse.x + Units.dp(10)
                xConversation.y = mouse.y + Units.dp(10)

                // Get x axis value
                var mousePos = Qt.point(mouse.x, mouse.y);
                var valuePoint = spectrum.mapToValue(mousePos);
                var us = (valuePoint.x / 2.048).toPrecision(2)
                var km = (us * 3e5).toPrecision(2)

                xDisplaySample.text = "sample: " + (valuePoint.x).toFixed()
                xDisplaySecond.text = us + " µs";
                xDisplayKm.text = km + " km";
            }
        }

        Rectangle {
            id: xConversation
            width: xConvLayout.implicitWidth
            height: xConvLayout.implicitHeight
            color: "lightgrey"
            visible: !spectrum.isWaterfall

            ColumnLayout {
                anchors.margins: 10

                id: xConvLayout
                Text {
                    id: xDisplaySample
                }

                Text {
                    id: xDisplaySecond
                }

                Text {
                    id: xDisplayKm
                }
            }
        }
    }

    Connections{
        target: guiHelper

        onSetImpulseResponseAxis: {
            spectrum.yMax = Ymax
            spectrum.freqMin = Xmin
            spectrum.freqMax = Xmax
        }

        onNewDebugOutput: {
            spectrum.waterfallObject.plotMessage(text)
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
           guiHelper.updateImpulseResponse();
        }
    }

    Component.onCompleted: {
        __registerSeries();
    }

    function __registerSeries() {
       if(spectrum.isWaterfall)
           guiHelper.registerImpulseResonseWaterfall(spectrum.waterfallObject);
       else
           guiHelper.registerImpulseResonseSeries(spectrum.spectrumObject.series(0))
    }
}
