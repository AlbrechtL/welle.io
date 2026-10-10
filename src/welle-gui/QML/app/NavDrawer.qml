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

// Navigation drawer behind the ☰ button: slides in from the left over a
// dimmed window, closes on selection, a click outside, Esc or a swipe
Drawer {
    id: root
    objectName: "navDrawer"

    // Highlighted destination: "all", "favorites", "settings", "about"
    property string current: "all"

    // Shown next to the station lists
    property int allStationsCount: 0
    property int favoritesCount: 0

    signal navigate(string destination)

    width: Math.min(320, parent.width * 0.85)
    height: parent.height
    edge: Qt.LeftEdge
    modal: true
    interactive: true
    Material.elevation: Theme.elevation(16)

    // Edge to the content if there is no shadow
    Rectangle {
        parent: root.background
        visible: !Theme.effects
        anchors.right: parent ? parent.right : undefined
        width: 1
        height: parent ? parent.height : 0
        color: Material.dividerColor
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header: icon, name and version
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.spacingL
            Layout.topMargin: Theme.spacingXL
            spacing: Theme.spacingM

            Image {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                source: "qrc:/icons/icon.png"
                sourceSize: Qt.size(80, 80)
                fillMode: Image.PreserveAspectFit
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Label {
                    text: "welle.io"
                    font.pixelSize: Theme.titleLarge
                    font.weight: Font.Medium
                }

                Label {
                    Layout.fillWidth: true
                    visible: text !== ""
                    text: Qt.application.version.split(" ")[0]
                    elide: Text.ElideRight
                    font.pixelSize: Theme.bodySmall
                    color: Material.secondaryTextColor
                }
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: items.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            ColumnLayout {
                id: items
                width: parent.width
                spacing: 2

                NavItem { destination: "all"; text: qsTr("All stations"); icon.name: "radio"; badge: root.allStationsCount }
                NavItem { destination: "favorites"; text: qsTr("Favorites"); icon.name: "star"; badge: root.favoritesCount }

                MenuSeparator { Layout.fillWidth: true }

                NavItem { destination: "settings"; text: qsTr("Settings"); icon.name: "settings" }
                NavItem { destination: "about"; text: qsTr("About"); icon.name: "info" }

                MenuSeparator { Layout.fillWidth: true }

                NavItem { destination: "exit"; text: qsTr("Exit"); icon.name: "exit" }
            }
        }
    }

    // Material 3 navigation drawer item: rounded, highlighted if current
    component NavItem: ItemDelegate {
        id: item

        property string destination
        // Number shown on the right, e.g. the stations of a list (-1: none)
        property int badge: -1

        objectName: "navItem." + destination
        rightPadding: badge >= 0 ? badgeLabel.width + 2 * Theme.spacingL : Theme.spacingL
        Layout.fillWidth: true
        Layout.leftMargin: Theme.spacingM
        Layout.rightMargin: Theme.spacingM
        highlighted: root.current === destination
        font.weight: highlighted ? Font.Medium : Font.Normal
        icon.color: highlighted ? Material.accent : Material.foreground

        background: Rectangle {
            implicitHeight: 48
            radius: height / 2
            color: item.highlighted ? Qt.alpha(item.Material.accent, 0.16)
                 : item.down ? item.Material.rippleColor
                 : item.hovered ? Qt.alpha(item.Material.foreground, 0.06) : "transparent"
        }

        Label {
            id: badgeLabel
            objectName: "navBadge." + item.destination
            anchors.right: parent.right
            anchors.rightMargin: Theme.spacingL
            anchors.verticalCenter: parent.verticalCenter
            visible: item.badge >= 0
            text: item.badge
            font.pixelSize: Theme.labelLarge
            color: item.highlighted ? item.Material.accent : item.Material.secondaryTextColor
        }

        onClicked: {
            root.close()
            root.navigate(destination)
        }
    }
}
