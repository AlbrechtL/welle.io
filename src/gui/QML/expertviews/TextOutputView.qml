import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1


// Import custom styles
import "../texts"
import "../components"

Item {
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.preferredWidth: Units.dp(400)
    Layout.preferredHeight: Units.dp(400)

    Rectangle {
        color: "black"
        anchors.fill: parent

        Flickable {
            id: flick
            anchors.fill: parent
            //flickableDirection: Flickable.VerticalFlick

            TextArea.flickable: TextArea {
                id: textField
                font.family: "Monospace"
                font.pixelSize: Units.em(0.9)
                color: "white"
                width: flick.width
                wrapMode: TextEdit.Wrap

                onLineCountChanged: {
                    // Count total number of lines
                    var lines = 0
                    for(var i = 0; i<length; ++i) {
                        if(getText(i, i+1) === '\n')
                            lines++;
                    }

                    // Remove first line, keep tha last 100 lines
                    if(lines > 100 ) {
                        var j = 0
                        while (getText(j, j+1) !== '\n') {
                            j++
                        }
                        text = text.slice(j+1)
                    }
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
