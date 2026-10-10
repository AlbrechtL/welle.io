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

// Versions, authors, thanks and licenses below a header with the app
Item {
    id: root
    objectName: "aboutPage"

    property string title: qsTr("About")

    ColumnLayout {
        x: Math.max(Theme.spacingL, (root.width - width) / 2)
        y: Theme.spacingL
        width: Math.min(root.width - 2 * Theme.spacingL, 900)
        height: root.height - 2 * Theme.spacingL
        spacing: Theme.spacingL

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingL

            Image {
                Layout.preferredWidth: 64
                Layout.preferredHeight: 64
                source: "qrc:/icons/icon.png"
                sourceSize: Qt.size(128, 128)
                fillMode: Image.PreserveAspectFit
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: "welle.io"
                    font.pixelSize: Theme.headlineSmall
                    font.weight: Font.Medium
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Open source DAB and DAB+ software defined radio")
                    wrapMode: Text.Wrap
                    color: Material.secondaryTextColor
                }

                Label {
                    objectName: "websiteLink"
                    text: "<a href=\"https://www.welle.io\">www.welle.io</a>"
                    textFormat: Text.StyledText
                    linkColor: Material.accent
                    onLinkActivated: (link) => Qt.openUrlExternally(link)

                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                }
            }
        }

        InfoTabs {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // Versions, authors, thanks and licenses: one tab each, the licenses
    // have a second row to choose the license. The text can be selected
    // and copied, e.g. the versions for a bug report.
    component InfoTabs: Item {
        id: infoPage

        readonly property var pages: [
            { key: "versions", title: qsTr("Versions"), icon: "info", file: "Versions" },
            { key: "authors", title: qsTr("Authors"), icon: "authors", file: "Authors" },
            { key: "thanks", title: qsTr("Thanks"), icon: "thanks", file: "Thanks" },
            { key: "licenses", title: qsTr("Licenses"), icon: "license", file: "" }
        ]
        readonly property var licenses: [
            { title: "GPL-2", file: "GPL-2" },
            { title: "LGPL-2.1", file: "LGPL-2.1" },
            { title: "BSD-3-Clause (kiss_fft)", file: "BSD-3-Clause" }
        ]
        property int licenseIndex: 0
        readonly property bool showsLicenses: mainBar.currentIndex === 3

        // Narrow pages: icon above the text
        readonly property bool narrow: width < 480

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            TabBar {
                id: mainBar
                objectName: "aboutTabs"
                Layout.fillWidth: true

                Repeater {
                    model: infoPage.pages

                    TabButton {
                        required property var modelData
                        objectName: "aboutTab." + modelData.key
                        text: modelData.title
                        icon.name: modelData.icon
                        display: infoPage.narrow ? AbstractButton.TextUnderIcon : AbstractButton.TextBesideIcon
                        font.capitalization: Font.MixedCase
                        font.pixelSize: Theme.titleSmall
                    }
                }

                onCurrentIndexChanged: displayPage()

                Connections {
                    target: guiHelper
                    function onTranslationFinished() {
                        // Update the InfoPage otherwise it won't be re-translated
                        // if we change language in the GUI
                        infoPage.displayPage()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: Material.dividerColor
            }

            // License selection, only on the licenses tab
            Flow {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacingM
                visible: infoPage.showsLicenses
                spacing: Theme.spacingS

                Repeater {
                    model: infoPage.licenses

                    FilterChip {
                        required property var modelData
                        required property int index
                        objectName: "licenseChip." + index
                        text: modelData.title
                        checked: infoPage.licenseIndex === index
                        onClicked: {
                            infoPage.licenseIndex = index
                            infoPage.displayPage()
                        }
                    }
                }
            }

            // Text on a card
            Pane {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: Theme.spacingM
                padding: 0
                Material.background: Theme.surfaceContainer
                Material.elevation: 0
                Material.roundedScale: Material.MediumScale

                Flickable {
                    id: infoPageFlickable
                    anchors.fill: parent
                    anchors.margins: Theme.spacingL
                    anchors.rightMargin: Theme.spacingS
                    contentHeight: fileContent.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    TextEdit {
                        id: fileContent
                        objectName: "aboutContent"
                        width: infoPageFlickable.width - scrollbar.width - Theme.spacingS
                        readOnly: true
                        selectByMouse: true
                        wrapMode: Text.Wrap
                        textFormat: TextEdit.PlainText
                        font.pixelSize: Theme.bodyMedium
                        color: Material.foreground
                        selectionColor: Qt.alpha(Material.accent, 0.4)
                        selectedTextColor: Material.foreground
                        text: guiHelper.getInfoPage("Versions")

                        Accessible.role: Accessible.StaticText
                        Accessible.name: text
                    }

                    ScrollBar.vertical: ScrollBar { id: scrollbar }
                }
            }
        }

        function displayPage() {
            var page = pages[mainBar.currentIndex]
            if (!page) {
                fileContent.text = ""
                return
            }
            fileContent.text = guiHelper.getInfoPage(page.key === "licenses" ? licenses[licenseIndex].file : page.file)
            infoPageFlickable.contentY = 0
        }
    }
}
