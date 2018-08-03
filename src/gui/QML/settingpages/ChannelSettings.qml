import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

import "../texts"
import "../components"

Item {
    implicitHeight: layout.implicitHeight
    implicitWidth:  layout.implicitWidth

    Settings {
        property alias enableLastPlayedStationState : enableLastPlayedStation.checked
    }


    ColumnLayout{
        id: layout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Units.dp(20)

        Switch {
            id: enableLastPlayedStation
            text: qsTr("Automatic start playing last station")
            font.pixelSize: TextStyle.textStandartSize
            font.family: TextStyle.textFont
            height: Units.dp(24)
            checked: false
        }

        Button {
            id: clearListButton
            text: qsTr("Clear station list")
            font.pixelSize: TextStyle.textStandartSize
            font.family: TextStyle.textFont
            onClicked: cppGUI.clearStationList()
        }
    }
}
