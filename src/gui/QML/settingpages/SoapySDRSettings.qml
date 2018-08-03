import QtQuick 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "../texts"
import "../components"

Item {
    implicitHeight: layout.implicitHeight
    implicitWidth:  layout.implicitWidth

    ColumnLayout{
        id: layout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        Layout.preferredWidth: Units.dp(100)
        spacing: Units.dp(20)

        TextStandart {
            text: qsTr("Not implemented yet")
        }
    }
}
