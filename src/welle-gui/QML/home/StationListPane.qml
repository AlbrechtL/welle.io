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
import "../models"

// The station list (all stations or favorites, chosen in the navigation
// drawer): title, a menu with the station scan, the scan progress, an
// empty state and, in expert mode, the manual channel selection
Pane {
    id: root
    objectName: "stationListPane"

    property StationLists stations
    property bool isExpert: false

    // 0: all stations, 1: favorites. Chosen in the drawer, only shown here.
    readonly property int listIndex: stations ? stations.shownIndex : 0

    readonly property alias view: stationChannelView
    readonly property bool isScanning: radioController.isChannelScan
    property int scanChannel: 0

    signal openStationSettings
    signal showAllStations

    padding: 0

    Connections {
        target: radioController
        function onScanProgress(progress) { root.scanChannel = progress }
        function onIsChannelScanChanged() { if (!radioController.isChannelScan) root.scanChannel = 0 }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingL
            Layout.topMargin: Theme.spacingXS
            spacing: Theme.spacingS

            Label {
                objectName: "stationListTitle"
                text: root.listIndex === 0 ? qsTr("All stations") : qsTr("Favorites")
                elide: Text.ElideRight
                font.pixelSize: Theme.titleMedium
                font.weight: Font.Medium
                // Natural width, shorter only if the pane is too narrow
                Layout.preferredWidth: implicitWidth
                Layout.minimumWidth: 40
            }

            Label {
                objectName: "stationListCount"
                visible: stationChannelView.count > 0
                text: stationChannelView.count
                font.pixelSize: Theme.labelLarge
                color: Material.secondaryTextColor
            }

            Item { Layout.fillWidth: true }

            ToolButton {
                objectName: "stationListMenuButton"
                icon.name: "more_vert"
                onClicked: stationMenu.open()

                Accessible.name: qsTr("Stations menu")
                Accessible.description: qsTr("Show stations menu")

                AutoSizeMenu {
                    id: stationMenu
                    objectName: "stationListMenu"
                    y: parent.height

                    MenuItem {
                        objectName: "startScanItem"
                        text: qsTr("Start station scan")
                        icon.name: "scan"
                        enabled: !root.isScanning
                        onTriggered: radioController.startScan()
                    }

                    MenuItem {
                        objectName: "stopScanItem"
                        text: qsTr("Stop station scan")
                        icon.name: "stop"
                        enabled: root.isScanning
                        onTriggered: radioController.stopScan()
                    }

                    MenuItem {
                        objectName: "clearListItem"
                        text: qsTr("Clear station list")
                        icon.name: "delete_sweep"
                        onTriggered: root.stations.all.clearStations()
                    }

                    MenuSeparator {}

                    MenuItem {
                        objectName: "stationSettingsItem"
                        text: qsTr("Station settings")
                        icon.name: "settings"
                        onTriggered: root.openStationSettings()
                    }
                }
            }
        }

        // Scan progress
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.spacingL
            visible: root.isScanning
            spacing: Theme.spacingXS

            Label {
                Layout.fillWidth: true
                text: qsTr("Scanning channel %1 of %2").arg(Math.max(root.scanChannel, 1)).arg(radioController.numberOfChannels)
                      + (radioController.channel !== "" ? " (" + radioController.channel + ")" : "")
                elide: Text.ElideRight
                font.pixelSize: Theme.bodySmall
                color: Material.secondaryTextColor
            }

            ProgressBar {
                objectName: "scanProgress"
                Layout.fillWidth: true
                from: 0
                to: radioController.numberOfChannels
                value: root.scanChannel

                Accessible.name: qsTr("Station scan progress")
            }
        }

        // Empty state
        ColumnLayout {
            objectName: "stationListEmpty"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.spacingXL
            visible: stationChannelView.count === 0
            spacing: Theme.spacingM

            Item { Layout.fillHeight: true }

            IconImage {
                Layout.alignment: Qt.AlignHCenter
                name: root.listIndex === 0 ? "radio" : "star"
                width: 48
                height: 48
                sourceSize: Qt.size(width, height)
                color: Material.hintTextColor
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: root.listIndex === 0 ? qsTr("No stations in list") : qsTr("No favorites yet")
                font.pixelSize: Theme.titleMedium
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: root.listIndex === 0 ? qsTr("Search for stations on all channels.")
                                           : qsTr("Mark a station in the list of all stations with the star to add it here.")
                font.pixelSize: Theme.bodyMedium
                color: Material.secondaryTextColor
            }

            Button {
                objectName: "scanButton"
                Layout.alignment: Qt.AlignHCenter
                visible: root.listIndex === 0
                enabled: !root.isScanning
                highlighted: true
                Material.elevation: 0
                icon.name: "scan"
                text: qsTr("Start station scan")
                font.capitalization: Font.MixedCase
                onClicked: radioController.startScan()
            }

            Button {
                objectName: "showAllStationsButton"
                Layout.alignment: Qt.AlignHCenter
                visible: root.listIndex === 1
                flat: true
                icon.name: "radio"
                text: qsTr("Show all stations")
                font.capitalization: Font.MixedCase
                onClicked: root.showAllStations()
            }

            Item { Layout.fillHeight: true }
        }

        ListView {
            id: stationChannelView
            objectName: "stationListView"
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: count > 0
            clip: true
            model: root.stations ? root.stations.shown : null
            boundsBehavior: Flickable.StopAtBounds
            keyNavigationEnabled: true

            delegate: StationDelegate {
                required property string stationName
                required property int stationSId
                required property string channelName
                required property string availableChannelNames
                required property bool favorit

                stationNameText: stationName.trim()
                stationSIdValue: stationSId
                channelNameText: channelName == "File" ? qsTr("File") : channelName
                availableChannelNamesText: channelName == "File" ? "" : availableChannelNames
                stations: root.stations
                isFavorit: favorit
                isExpert: root.isExpert
                onClicked: radioController.play(channelName, stationName, stationSId)
                onFavoritClicked: root.stations.setFavorite(stationName, stationSId, channelName, !favorit)
                onSetDefaultChannel: (newDefaultChannel) => {
                    root.stations.all.setDefaultChannel(stationSId, newDefaultChannel)
                    radioController.play(newDefaultChannel, stationName, stationSId)
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }

        // Manual channel (expert mode)
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingL
            Layout.rightMargin: Theme.spacingS
            Layout.bottomMargin: Theme.spacingXS
            visible: root.isExpert

            Label {
                Layout.fillWidth: true
                text: qsTr("Manual channel")
                elide: Text.ElideRight
            }

            ComboBox {
                id: manualChannelBox
                objectName: "manualChannelBox"
                enabled: radioController.deviceId !== 3 // CDeviceID::RAWFILE
                model: ["5A", "5B", "5C", "5D",
                    "6A", "6B", "6C", "6D",
                    "7A", "7B", "7C", "7D",
                    "8A", "8B", "8C", "8D",
                    "9A", "9B", "9C", "9D",
                    "10A", "10B", "10C", "10D",
                    "11A", "11B", "11C", "11D",
                    "12A", "12B", "12C", "12D",
                    "13A", "13B", "13C", "13D", "13E", "13F"]

                popup.Material.elevation: Theme.elevation(8)
                onActivated: (index) => radioController.setManualChannel(model[index])

                Accessible.name: qsTr("Manual channel")
            }
        }
    }
}
