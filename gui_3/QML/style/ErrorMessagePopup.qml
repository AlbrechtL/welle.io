import QtQuick 2.0
import QtQuick.Controls 2.0

// Import custom styles
import "."

Popup {
    property alias text: textView.text

    id: popup
    x: mainWindow.width/2 - width/2
    y: mainWindow.height  - toolBar.height - height
    width: textView.width + Units.dp(65)
    height: Units.dp(50)
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    enter:  Transition {
            NumberAnimation { property: "y"; from: mainWindow.height ; to: mainWindow.height - toolBar.height - popup.height;}
        }
    exit: Transition {
            NumberAnimation { property: "y"; from: mainWindow.height - toolBar.height - popup.height ; to: mainWindow.height;}
        }
    //onOpened: closeTimer.running = true;

    TextStandart {
        id: textView
        text: "Error Message Popup"
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

    /*Timer {
        id: closeTimer
        interval: 1 * 1000 // 10 s
        repeat: false
        onTriggered: {
           popup.close()
        }
    }*/
}
