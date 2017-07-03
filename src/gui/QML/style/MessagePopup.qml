import QtQuick 2.5
import QtQuick.Controls 2.1

// Import custom styles
import "."

Popup {
    property alias text: textView.text
    property alias color: backgroundRect.color
    property int revealedY: 0
    property int hiddenY: -150

    id: popup
    width: textView.width + Units.dp(65)
    height: Units.dp(50)
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
        }

        MouseArea {
            anchors.fill: parent
            onClicked: popup.close()
        }
    }
}
