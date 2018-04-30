import QtQuick 2.5
import QtQuick.Controls 2.3

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
        NumberAnimation { property: "y"; from: hiddenY ; to: revealedY;}
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
            text: "Message Popup"
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            wrapMode: Text.WordWrap
            color: "white"
            onTextChanged: {
                if(paintedWidth + Units.dp(65) > mainWindow.width) width = mainWindow.width - Units.dp(65)
            }
        }

        Button {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            icon.name: "exit"
            icon.width: width
            icon.height: width
            icon.color: "white"
            padding: 0
            background: null
            height: Units.dp(30)
            width: height
        }

        MouseArea {
            anchors.fill: parent
            onClicked: popup.close()
        }
    }
}
