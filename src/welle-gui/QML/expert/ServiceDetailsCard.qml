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
import QtQuick.Controls.Material

import "../controls"

// Technical details of the receiver and the service: device, channel,
// frequency correction, SNR and the error counters with a status color
DashboardCard {
    id: card
    labelText: qsTr("Service Details")
    cardHeight: 360

    readonly property bool rsOk: radioController.rsCorrectedErrors === 0 && radioController.rsUncorrectedErrors === 0
    readonly property bool rsCorrected: radioController.rsCorrectedErrors >= 0 && radioController.rsUncorrectedErrors === 0

    content: Flickable {
        anchors.fill: parent
        contentHeight: details.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: details
            width: parent.width
            spacing: Theme.spacingXS

            Detail { name: qsTr("Device"); value: radioController.deviceName }
            Detail {
                name: qsTr("Current channel")
                value: radioController.channel + " (" + (radioController.frequency > 0 ? radioController.frequency / 1e6 : "N/A") + " MHz)"
            }
            Detail {
                name: qsTr("Frequency correction")
                value: radioController.frequencyCorrection + " Hz ("
                       + (radioController.frequency > 0 ? radioController.frequencyCorrectionPpm.toFixed(2) : "N/A") + " ppm)"
            }
            Detail { name: qsTr("SNR"); value: radioController.snr.toFixed(2) + " dB" }
            Detail {
                name: qsTr("Frame sync")
                value: radioController.isSync ? qsTr("OK") : qsTr("Not synced")
                status: radioController.isSync ? Theme.success : Theme.error
            }
            Detail {
                name: qsTr("FIC CRC")
                value: radioController.isFICCRC ? qsTr("OK") : qsTr("Error")
                status: radioController.isFICCRC ? Theme.success : Theme.error
            }
            Detail {
                name: qsTr("Frame errors")
                value: radioController.frameErrors
                status: (radioController.frameErrors === 0 && radioController.isSync && radioController.isFICCRC) ? Theme.success : Theme.error
            }
            Detail {
                name: qsTr("RS errors")
                value: card.rsOk ? qsTr("OK") : card.rsCorrected ? qsTr("Corrected Error") : qsTr("Uncorrected Error")
                status: card.rsOk ? Theme.success : card.rsCorrected ? Theme.warning : Theme.error
            }
            Detail {
                name: qsTr("AAC errors")
                value: radioController.aacErrors
                status: radioController.aacErrors === 0 ? Theme.success : Theme.error
            }
            Detail { name: qsTr("Ensemble ID"); value: "0x" + radioController.ensembleId.toString(16) }
            Detail { name: qsTr("DAB date and time"); value: radioController.dateTime.toString() }
        }
    }

    // One detail: name, a status dot (if status is set) and the value
    component Detail: RowLayout {
        id: detail

        property string name
        property string value
        property color status: "transparent"

        Layout.fillWidth: true
        spacing: Theme.spacingS

        Label {
            // Shorter on narrow cards (not bound to the width of the row,
            // the layout would arrange itself again and again)
            Layout.preferredWidth: 170
            Layout.minimumWidth: 80
            text: detail.name + ":"
            elide: Text.ElideRight
            color: Material.secondaryTextColor
        }

        Rectangle {
            visible: detail.status.a > 0
            implicitWidth: 12
            implicitHeight: 12
            radius: 6
            color: detail.status
        }

        Label {
            Layout.fillWidth: true
            text: detail.value
            elide: Text.ElideRight
        }
    }
}
