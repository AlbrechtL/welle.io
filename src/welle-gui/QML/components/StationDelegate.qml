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
 
import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtGraphicalEffects 1.0

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

    Rectangle {
        id: selectRecangle
        anchors.fill: parent
        color: (mainWindow.Material.theme === Material.Dark ) ? "dimgrey" : (mainWindow.Universal.theme === Universal.Dark ) ? "dimgrey" : "whitesmoke"
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

//                Item {
//                    Layout.preferredHeight: playbackStatusImage.height
//                    Layout.preferredWidth: playbackStatusImage.width
//                    Image {
//                        id: playbackStatusImage
//                        width: Units.dp(10)
//                        height: Units.dp(10)
//                        fillMode: Image.PreserveAspectFit
//                        visible:false
//                        source: "qrc:/icons/welle_io_icons/20x20/play.png"

//                        layer {
//                            enabled: true
//                            effect: ColorOverlay {
//                                color: (mainWindow.Material.theme === Material.Dark ) ? "lightgrey" : (mainWindow.Universal.theme === Universal.Dark ) ? "lightgrey" : TextStyle.textColor
//                            }
//                        }
//                    }
//                    Item {
//                        id: playbackStatusImageStrikethrough
//                        anchors.fill: playbackStatusImage

//                        Canvas {
//                            id: playbackStatusImageStrikethroughCanvas
//                            anchors.fill: parent
//                            onPaint: {
//                                var ctx = getContext("2d");
//                                ctx.strokeStyle = (mainWindow.Material.theme === Material.Dark ) ? "lightgrey" : (mainWindow.Universal.theme === Universal.Dark ) ? "lightgrey" : TextStyle.textColor
//                                ctx.lineWidth = 2
//                                ctx.lineCap = "round"
//                                ctx.beginPath()
//                                ctx.moveTo(0, parent.height)
//                                ctx.lineTo(parent.width, 0)
//                                ctx.stroke()
//                            }
//                        }
//                    }
//                }

                TextStation {
                    id: channelPlayStatus
                }
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

    Component.onCompleted: { setPlaybackStatus() }

//    SequentialAnimation {
//        id: playbackStatusImageEffect
//        running: false
//        loops: Animation.Infinite
//        NumberAnimation {
//            target: playbackStatusImage
//            property: "opacity"
//            from: 1
//            to: 0.2
//            duration: 1000;
//        }
//        NumberAnimation  {
//            target:playbackStatusImage
//            property: "opacity"
//            from: 0.2
//            to: 1
//            duration: 1000;
//        }
//    }

    Connections {
        target: radioController
        onIsPlayingChanged: setPlaybackStatus()
    }

    function setPlaybackStatus() {
//        if (stationSIdValue == radioController.autoService) {
//            playbackStatusImage.visible = true
//            if (radioController.isPlaying) {
//                //channelPlayStatus.text = ""
//                //playbackStatusImageEffect.start() //Don't use the animation because it consumes CPU
//                playbackStatusImage.opacity = 1
//                playbackStatusImageStrikethrough.visible = false
//            } else {
//                //channelPlayStatus.text = qsTr("stopped")
//                //playbackStatusImageEffect.stop() //Don't use the animation because it consumes CPU
//                playbackStatusImage.opacity = 0.6
//                playbackStatusImageStrikethrough.visible = true
//                playbackStatusImageStrikethroughCanvas.requestPaint()
//            }
//        }
//        else {
//            //channelPlayStatus.text = ""
//            playbackStatusImage.visible = false
//            playbackStatusImageStrikethrough.visible = false
//        }

        if (stationSIdValue === radioController.autoService)
            selectRecangle.visible = true
        else
            selectRecangle.visible = false
    }
}
