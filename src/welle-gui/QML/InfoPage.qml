/*
 *    Copyright (C) 2017 - 2021
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

// Import custom styles
import "texts"

Item {
    id: infoPage

    ColumnLayout {
        anchors.fill: parent

        TabBar {
            id: mainBar
            width: parent.width
            Layout.fillWidth: true

            TabButton {
                font.capitalization: Font.MixedCase
                font.pixelSize: TextStyle.textStandartSize
                text: qsTr("Versions")
                width: implicitWidth
            }
            TabButton {
                font.capitalization: Font.MixedCase
                font.pixelSize: TextStyle.textStandartSize
                text: qsTr("Authors")
                width: implicitWidth
            }
            TabButton {
                font.capitalization: Font.MixedCase
                font.pixelSize: TextStyle.textStandartSize
                text: qsTr("Thanks")
                width: implicitWidth
            }
            TabButton {
                font.capitalization: Font.MixedCase
                font.pixelSize: TextStyle.textStandartSize
                text: qsTr("Licenses")
                width: implicitWidth
            }

            onCurrentIndexChanged: {
                displayPage()
            }

            Connections {
                target: guiHelper
                function onTranslationFinished() {
                    // Update the InfoPage otherwise it won't be re-translated
                    // if we change language in the GUI
                    displayPage()
                }
            }
        }

        TabBar {
            id: licensesBar
            width: parent.width
            Layout.fillWidth: true

            Repeater {
                model: ["GPL-2", "LGPL-2.1", "BSD-3-Clause (kiss_fft)"]
                TabButton {
                    font.capitalization: Font.MixedCase
                    font.pixelSize: TextStyle.textStandartSize
                    text: modelData
                    width: implicitWidth
                }
            }

            onCurrentIndexChanged: {
                displayPage()
            }
        }

        Flickable {
            id: infoPageFlickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: fileContent.implicitHeight

            clip: true

            TextStandart {
                id: fileContent
                text: guiHelper.getInfoPage("Versions")
                Layout.alignment: Qt.AlignLeft
                wrapMode: Text.Wrap
                width: parent.width - scrollbar.width
                textFormat: Text.PlainText
            }

            ScrollBar.vertical: ScrollBar { id: scrollbar}
        }
    }

    function displayPage() {
        switch(mainBar.currentIndex) {
            case 0:
                fileContent.text = guiHelper.getInfoPage("Versions")
                licensesBar.visible = false
                break
            case 1:
                fileContent.text = guiHelper.getInfoPage("Authors")
                licensesBar.visible = false
                break
            case 2:
                fileContent.text = guiHelper.getInfoPage("Thanks")
                licensesBar.visible = false
                break
            case 3:
                switch(licensesBar.currentIndex) {
                    case 0: fileContent.text = guiHelper.getInfoPage("GPL-2"); break;
                    case 1: fileContent.text = guiHelper.getInfoPage("LGPL-2.1"); break;
                    case 2: fileContent.text = guiHelper.getInfoPage("BSD-3-Clause"); break;
                    default: fileContent.text = ""; break;
                }
                licensesBar.visible = true
                break
            default:
                fileContent.text = ""; break;
        }
        infoPageFlickable.contentY = 0
    }

}
