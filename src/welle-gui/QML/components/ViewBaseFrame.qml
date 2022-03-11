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

// Import custom styles
import "../texts"
import "../components"

Rectangle {
    id: root
    color: (Universal.theme === Universal.Dark) ? "grey" : "white"
    //border.color: isExpert ? "lightgrey": "white"
    border.color: "lightgrey"

    Layout.fillHeight: true
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop | Qt.AlignLeft

    default property alias content: placeholder.data
    property alias labelText: label.text
    property string sourcePath
    property bool isExpert: false
    property bool isMaximize: false

    signal requestPositionChange(var sender, int row, int column)
    signal requestMaximize(var sender, bool isMaximize)
    signal itemRemove(var sender)

    Component {
        id: menuItem
        MenuItem {
            font.pixelSize: TextStyle.textStandartSize
        }
    }

    // Possibility to add options entries dynamically
    function addEntry(title, onTriggered) {
        var obj = menuItem.createObject(menu, {text: title})
        obj.triggered.connect(onTriggered)
        menu.insertItem(0, obj) // Put it to first position
    }

    Component.onCompleted: {
        requestPositionChange.connect(parent.onRequestPositionChange)
        requestMaximize.connect(parent.onRequestMaximize)
        itemRemove.connect(parent.onItemRemove)
    }


    Rectangle {
        id: rootBox
        anchors.fill: parent
        color: (Universal.theme === Universal.Dark) ? "darkgrey" : "lightgrey"
        visible: mouseArea.pressed
    }

    ColumnLayout {
        anchors.fill: parent

        ToolBar {
            Layout.fillWidth: true
            Layout.margins: Units.dp(1)
            Layout.topMargin: Units.dp(5)
            visible: isExpert
            implicitHeight: Units.dp(20)

            Accessible.name: qsTr("%1 title bar").arg(qsTr(label.text))
            Accessible.description: qsTr("%1 title bar to drag the element").arg(qsTr(label.text))

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true

                drag.target: root
                drag.axis: Drag.XAxis | Drag.YAxis
                drag.minimumX: 0
                drag.minimumY: 0

                onPressed: {
                    // Bring item to front
                    root.z = 1
                }

                onReleased: {
                    // Calculate row and column from current size
                    var column = (root.x / root.width)
                    var row = (root.y / root.height)
                    requestPositionChange(root, row.toFixed(0), column.toFixed(0))

                    // Reset stacking order
                    root.z = 0
                }
            }

            Label {
                id: label
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: TextStyle.textStandartSize
                Accessible.ignored: true
            }

            Button {
                id: menuButton
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: icon.width + Units.dp(20)

                icon.name: "menu"
                icon.height: Units.dp(10)
                icon.width: Units.dp(10)
                flat:true

                Accessible.name: qsTr("%1 menu").arg(qsTr(label.text))
                Accessible.description: qsTr("Show %1 menu").arg(qsTr(label.text))

                onClicked: menu.open()

                WMenu {
                    id: menu
                    sizeToContents: true
                    x: parent.width - width
                    transformOrigin: Menu.TopRight

                    MenuItem {
                        text: isMaximize ? qsTr("Minimize") : qsTr("Maximize")
                        font.pixelSize: TextStyle.textStandartSize
                        onTriggered: {
                            isMaximize = !isMaximize
                            requestMaximize(root, isMaximize)
                        }
                    }

                    MenuItem {
                        text: qsTr("Remove")
                        font.pixelSize: TextStyle.textStandartSize
                        onTriggered: {
                            itemRemove(root)
                            root.destroy()
                        }
                    }
                }
            }
        }

        Item {
            id: placeholder
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Units.dp(5)
        }
    }
}
