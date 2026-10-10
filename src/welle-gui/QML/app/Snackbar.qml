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

import "../controls"

// Message bar at the bottom of the window. Errors stay until they are
// dismissed, info messages disappear after 5 seconds. Messages that arrive
// while one is shown wait in a queue; the same message is not queued twice.
Popup {
    id: root
    objectName: "snackbar"

    // Distance to the bottom of the window, e.g. above the mini player
    property real bottomOffset: 0

    // Shown message
    property string text: ""
    property string kind: "info" // "info" or "error"

    property var queue: []

    // Shows a message now or after the messages before it
    function show(message, messageKind) {
        if (message === "")
            return
        if ((visible && message === text) || queue.some(m => m.text === message))
            return
        queue.push({ text: message, kind: messageKind })
        if (!visible)
            _showNext()
    }

    function _showNext() {
        if (queue.length === 0)
            return
        var next = queue.shift()
        text = next.text
        kind = next.kind
        open()
        if (kind === "info")
            hideTimer.restart()
    }

    parent: Overlay.overlay
    width: Math.min(560, parent.width - 2 * Theme.spacingL)
    x: Math.round((parent.width - width) / 2)
    y: parent.height - height - Theme.spacingL - bottomOffset
    padding: 0
    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose

    Material.theme: Theme.dark ? Material.Light : Material.Dark
    Material.background: Theme.inverseSurface
    Material.elevation: Theme.elevation(6)
    Material.roundedScale: Material.ExtraSmallScale

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }
        NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: 150; easing.type: Easing.OutQuad }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 100 }
    }

    onClosed: _showNext()

    Timer {
        id: hideTimer
        interval: 5000
        onTriggered: root.close()
    }

    contentItem: RowLayout {
        spacing: Theme.spacingM

        IconImage {
            Layout.leftMargin: Theme.spacingL
            visible: root.kind === "error"
            name: "error"
            width: 24
            height: 24
            sourceSize: Qt.size(width, height)
            color: Theme.dark ? "#b3261e" : "#f2b8b5"
        }

        Label {
            objectName: "snackbar.text"
            Layout.fillWidth: true
            Layout.leftMargin: root.kind === "error" ? 0 : Theme.spacingL
            Layout.topMargin: Theme.spacingM
            Layout.bottomMargin: Theme.spacingM
            text: root.text
            wrapMode: Text.Wrap
            color: Theme.inverseOnSurface
            font.pixelSize: Theme.bodyMedium
        }

        ToolButton {
            objectName: "snackbar.close"
            Layout.rightMargin: Theme.spacingXS
            icon.name: "close"
            icon.color: Theme.inverseOnSurface
            onClicked: root.close()

            Accessible.name: qsTr("Dismiss")
        }

        Accessible.role: Accessible.AlertMessage
        Accessible.name: root.text
    }
}
