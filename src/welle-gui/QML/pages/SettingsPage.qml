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
import "../settings"

// All settings on one scrolling page of cards. Wide windows show an index
// of the sections on the left. MainView creates the page at startup,
// because the settings are applied when they are loaded (input device,
// gain), and pushes it onto the StackView to show it.
Item {
    id: root
    objectName: "settingsPage"

    property string title: qsTr("Settings")

    readonly property alias generalSettings: general
    readonly property alias stationSettings: stations
    readonly property alias receiverSettings: receiver
    readonly property alias appearanceSettings: appearance
    readonly property alias expertSettings: expert

    readonly property bool showIndex: width >= 840
    readonly property var sections: [
        { key: "general", title: qsTr("General"), icon: "settings", item: general },
        { key: "stations", title: qsTr("Stations"), icon: "radio", item: stations },
        { key: "receiver", title: qsTr("Receiver"), icon: "antenna", item: receiver },
        { key: "appearance", title: qsTr("Appearance"), icon: "image", item: appearance },
        { key: "expert", title: qsTr("Expert"), icon: "tune", item: expert }
    ]

    // Section at the top of the view
    readonly property string currentSection: {
        var current = sections[0].key
        var top = flickable.contentY + Theme.spacingXL * 2
        for (var i = 0; i < sections.length; i++) {
            if (sections[i].item.y <= top)
                current = sections[i].key
        }
        if (flickable.atYEnd && flickable.contentHeight > flickable.height)
            current = sections[sections.length - 1].key
        return current
    }

    // Section to scroll to when the page is shown
    property string pendingSection: ""

    StackView.onActivated: {
        if (pendingSection !== "") {
            showSection(pendingSection)
            pendingSection = ""
        }
    }

    // Scrolls to a section ("general", "stations", ...)
    function showSection(key) {
        for (var i = 0; i < sections.length; i++) {
            if (sections[i].key === key) {
                var target = Math.max(0, Math.min(sections[i].item.y - Theme.spacingL,
                                                  flickable.contentHeight - flickable.height))
                scrollAnimation.to = target
                scrollAnimation.restart()
            }
        }
    }

    NumberAnimation {
        id: scrollAnimation
        target: flickable
        property: "contentY"
        duration: 250
        easing.type: Easing.OutCubic
    }

    // Index of the sections
    ColumnLayout {
        id: index
        x: Theme.spacingM
        y: Theme.spacingL
        width: 220
        visible: root.showIndex
        spacing: 2

        Repeater {
            model: root.sections

            ItemDelegate {
                id: indexItem
                required property var modelData

                objectName: "settingsIndex." + modelData.key
                Layout.fillWidth: true
                text: modelData.title
                icon.name: modelData.icon
                highlighted: root.currentSection === modelData.key
                icon.color: highlighted ? Material.accent : Material.foreground
                onClicked: root.showSection(modelData.key)

                background: Rectangle {
                    implicitHeight: 44
                    radius: height / 2
                    color: indexItem.highlighted ? Qt.alpha(indexItem.Material.accent, 0.16)
                         : indexItem.hovered ? Qt.alpha(indexItem.Material.foreground, 0.06) : "transparent"
                }
            }
        }
    }

    Flickable {
        id: flickable
        objectName: "settingsFlickable"
        x: root.showIndex ? index.x + index.width + Theme.spacingL : 0
        width: root.width - x
        height: root.height
        contentHeight: column.implicitHeight + 2 * Theme.spacingL
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: column
            x: Math.max(Theme.spacingL, (flickable.width - width) / 2)
            y: Theme.spacingL
            width: Math.min(flickable.width - 2 * Theme.spacingL, 720)
            spacing: Theme.spacingXL

            GeneralSettings { id: general }
            StationSettings { id: stations }
            ReceiverSettings { id: receiver }
            AppearanceSettings { id: appearance }
            ExpertSettings { id: expert }
        }
    }
}
