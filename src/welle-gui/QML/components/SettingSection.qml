import QtQuick 2.6
import QtQuick.Layouts 1.1

import "../texts"

ColumnLayout{
    property alias isNotFirst : line.visible
    property alias text : text.text

    spacing: Units.dp(20)
//    Layout.fillWidth: true

    Accessible.role: Accessible.StaticText
    Accessible.name: text.text

    ColumnLayout{
        Rectangle {
            id: line
            Layout.fillWidth: true
            height: 1
            color: "transparent"
            border.color: "lightgrey"
        }

        Text {
            id: text
            font.pixelSize: TextStyle.textStandartSize
            font.family: TextStyle.textFont
            color: "dimgrey"
        }
    }
}
