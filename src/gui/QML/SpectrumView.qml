import QtQuick 2.0
import QtCharts 2.1

// Import custom styles
import "style"

ChartView {
    id: spectrumView
    animationOptions: ChartView.NoAnimation
    theme: ChartView.ChartThemeDark
    backgroundColor: "#00000000"
    //titleColor: "white"
    legend.visible: false
    title: qsTr("Spectrum")

    property real maxYAxis: 0
    property int plotType: 0

    onPlotTypeChanged: {
        switch(plotType) {
        case 0:
            title = qsTr("Spectrum");
            axisX.titleText = qsTr("Frequency") + " [MHz]";
            axisY1.titleText = qsTr("Amplitude")
            break;
        case 1:
            title = qsTr("Impulse Response");
            axisX.titleText = qsTr("Samples");
            axisY1.titleText = qsTr("Amplitude");
            break;
        case 2:
            title = qsTr("Constellation Diagram (not implemented)");
            axisX.titleText = qsTr("Q");
            axisY1.titleText = qsTr("I");
            break;
        case 3:
            title = qsTr("Null Symbol (not implemented)");
            axisX.titleText = qsTr("Frequency") + " [MHz]";
            axisY1.titleText = qsTr("Amplitude");
            break;
        }


    }

    Connections{
        target: cppGUI
        onSetYAxisMax:{
            if(axisY1.max < max) // Up scale y axis emidetly if y should be bigger
            {
                axisY1.max = max
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
        cppGUI.registerSpectrumSeries(spectrumView.series(0));
    }

    ValueAxis {
        id: axisY1
        min: 0
        titleText: qsTr("Amplitude")
    }


    ValueAxis {
        id: axisX
        titleText: qsTr("Frequency") + " [MHz]"
    }

    LineSeries {
        id: lineSeries1
        axisX: axisX
        axisY: axisY1
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
           axisY1.max = maxYAxis
        }
    }
}
