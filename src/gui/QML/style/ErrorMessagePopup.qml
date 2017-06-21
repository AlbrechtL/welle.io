import QtQuick 2.0
import QtQuick.Controls 2.0

// Import custom styles
import "."

Popup {
    property alias text: textView.text

    id: popup
    width: (textView.width + Units.dp(65)) < mainWindow.width ? (textView.width + Units.dp(65)) : mainWindow.width
    height: (textView.paintedHeight + Units.dp(10)) < Units.dp(50) ? Units.dp(50) : (textView.paintedHeight + Units.dp(10))
    x: mainWindow.width/2 - width/2
    y: mainWindow.height  - toolBar_.height - height

    closePolicy: Popup.NoAutoClose
    enter:  Transition {
            NumberAnimation { property: "y"; from: mainWindow.height ; to: mainWindow.height - toolBar.height - popup.height;}
        }
    exit: Transition {
            NumberAnimation { property: "y"; from: mainWindow.height - toolBar.height - popup.height ; to: mainWindow.height;}
        }

    TextStandart {
        id: textView
        text: "Error Message Popup"
        width: (paintedWidth + Units.dp(65)) > mainWindow.width ? mainWindow.width - Units.dp(65): implicitWidth
        wrapMode: Text.WordWrap
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
    }

    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        Image {
            anchors.verticalCenter: parent.verticalCenter
            source: "../images/icon-exit.png"
            height: Units.dp(25)
            fillMode: Image.PreserveAspectFit
        }
        MouseArea {
            scale: 1
            anchors.fill: parent
            anchors.margins: Units.dp(-20)
            onClicked: popup.close()
        }
    }

    background: Rectangle {color: "darkRed"}
}
