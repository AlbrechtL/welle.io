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

pragma Singleton

import QtQuick
import QtQuick.Controls.Material

// Design tokens of the user interface: breakpoints, spacing, corner radii,
// type scale and the colors that the Material style doesn't provide.
// Colors of the style itself (foreground, background, accent, divider)
// come from the Material attached properties.
QtObject {
    // Set by MainView: the window uses the dark theme
    property bool dark: false

    // Set by MainView: shader effects are available. The software renderer
    // of Qt Quick (QT_QUICK_BACKEND=software) can't draw them; there the
    // Material shadows would hide the backgrounds of elevated surfaces.
    property bool effects: true

    // Elevation of a surface, 0 without shader effects
    function elevation(level) {
        return effects ? level : 0
    }

    // Window widths of the responsive layout. Below compactWidth the
    // phone layout is used: one pane, mini player at the bottom.
    readonly property int compactWidth: 600
    readonly property int expandedWidth: 1000

    // Spacing
    readonly property int spacingXS: 4
    readonly property int spacingS: 8
    readonly property int spacingM: 12
    readonly property int spacingL: 16
    readonly property int spacingXL: 24

    // Corner radii
    readonly property int radiusS: 8
    readonly property int radiusM: 12
    readonly property int radiusL: 16
    readonly property int radiusXL: 28

    // Type scale (pixel sizes, after Material 3)
    readonly property int headlineLarge: 32
    readonly property int headlineMedium: 28
    readonly property int headlineSmall: 24
    readonly property int titleLarge: 22
    readonly property int titleMedium: 16
    readonly property int titleSmall: 14
    readonly property int bodyLarge: 16
    readonly property int bodyMedium: 14
    readonly property int bodySmall: 12
    readonly property int labelLarge: 14
    readonly property int labelMedium: 12
    readonly property int labelSmall: 11

    // Status colors
    readonly property color success: dark ? "#81c995" : "#1e8e3e"
    readonly property color warning: dark ? "#fdd663" : "#b06000"
    readonly property color error: dark ? "#f2b8b5" : "#b3261e"
    readonly property color favorite: "#f9ab00"

    // Background of cards
    readonly property color surfaceContainer: dark ? "#211f26" : "#f7f2fa"

    // Snackbars use the inverse colors of the window
    readonly property color inverseSurface: dark ? "#e6e1e5" : "#313033"
    readonly property color inverseOnSurface: dark ? "#313033" : "#f4eff4"

    // Accent colors the user can choose in the settings. The settings save
    // the index, so new colors are added at the end.
    readonly property int defaultAccent: 4 // Blue
    readonly property var accents: [
        { name: "Teal", color: Material.Teal },
        { name: "Indigo", color: Material.Indigo },
        { name: "Deep Orange", color: Material.DeepOrange },
        { name: "Purple", color: Material.Purple },
        { name: "Blue", color: Material.Blue },
        { name: "Green", color: Material.Green }
    ]
}
