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

// Import custom styles
import "../texts"
import "../components"

ViewBaseFrame {
    labelText: qsTr("Constellation Diagram")

    content: ChartView {
        id: chart
        anchors.fill: parent
        animationOptions: ChartView.NoAnimation
        theme: ChartView.ChartThemeLight
        backgroundColor: "#00000000"
        legend.visible: false

        property real maxYAxis: 0

        Component.onCompleted: {
            var scatter = createSeries(ChartView.SeriesTypeScatter, "scatter series", axisX, axisY)
            scatter.markerSize = 1.0
            scatter.borderColor = "#38ad6b";
            guiHelper.registerConstellationSeries(series(0));
        }

        Connections{
            target: guiHelper

            function onSetConstellationAxis(Xmin, Xmax) {
                axisX.min = Xmin
                axisX.max = Xmax
            }
        }

        ValueAxis {
            id: axisY
            titleText: qsTr("DQPSK Angle [Degree]")
            max: 180
            min: -180
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
               guiHelper.updateConstellation();
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
