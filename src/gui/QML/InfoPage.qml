import QtQuick 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "style"

Item {
    id: infoPage

    Flickable {
        id:infoFlickable
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.bottomMargin: 8
        anchors.topMargin: 8
        flickableDirection: Flickable.VerticalFlick
        anchors.fill: parent
        contentHeight: fileContent.implicitHeight
        anchors.margins: Units.dp(20)
        onFlickStarted: {
            infoScroll.stop()
            infoScrollTimeout.restart()
        }

        Rectangle {
            id: rectangle
            color: "#212126"
            anchors.fill: parent
        }

        TextStandart {
            id: fileContent
            width: parent.width
            text: cppGUI.licenses.FileContent
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 0
            Layout.alignment: Qt.AlignLeft
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            textFormat: Text.PlainText

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    infoScroll.stop()
                    infoScrollTimeout.restart()
                }
            }
        }

        Timer {
            id:infoScroll
            interval: 50; running: false; repeat: true
            onTriggered: {
                if(infoFlickable.contentY <= infoFlickable.contentHeight-infoFlickable.height){
                    infoFlickable.contentY+= 1;
                } else {
                    stop();
                }
            }
        }
        Timer {
            id:infoScrollTimeout
            interval: 5000; running: true; repeat: false
            onTriggered: infoScroll.restart();
        }

    }
}
