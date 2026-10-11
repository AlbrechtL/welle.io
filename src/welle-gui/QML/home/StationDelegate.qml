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
import QtQuick.Controls.impl

import "../controls"
import "../models"

// One station of the station list: avatar, name, ensemble (and channel and
// SId in expert mode), a button to choose the ensemble if the station is
// on several channels, and the favorite star
ItemDelegate {
    id: root
    objectName: "stationRow"

    property string stationNameText
    property string channelNameText
    property string availableChannelNamesText
    property StationLists stations
    property int stationSIdValue
    property bool isExpert: false
    property bool isFavorit: false

    // For the UI tests. Not named like the roles of StationListModel
    // (stationName, stationSId, channelName), that would hide them.
    readonly property int sid: stationSIdValue
    readonly property string channel: channelNameText
    readonly property string label: stationNameText

    readonly property bool isPlaying: radioController.isPlaying && radioController.service === stationSIdValue
    readonly property var availableChannels: availableChannelNamesText === "" ? [] : availableChannelNamesText.split(',')
    readonly property string ensembleName: stations ? stations.ensembleName(channelNameText) : ""

    signal favoritClicked
    signal setDefaultChannel(string newDefaultChannel)

    width: ListView.view ? ListView.view.width : implicitWidth
    text: stationNameText
    highlighted: isPlaying
    leftPadding: Theme.spacingL
    rightPadding: Theme.spacingXS

    contentItem: RowLayout {
        spacing: Theme.spacingM

        // Avatar: the station logo (SPI) if there is one, otherwise the first
        // letter of the station. While the station plays an equalizer is
        // shown instead of the letter, or as a badge on the logo.
        Item {
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36

            // Re-evaluated whenever a new logo was received
            readonly property bool hasLogo: stationLogos.revision >= 0 && stationLogos.hasLogo(root.stationSIdValue)

            Rectangle {
                anchors.fill: parent
                visible: !parent.hasLogo
                radius: 18
                color: root.isPlaying ? Material.accent : Qt.alpha(Material.accent, 0.14)

                Label {
                    anchors.centerIn: parent
                    visible: !root.isPlaying
                    text: root.stationNameText.trim().charAt(0).toUpperCase()
                    font.pixelSize: Theme.titleMedium
                    font.weight: Font.Medium
                    color: Material.accent
                }

                IconImage {
                    anchors.centerIn: parent
                    visible: root.isPlaying
                    name: "graphic_eq"
                    width: 20
                    height: 20
                    sourceSize: Qt.size(width, height)
                    color: root.Material.theme === Material.Dark ? "black" : "white"
                }
            }

            // Logos are made for a light background, also in the dark theme
            Rectangle {
                anchors.fill: parent
                visible: parent.hasLogo
                radius: Theme.radiusS
                color: "white"
                border.width: 1
                border.color: Qt.alpha(Material.foreground, 0.12)

                Image {
                    objectName: "stationLogo"
                    anchors.fill: parent
                    anchors.margins: 2
                    source: parent.visible ? "image://logo/" + root.stationSIdValue + "?r=" + stationLogos.revision : ""
                    sourceSize: Qt.size(64, 64)
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    cache: false
                }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: -3
                width: 18
                height: 18
                radius: 9
                visible: parent.hasLogo && root.isPlaying
                color: Material.accent

                IconImage {
                    anchors.centerIn: parent
                    name: "graphic_eq"
                    width: 12
                    height: 12
                    sourceSize: Qt.size(width, height)
                    color: root.Material.theme === Material.Dark ? "black" : "white"
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
                Layout.fillWidth: true
                text: root.stationNameText
                elide: Text.ElideRight
                font.pixelSize: Theme.bodyLarge
                font.weight: root.isPlaying ? Font.Medium : Font.Normal
                color: root.isPlaying ? Material.accent : Material.foreground
            }

            Label {
                Layout.fillWidth: true
                visible: text !== ""
                text: {
                    var parts = []
                    if (root.ensembleName !== "")
                        parts.push(root.ensembleName)
                    if (root.isExpert) {
                        parts.push(root.availableChannels.length > 1
                                   ? root.channelNameText + " (" + root.availableChannelNamesText + ")"
                                   : root.channelNameText)
                        parts.push("0x" + root.stationSIdValue.toString(16).toUpperCase())
                    }
                    return parts.join(" · ")
                }
                elide: Text.ElideRight
                font.pixelSize: Theme.bodySmall
                color: Material.secondaryTextColor
            }
        }

        ToolButton {
            objectName: "ensembleSwitchButton"
            icon.name: "ensemble_switch"
            visible: root.availableChannels.length > 1
            onClicked: ensembleMenu.open()

            Accessible.name: qsTr("Choose ensemble")

            AutoSizeMenu {
                id: ensembleMenu

                Instantiator {
                    model: root.availableChannels
                    delegate: MenuItem {
                        required property string modelData
                        text: {
                            var name = root.stations ? root.stations.ensembleName(modelData) : ""
                            return name !== "" ? name + " (" + modelData + ")" : modelData
                        }
                        checkable: true
                        checked: modelData === root.channelNameText
                        onTriggered: root.setDefaultChannel(modelData)
                    }

                    onObjectAdded: (index, object) => ensembleMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => ensembleMenu.removeItem(object)
                }
            }
        }

        ToolButton {
            objectName: "favoriteButton"
            icon.name: root.isFavorit ? "star_filled" : "star"
            icon.color: root.isFavorit ? Theme.favorite : Material.hintTextColor
            onClicked: root.favoritClicked()

            Accessible.name: root.isFavorit ? qsTr("Remove station from favorites") : qsTr("Add station to favorites")
        }
    }
}
