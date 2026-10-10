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

// A setting that is switched on or off: title, optional description and
// the switch on the right. The whole row can be clicked.
SwitchDelegate {
    id: root

    property string description

    Layout.fillWidth: true
    leftPadding: Theme.spacingL
    rightPadding: Theme.spacingL

    contentItem: ColumnLayout {
        spacing: 2

        Label {
            Layout.fillWidth: true
            Layout.rightMargin: root.indicator.width + root.spacing
            text: root.text
            wrapMode: Text.Wrap
            font.pixelSize: Theme.bodyLarge
            color: root.enabled ? Material.foreground : Material.hintTextColor
        }

        Label {
            Layout.fillWidth: true
            Layout.rightMargin: root.indicator.width + root.spacing
            visible: text !== ""
            text: root.description
            wrapMode: Text.Wrap
            font.pixelSize: Theme.bodySmall
            color: root.enabled ? Material.secondaryTextColor : Material.hintTextColor
        }
    }
}
