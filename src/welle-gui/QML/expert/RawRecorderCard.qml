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
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material


import "../controls"

DashboardCard {
    labelText: qsTr("I/Q RAW Recorder (experimental)")

    cardHeight: 200

    property bool isStart: false
    property int ringeBufferSize: 0

    content: ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.spacingS

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingM

            Label {
                Layout.fillWidth: true
                text: qsTr("Ring buffer length [s]")
                wrapMode: Text.Wrap
            }

            ComboBox {
                id: ringeBufferSetting
                model: [5, 10, 60, 120, 240]
                enabled: !isStart
                popup.Material.elevation: Theme.elevation(8)
                onCurrentIndexChanged: ringeBufferSize = model[currentIndex] * 2048 * 1024
                Component.onCompleted: ringeBufferSize = model[currentIndex] * 2048 * 1024

                Accessible.name: qsTr("Ring buffer length [s]")
            }
        }

        Label {
            text: qsTr("Ring buffer size (roughly): ") + (ringeBufferSize / 1000000 * 2).toFixed(0) + " MB"
            color: Material.secondaryTextColor
        }

        Button {
            objectName: "recorderButton"
            text: isStart ? qsTr("Save ring buffer") : qsTr("Init")
            highlighted: true
            Material.elevation: 0

            onClicked: {
                if(!isStart)
                    radioController.initRecorder(ringeBufferSize)
                else
                    radioController.triggerRecorder("")

                isStart = !isStart
            }
        }
    }
}
