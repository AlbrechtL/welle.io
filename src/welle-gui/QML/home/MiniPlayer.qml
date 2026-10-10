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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Controls.impl

import "../app"
import "../controls"
import "../models"

// Bar at the bottom of the phone layout with the station that plays.
// A click opens the full player (or the expert views).
Pane {
    id: root

    signal openPlayer

    property MotSlideModel slides
    property Player player

    padding: 0
    Material.elevation: Theme.elevation(3)

    // Edge to the list if there is no shadow
    Rectangle {
        z: 1
        width: parent.width
        height: 1
        visible: !Theme.effects
        color: Material.dividerColor
    }

    ItemDelegate {
        id: delegate
        objectName: "miniPlayer"
        anchors.fill: parent
        padding: Theme.spacingS
        leftPadding: Theme.spacingS
        text: radioController.title.trim()
        onClicked: root.openPlayer()

        Accessible.name: qsTr("Now playing: %1").arg(text)
        Accessible.description: qsTr("Open the player")

        contentItem: RowLayout {
            spacing: Theme.spacingM

            Rectangle {
                Layout.preferredWidth: 48
                Layout.preferredHeight: 48
                radius: Theme.radiusS
                color: Qt.alpha(Material.foreground, 0.06)
                clip: true

                Image {
                    anchors.fill: parent
                    visible: root.slides.source !== ""
                    source: root.slides.source
                    fillMode: Image.PreserveAspectCrop
                    cache: false
                }

                IconImage {
                    anchors.centerIn: parent
                    visible: root.slides.source === ""
                    name: "radio"
                    width: 28
                    height: 28
                    sourceSize: Qt.size(width, height)
                    color: Material.hintTextColor
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Label {
                    objectName: "miniPlayer.station"
                    Layout.fillWidth: true
                    text: radioController.title.trim()
                    elide: Text.ElideRight
                    font.pixelSize: Theme.titleSmall
                    font.weight: Font.Medium
                }

                Label {
                    Layout.fillWidth: true
                    visible: text !== ""
                    text: radioController.text.trim() !== "" ? radioController.text.trim() : radioController.ensemble.trim()
                    elide: Text.ElideRight
                    font.pixelSize: Theme.bodySmall
                    color: Material.secondaryTextColor
                }
            }

            ToolButton {
                objectName: "miniPlayButton"
                icon.name: root.player.isActive ? "stop" : "play"
                onClicked: root.player.togglePlay()

                Accessible.name: root.player.isActive ? qsTr("Stop") : qsTr("Play")
            }
        }
    }
}
