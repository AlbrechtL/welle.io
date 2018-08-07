import QtQuick 2.0
import QtQuick.Controls 2.4
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
            anchors.fill: parent
            //flickableDirection: Flickable.VerticalFlick

            TextArea.flickable: TextArea {
                id: textField
                font.family: "Monospace"
                font.pixelSize: Units.em(0.9)
                color: "white"
                width: parent.width
                wrapMode: TextEdit.Wrap
            }

            ScrollBar.vertical: ScrollBar { }
        }
    }

    Connections{
        target: cppGUI

        onNewDebugOutput: {
            textField.append(text)
        }
    }
}
