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
import QtCore


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

        function onSetImpulseResponseAxis(Ymax, Xmin, Xmax) {
            spectrum.yMax = Ymax
            spectrum.freqMin = Xmin
            spectrum.freqMax = Xmax
        }

//        onNewDebugOutput: {
//            spectrum.waterfallObject.plotMessage(text)
//        }
    }

    Connections {
        target: spectrum

        function onIsWaterfallChanged() {
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
