/*
 *    Copyright (C) 2017 - 2021
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
import QtQuick
import QtCharts
import QtQuick.Layouts
import QtQuick.Controls
import QtCore
import io.welle

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
                    text: "← " + (Math.round((freqMin + Number.EPSILON) * 100) / 100)
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
                    text: (Math.round((freqMax  + Number.EPSILON) * 100) / 100) + " →"
                    }
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
