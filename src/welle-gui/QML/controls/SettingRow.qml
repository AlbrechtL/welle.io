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

// A setting with a control (combo box, slider, text field, button): title
// and optional description on the left, the control on the right. On
// narrow pages the control goes below the text.
Item {
    id: root

    property string title
    property string description
    // The control fills the row below the text, e.g. a slider
    property bool wideControl: false
    default property alias control: controlSlot.data

    readonly property bool stacked: wideControl || width < 480

    Layout.fillWidth: true
    implicitHeight: grid.implicitHeight + 2 * Theme.spacingM
    opacity: enabled ? 1 : 0.5

    GridLayout {
        id: grid
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.spacingL
        anchors.rightMargin: Theme.spacingL
        columns: root.stacked ? 1 : 2
        columnSpacing: Theme.spacingL
        rowSpacing: Theme.spacingS

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            visible: root.title !== "" || root.description !== ""

            Label {
                Layout.fillWidth: true
                visible: text !== ""
                text: root.title
                wrapMode: Text.Wrap
                font.pixelSize: Theme.bodyLarge
            }

            Label {
                Layout.fillWidth: true
                visible: text !== ""
                text: root.description
                wrapMode: Text.Wrap
                font.pixelSize: Theme.bodySmall
                color: Material.secondaryTextColor
            }
        }

        RowLayout {
            id: controlSlot
            Layout.fillWidth: root.stacked
            Layout.alignment: root.stacked ? Qt.AlignLeft : (Qt.AlignRight | Qt.AlignVCenter)
            spacing: Theme.spacingS
        }
    }
}
