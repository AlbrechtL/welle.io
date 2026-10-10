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

// The station that plays on a card like the slide show: ensemble and
// signal at the top, station name and dynamic label (DLS) in the center,
// chips with DAB/DAB+, audio format and programme type at the bottom
Rectangle {
    id: root
    objectName: "stationCard"

    // Smaller fonts, e.g. on a dashboard card
    property bool small: width < 360
    // Less space around the text, for a card as high as its content
    property bool dense: false
    property bool showBackground: true

    readonly property real margin: dense ? Theme.spacingM : Theme.spacingL

    // Height of the content: the centered station name and DLS keep the
    // distance of the top and bottom row to both sides
    implicitHeight: 2 * margin + 2 * Math.max(topRow.implicitHeight, chips.height)
                    + 2 * Theme.spacingS + center.implicitHeight

    radius: Theme.radiusL
    color: showBackground ? Qt.alpha(Material.foreground, 0.06) : "transparent"

    RowLayout {
        id: topRow
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.margin
        spacing: Theme.spacingS

        Label {
            objectName: "nowPlaying.ensemble"
            Layout.fillWidth: true
            text: radioController.ensemble.trim()
            elide: Text.ElideRight
            font.pixelSize: Theme.labelLarge
            font.weight: Font.Medium
            color: Material.secondaryTextColor
        }

        SignalIndicator {
            objectName: "nowPlaying.signal"
        }
    }

    // Station name and DLS, centered in the card
    ColumnLayout {
        id: center
        anchors.centerIn: parent
        width: parent.width - 2 * root.margin
        spacing: root.dense ? Theme.spacingXS : Theme.spacingS

        Label {
            objectName: "nowPlaying.station"
            Layout.fillWidth: true
            text: radioController.title.trim()
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            font.pixelSize: root.small || root.dense ? Theme.titleLarge : Theme.headlineMedium
            font.weight: Font.Medium
        }

        Label {
            objectName: "nowPlaying.dls"
            Layout.fillWidth: true
            text: radioController.text.trim()
            visible: text !== ""
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            maximumLineCount: root.small || root.dense ? 2 : 3
            elide: Text.ElideRight
            font.pixelSize: root.small ? Theme.bodyMedium : Theme.bodyLarge
            color: Material.secondaryTextColor
        }
    }

    // DAB/DAB+ on the left, the audio format in the center, the programme
    // type (genre) on the right
    Item {
        id: chips
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: root.margin
        height: Math.max(dabType.implicitHeight, audioMode.implicitHeight, programmeType.implicitHeight)

        // Room for the center chip between the chips on the left and right
        readonly property real leftEdge: dabType.visible ? dabType.width + Theme.spacingS : 0
        readonly property real rightEdge: programmeType.visible ? width - programmeType.width - Theme.spacingS : width

        InfoChip {
            id: dabType
            objectName: "nowPlaying.dabType"
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            accent: true
            text: radioController.audioMode !== "" ? (radioController.isDAB ? "DAB" : "DAB+") : ""
        }

        InfoChip {
            id: audioMode
            objectName: "nowPlaying.audioMode"
            anchors.verticalCenter: parent.verticalCenter
            // Centered, moved towards the free side if it doesn't fit
            x: Math.max(chips.leftEdge, Math.min((chips.width - width) / 2, chips.rightEdge - width))
            maximumWidth: Math.max(0, chips.rightEdge - chips.leftEdge)
            text: radioController.audioMode
        }

        InfoChip {
            id: programmeType
            objectName: "nowPlaying.programmeType"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            maximumWidth: chips.width / 3
            text: radioController.stationType !== "" ? qsTranslate("DABConstants", radioController.stationType) : ""
        }
    }
}
