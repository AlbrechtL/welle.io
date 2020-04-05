import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1


// Import custom styles
import "../texts"
import "../components"

ViewBaseFrame {
    labelText: qsTr("Console Output")

    Timer {
        interval: 1000; // run every 1 s
        running: true;
        repeat: true
        onTriggered: {
            // Count total number of lines ("\n")
            var lines = (textField.text.match(/\n/g)|| []).length

            // Remove first line, keep the last 100 lines
            if(lines > 100 ) {
                var j = 0
                while (textField.text.charAt(j) !== '\n') {
                    j++
                }
                textField.text = textField.text.slice(j+1)
            }
        }
    }

    content:  Rectangle {
        anchors.fill: parent
        Flickable {
            id: flick
            anchors.fill: parent
            //flickableDirection: Flickable.VerticalFlick

            TextArea.flickable: TextArea {
                id: textField
                font.family: "Monospace"
                font.pixelSize: Units.em(0.9)
                background: Rectangle { color: "black" }
                color: "white"
                width: flick.width - scrollbar.width
                wrapMode: TextEdit.Wrap
                readOnly: true

                onLineCountChanged: {
                    // Scroll to the end
                    cursorPosition = length
                }
            }

            ScrollBar.vertical: ScrollBar { id: scrollbar }
        }
    }

    Connections{
        target: guiHelper

        onNewDebugOutput: {
            textField.append(text)
        }
    }

    Component.onCompleted: {
        if(Qt.platform.os == "android") {
            infoMessagePopup.text = qsTr("Warning: The console view can slow down the complete app!");
            infoMessagePopup.open();
        }
    }
}
