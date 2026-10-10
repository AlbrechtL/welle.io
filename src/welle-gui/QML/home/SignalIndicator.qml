/*
 *    This file is part of the welle.io.
 *    This file was generated with an AI assistant (Claude Code).
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
import QtQuick.Controls.Material
import QtQuick.Controls.impl

import "../controls"

// Signal quality as five bars from the SNR, or a red antenna if the
// receiver has no signal while it plays or scans
Item {
    id: root

    readonly property real snr: radioController.snr
    readonly property int level: snr > 15 ? 5 : snr > 11 ? 4 : snr > 8 ? 3 : snr > 5 ? 2 : snr > 2 ? 1 : 0
    readonly property bool isActive: radioController.isPlaying || radioController.isChannelScan
    readonly property bool noSignal: isActive && !radioController.isSync

    implicitWidth: 24
    implicitHeight: 20

    Row {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 2
        visible: !root.noSignal

        Repeater {
            model: 5
            Rectangle {
                required property int index
                width: 3
                height: 4 * (index + 1)
                anchors.bottom: parent.bottom
                radius: 1
                color: root.level > index ? Theme.success : Material.dividerColor
            }
        }
    }

    IconImage {
        anchors.fill: parent
        visible: root.noSignal
        name: "antenna_no_signal"
        sourceSize: Qt.size(width, height)
        color: Theme.error
    }

    HoverHandler { id: hover }
    ToolTip.visible: hover.hovered
    ToolTip.delay: 500
    ToolTip.text: Accessible.name

    Accessible.role: Accessible.Indicator
    Accessible.name: noSignal ? qsTr("No signal")
                              : qsTr("Signal %1 of 5 (SNR %2 dB)").arg(level).arg(snr.toFixed(1))
}
