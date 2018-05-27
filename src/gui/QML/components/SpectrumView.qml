import QtQuick 2.0
import QtCharts 2.1


// Import custom styles
import "../texts"

ChartView {
    id: spectrumView
    animationOptions: ChartView.NoAnimation
    theme: ChartView.ChartThemeLight
    backgroundColor: "#00000000"
    //titleColor: "white"
    legend.visible: false
    title: qsTr("Spectrum")

    property real maxYAxis: 0
    property int plotType: 0

    onPlotTypeChanged: {
        axisY.min = 0;
        switch(plotType) {
        case 0:
            title = qsTr("Spectrum")
            axisX.titleText = qsTr("Frequency") + " [MHz]"
            axisY.titleText = qsTr("Amplitude")
            break;
        case 1:
            title = qsTr("Impulse Response")
            axisX.titleText = qsTr("Samples")
            axisY.titleText = qsTr("Amplitude")
            axisY.min = -20
            break;
        case 2:
            title = qsTr("Constellation Diagram")
            axisX.titleText = qsTr("Subcarrier")
            axisY.titleText = qsTr("DQPSK Angle [Degree]")
            axisY.min = -180
            break;
        case 3:
            title = qsTr("Null Symbol")
            axisX.titleText = qsTr("Frequency") + " [MHz]"
            axisY.titleText = qsTr("Amplitude")
            break;
        }

        if(plotType != 2) {
            removeAllSeries()
            var line = createSeries(ChartView.SeriesTypeLine, "line series", axisX, axisY)
            line.color = "#38ad6b"
            cppGUI.registerSpectrumSeries(spectrumView.series(0));
        }
        else {
            removeAllSeries()
            var scatter = createSeries(ChartView.SeriesTypeScatter, "scatter series", axisX, axisY)
            scatter.markerSize = 1.0
            scatter.borderColor = "#38ad6b";
            cppGUI.registerSpectrumSeries(spectrumView.series(0));
        }
    }

    Connections{
        target: cppGUI
        onSetYAxisMax:{
            if(axisY.max < max) // Up scale y axis immediately if y should be bigger
            {
                axisY.max = max
            }
            else // Only for down scale
            {
                yAxisMaxTimer.running = true
                maxYAxis = max
            }
        }

        onSetXAxisMinMax:{
            axisX.min = min
            axisX.max = max
        }
    }

    Component.onCompleted: {
        plotTypeChanged(0);
    }

    ValueAxis {
        id: axisY
        min: 0
        titleText: qsTr("Amplitude")
    }


    ValueAxis {
        id: axisX
        titleText: qsTr("Frequency") + " [MHz]"
    }

    Timer {
        id: refreshTimer
        interval: 1 / 10 * 1000 // 10 Hz
        running: parent.visible ? true : false // Trigger new data only if spectrum is showed
        repeat: true
        onTriggered: {
           cppGUI.updateSpectrum();
        }
    }

    Timer {
        id: yAxisMaxTimer
        interval: 1 * 1000 // 1 s
        repeat: false
        onTriggered: {
           axisY.max = maxYAxis
        }
    }
}
