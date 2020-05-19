/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1

// Import custom styles
import "../texts"
import "../components"

Item {
    id: root
    width: parent.width
    height: Units.dp(44)

    property alias stationNameText: stationItem.text
    property alias channelNameText: channelItem.text
    property int stationSIdValue
    property bool isExpert: false
    property bool isFavorit: false

    signal clicked
    signal favoritClicked

    Rectangle {
        anchors.fill: parent
        color: (mainWindow.Material.theme === Material.Dark ) ? "dimgrey" : (mainWindow.Universal.theme === Universal.Dark ) ? "dimgrey" : "lightgrey"
        visible: mouse.pressed
    }

    MouseArea {
        id: mouse
        onClicked: root.clicked()
        anchors.fill: parent

        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: Units.dp(10)

            TextStandart {
                id: stationItem
            }

            RowLayout {
                TextStation {
                    id: channelItem
                    visible: root.isExpert ? 1 : 0
                }

                TextStation {
                    id: stationSIdItem
                    visible: root.isExpert ? 1 : 0
                    text: "0x" + stationSIdValue.toString(16).toUpperCase()
                }
            }
        }
    }

    Button {
        anchors.right: parent.right
        icon.name: isFavorit ? "star_yellow" : "star"
        icon.color: "transparent"
        implicitWidth: contentItem.implicitWidth + Units.dp(20)
        flat: true
        onClicked: root.favoritClicked()

        Accessible.role: Accessible.Button
        Accessible.name: isFavorit ? qsTr("Remove station from favorites") : qsTr("Add station to favorites")
        Accessible.onPressAction: click(mouse)
    }
}
