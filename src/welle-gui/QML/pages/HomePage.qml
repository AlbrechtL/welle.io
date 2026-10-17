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

import "../app"
import "../controls"
import "../expert"
import "../home"
import "../models"

// Start page: station list and, next to it, the station that plays (or
// the expert views). In the phone layout ("compact") the list fills the
// page, a mini player at the bottom opens the player as the detail page.
Item {
    id: root
    objectName: "homePage"

    // "compact", "medium" or "expanded", see Theme
    property string layoutMode: "medium"
    property bool isExpert: false
    property StationLists stations
    property Player player
    property MotSlideModel slides

    // Width of the station list next to the detail pane, saved by MainView.
    // At most 45 % of the page, so that the detail pane keeps enough room.
    property real listWidth: 320
    readonly property real shownListWidth: Math.round(Math.min(listWidth, Math.max(240, width * 0.45)))

    // Compact layout: the detail page is open
    property bool detailOpen: false

    readonly property bool compact: layoutMode === "compact"
    readonly property bool miniPlayerVisible: miniPlayer.visible
    readonly property alias stationListPane: listPane
    readonly property alias dashboard: dashboard
    readonly property string detailTitle: isExpert ? qsTr("Expert views") : qsTr("Now playing")

    signal openStationSettings
    signal openReceiverSettings
    signal showAllStations
    // A message for the snackbar (kind "info" or "error")
    signal message(string text, string kind)

    onCompactChanged: if (!compact) detailOpen = false

    StationListPane {
        id: listPane
        width: root.compact ? root.width : root.shownListWidth
        height: root.height - (miniPlayer.visible ? miniPlayer.height : 0)
        visible: !(root.compact && root.detailOpen)
        isExpert: root.isExpert
        stations: root.stations
        onOpenStationSettings: root.openStationSettings()
        onOpenReceiverSettings: root.openReceiverSettings()
        onShowAllStations: root.showAllStations()
    }

    // Divider between list and detail, can be dragged
    Rectangle {
        x: root.shownListWidth
        width: 1
        height: root.height
        visible: !root.compact
        color: Material.dividerColor
    }

    MouseArea {
        objectName: "listResizeHandle"
        x: root.shownListWidth - 4
        width: 9
        height: root.height
        visible: !root.compact
        cursorShape: Qt.SplitHCursor
        preventStealing: true
        onPositionChanged: (mouse) => {
            if (pressed) {
                var newWidth = mapToItem(root, mouse.x, 0).x
                root.listWidth = Math.round(Math.max(260, Math.min(newWidth, root.width * 0.6, 520)))
            }
        }
    }

    Item {
        id: detail
        objectName: "detailPane"
        x: root.compact ? 0 : root.shownListWidth + 1
        width: root.width - x
        height: root.height
        visible: !root.compact || root.detailOpen

        NowPlayingPane {
            anchors.fill: parent
            slides: root.slides
            visible: !root.isExpert
        }

        Dashboard {
            id: dashboard
            anchors.fill: parent
            visible: root.isExpert
            isExpert: root.isExpert
            slides: root.slides
            onMessage: (text, kind) => root.message(text, kind)
        }
    }

    MiniPlayer {
        slides: root.slides
        player: root.player
        id: miniPlayer
        y: root.height - height
        width: root.width
        height: 64
        visible: root.compact && !root.detailOpen
        onOpenPlayer: root.detailOpen = true
    }
}
