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

import "../controls"

// Top app bar: navigation drawer button (or back button on sub pages),
// title, play/stop and volume. Wide windows show the volume slider in the
// bar, narrow windows in a popup.
ToolBar {
    id: root

    property Player player
    property string title: "welle.io"
    property bool canGoBack: false
    property bool compact: false

    signal openNavigation
    signal back

    Material.background: ApplicationWindow.window ? ApplicationWindow.window.Material.backgroundColor : "transparent"
    Material.elevation: 0

    // Divider to the content
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Material.dividerColor
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingXS
        anchors.rightMargin: Theme.spacingXS
        spacing: 0

        ToolButton {
            objectName: "navButton"
            visible: !root.canGoBack
            icon.name: "menu"
            onClicked: root.openNavigation()

            Accessible.name: qsTr("Main menu")
            Accessible.description: qsTr("Open the navigation menu")
        }

        ToolButton {
            objectName: "backButton"
            visible: root.canGoBack
            icon.name: "back"
            onClicked: root.back()

            Accessible.name: qsTr("Back")
        }

        Label {
            objectName: "appTitle"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            text: root.title
            elide: Text.ElideRight
            font.pixelSize: Theme.titleLarge
            verticalAlignment: Text.AlignVCenter
        }

        ToolButton {
            id: playStopButton
            objectName: "playStopButton"
            icon.name: root.player.isActive ? "stop" : "play"
            onClicked: root.player.togglePlay()

            Accessible.name: root.player.isActive ? qsTr("Stop") : qsTr("Play")
            Accessible.description: radioController.isPlaying ? qsTr("Stop playback")
                                    : radioController.isChannelScan ? qsTr("Stop scan") : qsTr("Start playback")

            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: Accessible.name
        }

        ToolButton {
            id: volumeButton
            objectName: "volumeButton"
            icon.name: root.player.volume === 0 ? "speaker_mute" : "speaker"
            onClicked: root.compact ? volumePopup.open() : root.player.toggleMute()

            Accessible.name: qsTr("Volume")
            Accessible.description: root.compact ? qsTr("Show volume slider") : qsTr("Mute or unmute")

            ToolTip.visible: hovered && !volumePopup.visible
            ToolTip.delay: 500
            ToolTip.text: root.compact ? qsTr("Volume") : (root.player.volume === 0 ? qsTr("Unmute") : qsTr("Mute"))

            WheelHandler {
                onWheel: (event) => root.player.changeVolume(event.angleDelta.y > 0 ? 0.05 : -0.05)
            }
        }

        VolumeSlider {
            player: root.player
            Layout.preferredWidth: 120
            Layout.rightMargin: Theme.spacingS
            visible: !root.compact
        }
    }

    Popup {
        id: volumePopup
        objectName: "volumePopup"
        parent: volumeButton
        Material.elevation: Theme.elevation(8)
        x: Math.round((volumeButton.width - width) / 2)
        y: volumeButton.height
        padding: Theme.spacingS

        ColumnLayout {
            spacing: Theme.spacingXS

            VolumeSlider {
                player: root.player
                Layout.alignment: Qt.AlignHCenter
                orientation: Qt.Vertical
                Layout.preferredHeight: 140
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: Math.round(root.player.volume * 100) + "%"
                font.pixelSize: Theme.labelMedium
            }

            ToolButton {
                Layout.alignment: Qt.AlignHCenter
                icon.name: root.player.volume === 0 ? "speaker" : "speaker_mute"
                onClicked: root.player.toggleMute()

                Accessible.name: root.player.volume === 0 ? qsTr("Unmute") : qsTr("Mute")
            }
        }
    }

    component VolumeSlider: Slider {
        property Player player

        objectName: "volumeSlider"
        from: 0
        to: 1
        stepSize: 0.01
        wheelEnabled: true
        value: player ? player.volume : 0
        onMoved: player.setVolume(value)

        Accessible.name: qsTr("Volume")
    }
}
