import QtQuick 2.0
import QtCharts 2.1
import QtQuick.Layouts 1.1

// Import custom styles
import "../texts"
import "../components"

ViewBaseFrame {
    labelText: qsTr("Impulse Response")

    content: ChartView {
        id: chart
        anchors.fill: parent
        animationOptions: ChartView.NoAnimation
        theme: ChartView.ChartThemeLight
        backgroundColor: "#00000000"
        legend.visible: false

        property real maxYAxis: 0

        Component.onCompleted: {
            var line = createSeries(ChartView.SeriesTypeLine, "line series", axisX, axisY)
            line.color = "#38ad6b"
            guiHelper.registerImpulseResonseSeries(series(0));
        }

        Connections{
            target: guiHelper

            onSetImpulseResponseAxis: {
                if(axisY.max < Ymax) // Up scale y axis immediately if y should be bigger
                {
                    axisY.max = Ymax
                }
                else // Only for down scale
                {
                    yAxisMaxTimer.running = true
                    chart.maxYAxis = Ymax
                }

                axisX.min = Xmin
                axisX.max = Xmax
            }
        }

        ValueAxis {
            id: axisY
            titleText: qsTr("Amplitude")
            min: -20
        }

        ValueAxis {
            id: axisX
            titleText: qsTr("Samples")
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

        Timer {
            id: yAxisMaxTimer
            interval: 1 * 1000 // 1 s
            repeat: false
            onTriggered: {
               axisY.max = chart.maxYAxis
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onPositionChanged: {
                // Move text to mouse position
                xConversation.x = mouse.x + Units.dp(10)
                xConversation.y = mouse.y + Units.dp(10)

                // Get x axis value
                var mousePos = Qt.point(mouse.x, mouse.y);
                var valuePoint = chart.mapToValue(mousePos);
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
}
