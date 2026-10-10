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

import "../controls"

// Card of the expert dashboard: title and menu (maximize, move, remove and
// entries the view adds with addEntry()), the view below
Pane {
    id: root

    default property alias content: placeholder.data
    property alias labelText: label.text
    // Name of the view in the Dashboard
    property string viewName
    readonly property bool isMaximize: dashboard ? dashboard.maximizedView === viewName : false

    // Height of the card in the grid, views can change it
    property int cardHeight: 320

    // Set by the Dashboard
    property Dashboard dashboard: null
    property int index: 0
    property int count: 1

    padding: 0
    Material.elevation: Theme.elevation(1)
    Material.roundedScale: Material.MediumScale
    Material.background: Theme.surfaceContainer

    // Outline if there is no shadow
    Rectangle {
        parent: root.background
        anchors.fill: parent
        visible: !Theme.effects
        color: "transparent"
        radius: Theme.radiusM
        border.color: Material.dividerColor
    }

    Component {
        id: menuItem
        MenuItem {}
    }

    // Possibility to add options entries dynamically
    function addEntry(title, onTriggered) {
        // Menu is no Item, insertItem() moves the entry into it
        var obj = menuItem.createObject(root, {text: title})
        obj.triggered.connect(onTriggered)
        menu.insertItem(0, obj) // Put it to first position
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingL
            Layout.rightMargin: Theme.spacingXS
            Layout.topMargin: Theme.spacingXS
            spacing: Theme.spacingXS

            Label {
                id: label
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.pixelSize: Theme.titleSmall
                font.weight: Font.Medium
                Accessible.ignored: true
            }

            ToolButton {
                id: menuButton
                objectName: "cardMenuButton"
                icon.name: "more_vert"
                onClicked: menu.open()

                Accessible.name: qsTr("%1 menu").arg(label.text)
                Accessible.description: qsTr("Show %1 menu").arg(label.text)

                AutoSizeMenu {
                    id: menu
                    objectName: "cardMenu"
                    y: menuButton.height

                    MenuItem {
                        objectName: "cardMaximize"
                        text: root.isMaximize ? qsTr("Minimize") : qsTr("Maximize")
                        icon.name: root.isMaximize ? "close_fullscreen" : "open_in_full"
                        onTriggered: root.dashboard.toggleMaximize(root.viewName)
                    }

                    MenuItem {
                        objectName: "cardMoveEarlier"
                        text: qsTr("Move up")
                        icon.name: "move_up"
                        enabled: root.index > 0 && !root.isMaximize
                        onTriggered: root.dashboard.moveView(root.viewName, -1)
                    }

                    MenuItem {
                        objectName: "cardMoveLater"
                        text: qsTr("Move down")
                        icon.name: "move_down"
                        enabled: root.index < root.count - 1 && !root.isMaximize
                        onTriggered: root.dashboard.moveView(root.viewName, 1)
                    }

                    MenuItem {
                        objectName: "cardRemove"
                        text: qsTr("Remove")
                        icon.name: "delete"
                        onTriggered: root.dashboard.removeView(root.viewName)
                    }
                }
            }
        }

        Item {
            id: placeholder
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.spacingS
            Layout.topMargin: 0
            clip: true
        }
    }

    Accessible.role: Accessible.Grouping
    Accessible.name: labelText
}
