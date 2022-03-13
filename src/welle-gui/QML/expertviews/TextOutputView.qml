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

ViewBaseFrame {
    labelText: qsTr("Console Output")

    Timer {
        interval: 1000; // run every 1 s
        running: true;
        repeat: true
        onTriggered: {
            // Count total number of lines ("\n")
            var lines = (textField.text.match(/\n/g)|| []).length

            // Remove first line, keep the last 100 lines
            if(lines > 100 ) {
                var j = 0
                while (textField.text.charAt(j) !== '\n') {
                    j++
                }
                textField.text = textField.text.slice(j+1)
            }
        }
    }

    content:  Rectangle {
        anchors.fill: parent
        Flickable {
            id: flick
            anchors.fill: parent
            //flickableDirection: Flickable.VerticalFlick

            TextArea.flickable: TextArea {
                id: textField
                font.family: "Monospace"
                font.pixelSize: Units.em(0.9)
                background: Rectangle { color: "black" }
                color: "white"
                width: flick.width - scrollbar.width
                wrapMode: TextEdit.Wrap
                readOnly: true

                onLineCountChanged: {
                    // Scroll to the end
                    cursorPosition = length
                }
            }

            ScrollBar.vertical: ScrollBar { id: scrollbar }
        }
    }

    Connections{
        target: guiHelper

        function onNewDebugOutput(text) {
            textField.append(text)
        }
    }

    Component.onCompleted: {
        if(Qt.platform.os == "android") {
            infoMessagePopup.text = qsTr("Warning: The console view can slow down the complete app!");
            infoMessagePopup.open();
        }
    }
}
