/*
 *    Copyright (C) 2017 - 2026
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
import QtQuick.Layouts
import QtQuick.Controls
import QtCore

import "../controls"

// Spectrum, impulse response or null symbol as a chart or a waterfall
// (kind: "spectrum", "impulseResponse" or "nullSymbol")
DashboardCard {
    id: card

    property string kind: "spectrum"

    // Per kind: title, settings key of the waterfall switch, x axis
    readonly property var config: ({
        spectrum: { title: qsTr("Spectrum"), waterfallKey: "isSpectrumWaterfall",
                    xAxisText: qsTr("Frequency") + " [MHz]", yMin: 0 },
        impulseResponse: { title: qsTr("Impulse Response"), waterfallKey: "isImpulseResponseWaterfall",
                           xAxisText: qsTr("Samples"), yMin: -20 },
        nullSymbol: { title: qsTr("Null Symbol"), waterfallKey: "isNullSymbolWaterfall",
                      xAxisText: qsTr("Frequency") + " [MHz]", yMin: 0 }
    })[kind]

    labelText: config.title

    Settings {
        id: settings
    }

    content: Item {
        anchors.fill: parent

        SpectrumChart {
            id: chart
            xAxisText: card.config.xAxisText
            yMin: card.config.yMin
            isWaterfall: String(settings.value(card.config.waterfallKey, false)) === "true"
            onIsWaterfallChanged: {
                settings.setValue(card.config.waterfallKey, isWaterfall)
                card.registerSeries()
            }
        }

        // Impulse response: delay and distance at the mouse position
        MouseArea {
            anchors.fill: parent
            enabled: card.kind === "impulseResponse" && !chart.isWaterfall
            acceptedButtons: Qt.NoButton
            hoverEnabled: true
            onPositionChanged: (mouse) => {
                delayInfo.x = mouse.x + 10
                delayInfo.y = mouse.y + 10
                // 2.048 samples per µs, light travels 0.3 km per µs
                var value = chart.mapToValue(Qt.point(mouse.x, mouse.y))
                var us = value.x / 2.048
                delayInfo.text = "sample: " + value.x.toFixed() + "\n" + us.toFixed(1) + " µs\n" + (us * 0.3).toFixed(1) + " km"
            }
            onExited: delayInfo.text = ""
        }

        Label {
            id: delayInfo
            objectName: "delayInfo"
            visible: text !== "" && card.kind === "impulseResponse" && !chart.isWaterfall
            padding: Theme.spacingS
            background: Rectangle {
                color: Theme.inverseSurface
                opacity: 0.85
                radius: Theme.radiusS
            }
            color: Theme.inverseOnSurface
        }
    }

    Connections {
        target: guiHelper

        function onSetSpectrumAxis(yMax, xMin, xMax) { if (card.kind === "spectrum") card.setAxis(yMax, xMin, xMax) }
        function onSetImpulseResponseAxis(yMax, xMin, xMax) { if (card.kind === "impulseResponse") card.setAxis(yMax, xMin, xMax) }
        function onSetNullSymbolAxis(yMax, xMin, xMax) { if (card.kind === "nullSymbol") card.setAxis(yMax, xMin, xMax) }
    }

    // New data at 10 Hz, only while the card is shown
    Timer {
        interval: 100
        running: card.visible
        repeat: true
        onTriggered: {
            switch (card.kind) {
            case "spectrum": guiHelper.updateSpectrum(); break
            case "impulseResponse": guiHelper.updateImpulseResponse(); break
            case "nullSymbol": guiHelper.updateNullSymbol(); break
            }
        }
    }

    Component.onCompleted: registerSeries()

    function setAxis(yMax, xMin, xMax) {
        chart.yMax = yMax
        chart.freqMin = xMin
        chart.freqMax = xMax
    }

    // The chart or the waterfall gets the data
    function registerSeries() {
        var series = chart.spectrumObject.series(0)
        switch (kind) {
        case "spectrum":
            if (chart.isWaterfall) guiHelper.registerSpectrumWaterfall(chart.waterfallObject)
            else guiHelper.registerSpectrumSeries(series)
            break
        case "impulseResponse":
            if (chart.isWaterfall) guiHelper.registerImpulseResonseWaterfall(chart.waterfallObject)
            else guiHelper.registerImpulseResonseSeries(series)
            break
        case "nullSymbol":
            if (chart.isWaterfall) guiHelper.registerNullSymbolWaterfall(chart.waterfallObject)
            else guiHelper.registerNullSymbolSeries(series)
            break
        }
    }
}
