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

import "../controls"

import "../models"

// The station that plays: the station and the slide show. Side by side
// (station left) on two cards of the same size (4:3 like the slides) if
// there is enough room. Otherwise below each other (station on top): the
// station card as high as its content and the slide scaled to the rest of
// the height, without scrolling.
Item {
    id: root
    objectName: "nowPlayingPane"

    property MotSlideModel slides

    // Side by side from this width of the pane, below each other before
    readonly property int sideBySideWidth: 840
    readonly property bool sideBySide: width >= sideBySideWidth
    readonly property real controlsHeight: slideShow.controlsVisible ? 44 : 0
    readonly property real availableWidth: width - 2 * Theme.spacingL
    readonly property real availableHeight: height - 2 * Theme.spacingL

    // Not larger than 520 px: the slides are 320 x 240
    readonly property real maximumCardWidth: 520

    // Side by side: two cards of the same size, 4:3 like the slides
    readonly property real cardWidth: Math.round(Math.min((availableWidth - Theme.spacingL) / 2,
                                                          (availableHeight - controlsHeight) * 4 / 3, maximumCardWidth))

    // Below each other: the slide in the height that the station card
    // leaves (at least the height of its content), so that nothing has to
    // be scrolled (e.g. 800 x 480 displays). Scrolling only if the slide
    // would get smaller than this.
    readonly property real minimumSlideWidth: 160
    readonly property real stationWidth: Math.min(availableWidth, maximumCardWidth)
    readonly property real slideWidth: Math.round(Math.max(minimumSlideWidth, Math.min(stationWidth,
        (availableHeight - stationCard.implicitHeight - Theme.spacingM - controlsHeight) * 4 / 3)))
    // If the slide has the full width, the station card takes the height
    // that is left
    readonly property real stationHeight: Math.max(stationCard.implicitHeight,
        availableHeight - Theme.spacingM - Math.round(slideWidth * 3 / 4) - controlsHeight)

    Flickable {
        id: flickable
        objectName: "nowPlayingFlickable"
        anchors.fill: parent
        anchors.margins: Theme.spacingL
        contentHeight: Math.max(grid.implicitHeight, height)
        interactive: grid.implicitHeight > height
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollBar.vertical: ScrollBar {}

        GridLayout {
            id: grid
            x: Math.round((flickable.width - implicitWidth) / 2)
            y: Math.max(0, Math.round((flickable.height - implicitHeight) / 2))
            columns: root.sideBySide ? 2 : 1
            columnSpacing: Theme.spacingL
            rowSpacing: Theme.spacingM

            StationCard {
                id: stationCard
                dense: !root.sideBySide
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                Layout.preferredWidth: root.sideBySide ? root.cardWidth : root.stationWidth
                Layout.preferredHeight: root.sideBySide ? Math.round(root.cardWidth * 3 / 4) : root.stationHeight
            }

            // Below each other the card has the width of the station card,
            // the slide (scaled to the height) is centered on it
            SlideShow {
                id: slideShow
                slides: root.slides
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                Layout.preferredWidth: root.sideBySide ? root.cardWidth : root.stationWidth
                Layout.preferredHeight: Math.round((root.sideBySide ? root.cardWidth : root.slideWidth) * 3 / 4)
                                        + root.controlsHeight
                Layout.fillWidth: false
                Layout.fillHeight: false
            }
        }
    }
}
