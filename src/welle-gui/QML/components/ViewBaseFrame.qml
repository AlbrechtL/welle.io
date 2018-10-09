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

    ColumnLayout {
        anchors.fill: parent

        ToolBar {
            Layout.fillWidth: true
            Layout.topMargin: Units.dp(5)
            visible: isExpert

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
                        text: qsTr("Remove")
                        font.pixelSize: TextStyle.textStandartSize
                        onTriggered: root.destroy()
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
