import QtQuick 2.5
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0

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
