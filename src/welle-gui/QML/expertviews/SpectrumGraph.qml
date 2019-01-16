import QtQuick 2.0
import QtCharts 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import Qt.labs.settings 1.0
import io.welle 1.0

// Import custom styles
import "../texts"
import "../components"

ViewBaseFrame {
    id: test
    labelText: qsTr("Spectrum")

    property bool isWaterfall: false
    property real freqMin: 0
    property real freqMax: 0

    Settings {
        property alias isSpectrumWaterfall: test.isWaterfall
    }

    content:
        ColumnLayout {
            anchors.fill: parent

            WSwitch {
                Layout.fillWidth: true
                text: qsTr("Waterfall") + " (experimental)"
                checked: isWaterfall
                onCheckedChanged: {
                    if(checked) {
                        isWaterfall = true
                        __registerSeries()
                    }
                    else {
                        isWaterfall = false
                    }
                }
            }

            RowLayout {
                id: waterfall
                visible: isWaterfall
                Layout.fillHeight: true
                Layout.fillWidth: true

                Waterfall {
                id: plot
                sensitivity: sensitivitySlider.value
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
                        text: "← " + freqMin + " Hz"
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
                        text: freqMax + " Hz →"
                    }
                }
            }

            Column {
                spacing: 10

                Text {
                    text: "Sensitivity"
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
            id: chart
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
                min: 0
                titleText: qsTr("Amplitude")
            }


            ValueAxis {
                id: axisX
                titleText: qsTr("Frequency") + " [MHz]"
                min: freqMin
                max: freqMax
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

    Connections{
        target: guiHelper

        onSetSpectrumAxis: {
            if(axisY.max < Ymax) // Up scale y axis immediately if y should be bigger
            {
                axisY.max = Ymax
            }
            else // Only for down scale
            {
                yAxisMaxTimer.running = true
                chart.maxYAxis = Ymax
            }

            freqMin = Xmin
            freqMax = Xmax
        }

        onNewDebugOutput: {
            plot.plotMessage(text)
        }
    }

    Connections{
        target: radioController

//        onIsSyncChanged: {
//            if(radioController.isSync)
//                plot.plotMessage(qsTr("Sync found"))
//            else
//                plot.plotMessage(qsTr("Sync lost"))
//        }

//        onIsFICCRCChanged: {
//            if(radioController.isFICCRC)
//                plot.plotMessage(qsTr("CRC error"))
//        }


//        onFrameErrorsChanged: {
//            if(radioController.frameErrors > 0)
//                plot.plotMessage(qsTr("Frame errors: " + radioController.frameErrors))
//        }
    }

    Timer {
        id: refreshTimer
        interval: 1 / 10 * 1000 // 10 Hz
        running: parent.visible ? true : false // Trigger new data only if spectrum is showed
        repeat: true
        onTriggered: {
           guiHelper.updateSpectrum();
        }
    }

    Component.onCompleted: {
        __registerSeries();
    }

    function __registerSeries() {
       if(isWaterfall)
           guiHelper.registerWaterfall(plot);
       else
           guiHelper.registerSpectrumSeries(chart.series(0))
    }
}
