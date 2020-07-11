import QtQuick 2.0
import QtCharts 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import Qt.labs.settings 1.0
import io.welle 1.0

// Import custom styles
import "../texts"
import "../components"

ColumnLayout {
    anchors.fill: parent

    property string yAxisText: qsTr("Amplitude")  + " [dB]"
    property string xAxisText: qsTr("Frequency")  + " [MHz]"
    property bool isWaterfall: false
    property real freqMin: 0
    property real freqMax: 0
    property real yMax: 0
    property real yMin: 0
    property alias waterfallObject: waterfallPlot
    property alias spectrumObject: spectrumPlot

    onYMaxChanged: {
        if(axisY.max < yMax) // Up scale y axis immediately if y should be bigger
        {
            axisY.max = yMax
        }
        else // Only for down scale
        {
            yAxisMaxTimer.running = true
            spectrumPlot.maxYAxis = yMax
        }
    }

    WSwitch {
        Layout.fillWidth: true
        text: qsTr("Waterfall") + " (" + qsTr("experimental") + ")"
        checked: isWaterfall
        onCheckedChanged: {
            if(checked)
                isWaterfall = true
            else
                isWaterfall = false
        }
    }

    RowLayout {
        id: waterfall
        visible: isWaterfall
        Layout.fillHeight: true
        Layout.fillWidth: true

        Waterfall {
            id: waterfallPlot
            sensitivity: sensitivitySlider.value
            minValue: yMin
            Layout.fillHeight: true
            Layout.fillWidth: true

            Rectangle {
                color: "#88222222"
                radius: 5
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 10
                width: minFreq.width + 10
                height: minFreq.height + 10

                Text {
                    id: minFreq
                    anchors.centerIn: parent
                    color: "#fff"
                    text: "← " + freqMin
                }
            }

            Rectangle {
                color: "#88222222"
                radius: 5
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                width: maxFreq.width + 10
                height: maxFreq.height + 10

                Text {
                    id: maxFreq
                    anchors.centerIn: parent
                    color: "#fff"
                    text: freqMax + " →"
                    }
                }
        }

        Column {
            spacing: 10

            Text {
                text: qsTr("Sensitivity")
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Slider {
                id: sensitivitySlider
                anchors.horizontalCenter: parent.horizontalCenter
                height: 180
                orientation: Qt.Vertical

                from: 0.005
                to: 0.1
                stepSize: 0.0001
                value: 0.02
            }
        }
    }

    ChartView {
        id: spectrumPlot
        visible: !isWaterfall
        Layout.fillHeight: true
        Layout.fillWidth: true
        animationOptions: ChartView.NoAnimation
        theme: ChartView.ChartThemeLight
        backgroundColor: "#00000000"
        legend.visible: false

        property real maxYAxis: 0

        Component.onCompleted: {
            var line = createSeries(ChartView.SeriesTypeLine, "line series", axisX, axisY)
            line.color = "#38ad6b"
        }

        ValueAxis {
            id: axisY
            min: yMin
            titleText: yAxisText
        }


        ValueAxis {
            id: axisX
            titleText: xAxisText
            min: freqMin
            max: freqMax
        }

        Timer {
            id: yAxisMaxTimer
            interval: 1 * 1000 // 1 s
            repeat: false
            onTriggered: {
               axisY.max = spectrumPlot.maxYAxis
            }
        }
    }

    function mapToValue(position) {
        return spectrumPlot.mapToValue(position)
    }
}
