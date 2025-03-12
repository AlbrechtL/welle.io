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
import QtQuick.Controls.Universal
import Qt5Compat.GraphicalEffects

// Import custom styles
import "../texts"
import "../components"

Item {
    id: root
    width: parent == null ? undefined : parent.width
    height: Units.dp(44)

    property alias stationNameText: stationItem.text
    property string channelNameText
    property string availableChannelNamesText
    property string knownEnsembleNamesSerialized
    property int stationSIdValue
    property bool isExpert: false
    property bool isFavorit: false

    signal clicked
    signal favoritClicked
    signal setDefaultChannel(string newDefaultChannel)

    Rectangle {
        anchors.fill: parent
        color: (mainWindow.Universal.theme === Universal.Dark ) ? "dimgrey" : "lightgrey"
        visible: mouse.pressed
    }

    Rectangle {
        id: selectRecangle
        anchors.fill: parent
        color: (mainWindow.Universal.theme === Universal.Dark ) ? "dimgrey" : "whitesmoke"
        visible: false
    }

    MouseArea {
        id: mouse
        onClicked: root.clicked()
        anchors.fill: parent

        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: Units.dp(10)

            RowLayout {
                TextStandart {
                    id: stationItem
                }


                TextStation {
                    id: channelPlayStatus
                }
            }

            RowLayout {
                TextStation {
                    id: channelItem
                    visible: root.isExpert ? 1 : 0
                    text: availableChannelNamesText.split(',').length > 1 ? channelNameText + " (" + availableChannelNamesText + ")" : channelNameText
                }

                TextStation {
                    id: stationSIdItem
                    visible: root.isExpert ? 1 : 0
                    text: "0x" + stationSIdValue.toString(16).toUpperCase()
                }
            }
        }
    }

    Row {
        anchors.right: parent.right

        Button {
            icon.name: "ensemble_switch"
            icon.color: "grey"
            implicitWidth: contentItem.implicitWidth + Units.dp(15)
            flat: true
            visible: root.availableChannelNamesText.split(',').length > 1 ? 1 : 0
            onClicked: ensembleMenu.open()

            WMenu {
                id: ensembleMenu
                sizeToContents: true

                Instantiator {
                     id: recentFilesInstantiator
                     model: root.availableChannelNamesText.split(',')
                     delegate: MenuItem {
                         text: {
                             if(root.knownEnsembleNamesSerialized != "") {
                                 var knownEnsembleNames = JSON.parse(knownEnsembleNamesSerialized)
                                 for(const ensembleNameChannel in knownEnsembleNames) {
                                     if(ensembleNameChannel === modelData)
                                         return knownEnsembleNames[modelData]
                                 }
                             }

                             // Fallback if no ensemble name was found
                             return modelData
                         }
                         onTriggered: root.setDefaultChannel(modelData)
                     }

                     onObjectAdded: (index, object) => ensembleMenu.insertItem(index, object)
                     onObjectRemoved: (index, object) => ensembleMenu.removeItem(object)
                 }
            }
        }

        Button {
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

    Component.onCompleted: { setPlaybackStatus() }

    Connections {
        target: radioController
        function onIsPlayingChanged(){ setPlaybackStatus()}
    }

    function setPlaybackStatus() {
        if (stationSIdValue === radioController.autoService)
            selectRecangle.visible = true
        else
            selectRecangle.visible = false
    }
}
