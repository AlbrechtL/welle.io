import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.0

// Import custom styles
import "../texts"
import "../components"

Rectangle {
    id: root
    border.color: isExpert ? "lightgrey": "white"
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

    Component.onCompleted: {
        requestPositionChange.connect(parent.onRequestPositionChange)
        requestMaximize.connect(parent.onRequestMaximize)
        itemRemove.connect(parent.onItemRemove)
    }

    Rectangle {
        id: rootBox
        anchors.fill: parent
        color: "lightgrey"
        visible: mouseArea.pressed
    }

    ColumnLayout {
        anchors.fill: parent

        ToolBar {
            Layout.fillWidth: true
            Layout.topMargin: Units.dp(5)
            visible: isExpert

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

            background: Rectangle {
                implicitHeight: menuButton.height
                opacity: 0
            }

            Label {
                id: label
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: TextStyle.textStandartSize
            }

            ToolButton {
                id: menuButton
                anchors.right: parent.right
                icon.name: "menu"
                icon.height: Units.dp(10)
                icon.width: Units.dp(10)
                background: Rectangle {
                    color: menuButton.pressed ? "lightgrey" : "white"
                    opacity: menuButton.pressed ? 100 : 0
                }

                onClicked: optionsMenu.open()

                Menu {
                    id: optionsMenu
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
