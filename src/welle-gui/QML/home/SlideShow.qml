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
import QtQuick.Effects

import "../controls"
import "../models"

// The MOT slide show: the slide on a card, a radio icon if there is no
// slide, and below it the categories of a categorized slide show with
// buttons for the previous and next picture.
ColumnLayout {
    id: root

    property MotSlideModel slides
    property bool showControls: true

    readonly property bool hasSlide: slides.source !== ""

    // The software renderer (QT_QUICK_BACKEND=software, the offscreen
    // platform of the tests) can't run shader effects
    readonly property bool canUseEffects: Theme.effects
    readonly property bool controlsVisible: showControls && (slides.categories.count > 1 || slides.pictureCount > 1)

    spacing: Theme.spacingS

    Rectangle {
        objectName: "slideCard"
        Layout.fillWidth: true
        Layout.fillHeight: true
        radius: Theme.radiusL
        color: Qt.alpha(Material.foreground, 0.06)

        // The slide with rounded corners like the card
        Item {
            objectName: "nowPlaying.slide"
            anchors.fill: parent
            visible: root.hasSlide

            // For the UI tests
            readonly property string source: root.slides.source
            readonly property real paintedWidth: slideImage.paintedWidth
            readonly property real paintedHeight: slideImage.paintedHeight

            Image {
                id: slideImage
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                cache: false
                source: root.slides.source
                visible: !root.canUseEffects
            }

            Item {
                id: roundedMask
                anchors.fill: parent
                visible: false
                layer.enabled: root.canUseEffects

                Rectangle {
                    anchors.centerIn: parent
                    width: slideImage.paintedWidth
                    height: slideImage.paintedHeight
                    radius: Theme.radiusL
                }
            }

            MultiEffect {
                anchors.fill: parent
                visible: root.canUseEffects
                source: slideImage
                maskEnabled: true
                maskSource: roundedMask
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0
            }

            Accessible.role: Accessible.Graphic
            Accessible.name: qsTr("Slide show")
        }

        IconImage {
            anchors.centerIn: parent
            visible: !root.hasSlide
            name: "radio"
            width: Math.min(parent.width, parent.height) * 0.35
            height: width
            sourceSize: Qt.size(width, height)
            color: Material.hintTextColor
        }
    }

    RowLayout {
        Layout.fillWidth: true
        visible: root.controlsVisible
        spacing: Theme.spacingXS

        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: categoryRow.implicitHeight
            contentWidth: categoryRow.implicitWidth
            clip: true
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds

            Row {
                id: categoryRow
                spacing: Theme.spacingXS

                Repeater {
                    model: root.slides.categories

                    FilterChip {
                        required property string categoryTitle
                        required property int categoryId

                        objectName: "slideCategory"
                        text: categoryId === 0 ? qsTr("Latest") : categoryTitle
                        checked: categoryId === root.slides.currentCategoryId
                        onClicked: root.slides.selectCategory(categoryId)
                    }
                }
            }
        }

        ToolButton {
            objectName: "previousSlideButton"
            icon.name: "previous"
            visible: root.slides.pictureCount > 1
            onClicked: root.slides.previous()
            Accessible.name: qsTr("Previous slide")
        }

        ToolButton {
            objectName: "nextSlideButton"
            icon.name: "next"
            visible: root.slides.pictureCount > 1
            onClicked: root.slides.next()
            Accessible.name: qsTr("Next slide")
        }
    }
}
