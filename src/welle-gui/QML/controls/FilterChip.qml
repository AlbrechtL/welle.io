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

// Material 3 filter chip: outlined, tonal with a check mark when selected.
// For choosing one of a few options, e.g. a license or a slide category.
Button {
    id: chip

    checkable: true
    flat: true
    icon.name: checked ? "check" : ""
    icon.width: 18
    icon.height: 18
    icon.color: Material.accent
    font.capitalization: Font.MixedCase
    font.pixelSize: Theme.labelLarge
    font.weight: Font.Medium
    leftPadding: checked ? Theme.spacingS : Theme.spacingM
    rightPadding: Theme.spacingM
    topInset: 0
    bottomInset: 0
    implicitHeight: 32
    Material.foreground: checked ? Material.accent : Material.secondaryTextColor

    background: Rectangle {
        implicitHeight: 32
        radius: Theme.radiusS
        color: chip.checked ? Qt.alpha(chip.Material.accent, 0.16)
             : chip.hovered ? Qt.alpha(chip.Material.foreground, 0.06) : "transparent"
        border.width: chip.checked ? 0 : 1
        border.color: chip.Material.dividerColor
    }
}
