/*
 *    This file is part of the welle.io.
 *    Card of the expert dashboard to set the RF gain while watching the
 *    spectrum: automatic gain, manual gain and a link to the receiver
 *    settings. The settings page saves the values (ReceiverSettings.qml).
 *
 *    This file was generated with the help of an AI assistant (Claude Code).
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

DashboardCard {
    id: card

    labelText: qsTr("Receiver gain")
    cardHeight: 240

    // RAW files and the null device have no gain to set
    readonly property bool canSetGain: !radioController.agc && radioController.gainCount > 0
    // With automatic gain the slider shows the gain the AGC has set
    readonly property bool showsAgcGain: radioController.agc && radioController.deviceGain >= 0

    content: ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.spacingXS

        SettingSwitch {
            objectName: "gainCard.agc"
            leftPadding: Theme.spacingS
            rightPadding: Theme.spacingS
            text: qsTr("Automatic RF gain")
            description: radioController.gainCount > 0 ? "" : qsTr("No effect with this input device")
            checked: radioController.agc
            onClicked: {
                radioController.setAGC(checked)
                if (!checked)
                    radioController.setGain(radioController.gain)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
            spacing: Theme.spacingM
            enabled: card.canSetGain

            Label {
                Layout.fillWidth: true
                text: card.showsAgcGain ? qsTr("Automatic gain") : qsTr("Manual gain")
                elide: Text.ElideRight
                font.pixelSize: Theme.bodyLarge
                color: parent.enabled ? Material.foreground : Material.hintTextColor
            }

            Label {
                objectName: "gainCard.value"
                text: qsTr("Value: ") + (card.showsAgcGain ? radioController.deviceGainValue
                                                           : radioController.gainValue).toFixed(2)
                font.pixelSize: Theme.bodyMedium
                color: parent.enabled ? Material.secondaryTextColor : Material.hintTextColor
            }
        }

        Slider {
            id: gain
            objectName: "gainCard.gain"
            Layout.fillWidth: true
            enabled: card.canSetGain
            from: 0
            to: radioController.gainCount
            stepSize: 1
            value: card.showsAgcGain ? radioController.deviceGain : radioController.gain
            onMoved: radioController.setGain(value)

            Accessible.name: card.showsAgcGain ? qsTr("Automatic gain") : qsTr("Manual gain")
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
            objectName: "gainCard.noGain"
            visible: radioController.gainCount === 0
            text: qsTr("The input device has no adjustable gain")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.bodySmall
            color: Material.secondaryTextColor
        }

        Button {
            objectName: "gainCard.openSettings"
            text: qsTr("Receiver settings")
            icon.name: "antenna"
            flat: true
            onClicked: card.dashboard.openSettings("receiver")
        }
    }
}
