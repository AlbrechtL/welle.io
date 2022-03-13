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
import Qt5Compat.GraphicalEffects

import "../texts"

Popup {
    property alias text: textView.text
    property alias color: backgroundRect.color
    property int revealedY: 0
    property int hiddenY: -150

    id: popup
    width: textView.width + Units.dp(65)
    height: (textView.paintedHeight + Units.dp(10)) < Units.dp(50) ? Units.dp(50) : (textView.paintedHeight + Units.dp(10))
    closePolicy: Popup.NoAutoClose
    enter:  Transition {
        NumberAnimation { property: "y"; from: hiddenY; to: revealedY;}
    }
    exit: Transition {
        NumberAnimation { property: "y"; from: revealedY; to: hiddenY;}
    }

    background: Rectangle {
        id:backgroundRect
        color: "#8b0000"
    }
    contentItem: Item {
        TextStandart {
            id: textView
            text: ""
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            wrapMode: Text.WordWrap
            color: "white"
            onTextChanged: {
                if(paintedWidth + Units.dp(65) > mainWindow.width) width = mainWindow.width - Units.dp(65)
            }
        }

        Image {
            id:exitImage
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            source: "qrc:/icons/welle_io_icons/20x20@2/exit.png"
            height: Units.dp(30)
            width: height
        }
        
        ColorOverlay {
            anchors.fill: exitImage
            source: exitImage
            color: "white"
        }

        MouseArea {
            id: popupMA
            anchors.fill: parent
            onClicked: popup.close()
        }

        Accessible.role: Accessible.AlertMessage
        Accessible.name: popup.text
        Accessible.onPressAction: popupMA.click(mouse)
    }
}
