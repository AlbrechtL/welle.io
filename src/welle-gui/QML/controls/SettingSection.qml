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

// A group of settings: heading and a rounded card with the settings
// (SettingSwitch, SettingRow) below each other
ColumnLayout {
    id: root

    property alias text: header.text
    default property alias content: column.data

    // Kept for older setting pages, the cards don't need a separator
    property bool isNotFirst: true

    Layout.fillWidth: true
    spacing: Theme.spacingS

    Label {
        id: header
        Layout.fillWidth: true
        Layout.leftMargin: Theme.spacingL
        visible: text !== ""
        color: Material.accent
        font.pixelSize: Theme.titleSmall
        font.weight: Font.Medium
        elide: Text.ElideRight
    }

    Pane {
        Layout.fillWidth: true
        padding: 0
        topPadding: Theme.spacingXS
        bottomPadding: Theme.spacingXS
        Material.background: Qt.alpha(Material.foreground, Theme.dark ? 0.07 : 0.04)
        Material.elevation: 0
        Material.roundedScale: Material.MediumScale

        ColumnLayout {
            id: column
            width: parent.width
            spacing: 0
        }
    }

    Accessible.role: Accessible.Grouping
    Accessible.name: text
}
