import QtQuick 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "style"

Item {
    id: infoPage

    Flickable {
        anchors.fill: parent
        contentHeight: fileContent.implicitHeight
        contentWidth: fileContent.contentWidth
        anchors.margins: Units.dp(20)

        TextStandart {
            id: fileContent
            text: cppGUI.licenses.FileContent
            Layout.alignment: Qt.AlignLeft
            wrapMode: Text.Wrap
            width: infoPage.width
            textFormat: Text.PlainText
        }
    }
}
