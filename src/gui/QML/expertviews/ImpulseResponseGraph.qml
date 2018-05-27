import QtQuick 2.0
import QtCharts 2.1

// Import custom styles
import "../texts"
import "../components"

Item {
    height: Units.dp(400)
    width: Units.dp(400)

    ChartView {
        id: chart
        anchors.fill: parent

        animationOptions: ChartView.NoAnimation
        theme: ChartView.ChartThemeLight
        backgroundColor: "#00000000"
        legend.visible: false
        title: qsTr("Impulse Response")

        property real maxYAxis: 0

        Component.onCompleted: {
            var line = createSeries(ChartView.SeriesTypeLine, "line series", axisX, axisY)
            line.color = "#38ad6b"
            cppGUI.registerImpulseResonseSeries(series(0));
        }

        Connections{
            target: cppGUI

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
            titleText: qsTr("DQPSK Angle [Degree]")
            min: -20
        }

        ValueAxis {
            id: axisX
            titleText: qsTr("Subcarrier")
        }

        Timer {
            id: refreshTimer
            interval: 1 / 10 * 1000 // 10 Hz
            running: parent.visible ? true : false // Trigger new data only if spectrum is showed
            repeat: true
            onTriggered: {
               cppGUI.updateImpulseResponse();
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
    }
}
