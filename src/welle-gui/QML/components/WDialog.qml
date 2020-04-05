import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1

// Import custom styles
import "../texts"
import "../components"

Dialog {
    id: popup
    default property alias content: placeholder.data

    // Place in the middle of window
    x: (parent.width - width) / 2

    height: parent.height
    width: parent.width < Units.dp(500)  ? parent.width : Units.dp(500)

    modal: true
    focus: true
    clip: true

    padding: Units.dp(10)

    header: ToolBar {
           RowLayout {
               anchors.fill: parent
               ToolButton {
                   font.pixelSize: TextStyle.textStandartSize
                   font.family: TextStyle.textFont

                   text: qsTr("‹")
                   onClicked: popup.close()
               }
               Label {
                   font.pixelSize: TextStyle.textStandartSize
                   font.family: TextStyle.textFont

                   text: qsTranslate("MainView", title)
                   elide: Label.ElideRight
                   horizontalAlignment: Qt.AlignHCenter
                   verticalAlignment: Qt.AlignVCenter
                   Layout.fillWidth: true
               }
           }
       }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: placeholder.childrenRect.height

        Item {
            id: placeholder
            width: parent.width - scrollbar.width
        }

        ScrollBar.vertical: ScrollBar { id: scrollbar }
    }
}
