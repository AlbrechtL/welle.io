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

// A small rounded label, e.g. "DAB+" or the audio format. Hidden if the
// text is empty.
Rectangle {
    id: chip

    property alias text: label.text
    property bool accent: false

    visible: text !== ""
    // Width with the whole text
    readonly property real naturalWidth: label.implicitWidth + 2 * Theme.spacingM

    implicitWidth: Math.min(naturalWidth, maximumWidth)
    implicitHeight: label.implicitHeight + Theme.spacingXS * 2
    radius: Theme.radiusS
    color: accent ? Qt.alpha(Material.accent, 0.16) : "transparent"
    border.width: accent ? 0 : 1
    border.color: Material.dividerColor

    property real maximumWidth: 100000

    Label {
        id: label
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingM
        anchors.rightMargin: Theme.spacingM
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        font.pixelSize: Theme.labelMedium
        font.weight: Font.Medium
        color: chip.accent ? Material.accent : Material.secondaryTextColor
    }

    Accessible.role: Accessible.StaticText
    Accessible.name: text
}
