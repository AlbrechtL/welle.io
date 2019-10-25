import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1


// Import custom styles
import "../texts"
import "../components"

ViewBaseFrame {
    labelText: qsTr("Console Output")

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
                width: flick.width
                wrapMode: TextEdit.Wrap
                readOnly: true

                onLineCountChanged: {
                    // Count total number of lines ("\n")
                    var lines = (text.match(/\n/g)|| []).length

                    // Remove first line, keep tha last 100 lines
                    if(lines > 100 ) {
                        var j = 0
                        while (text.charAt(j) !== '\n') {
                            j++
                        }
                        text = text.slice(j+1)
                    }

                    // Scroll to the end
                    cursorPosition = length
                }
            }

            ScrollBar.vertical: ScrollBar { }
        }
    }

    Connections{
        target: guiHelper

        onNewDebugOutput: {
            textField.append(text)
        }
    }
}
