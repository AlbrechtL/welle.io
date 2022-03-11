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
import QtQuick.Controls
import QtQuick.Layouts

// Import custom styles
import "../texts"
import "../components"

Dialog {
    id: popup
    default property alias content: placeholder.data

    // Place in the middle of window
    x: (parent.width - width) / 2

    height: parent.height
    width: parent.width < Units.dp(500)  ? parent.width : Units.dp(500)

    modal: true
    focus: true
    clip: true

    padding: Units.dp(10)

    header: ToolBar {
           RowLayout {
               anchors.fill: parent
               ToolButton {
                   font.pixelSize: TextStyle.textStandartSize

                   icon.source: "qrc:/icons/welle_io_icons/20x20/back.png"
                   icon.width: Units.dp(16)
                   icon.height: Units.dp(16)
                   onClicked: popup.close()
               }
               Label {
                   font.pixelSize: TextStyle.textStandartSize

                   text: qsTranslate("MainView", title)
                   elide: Label.ElideRight
                   horizontalAlignment: Qt.AlignHCenter
                   verticalAlignment: Qt.AlignVCenter
                   Layout.fillWidth: true
               }
           }
       }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: placeholder.childrenRect.height

        Item {
            id: placeholder
            width: parent.width - scrollbar.width
        }

        ScrollBar.vertical: ScrollBar { id: scrollbar }
    }
}
