import QtQuick 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "."

Item{
    id: textExpert
    property alias name: nameView.text
    property alias text: textView.text

    Layout.fillWidth: true
    height: textLayout.height

    RowLayout{
        id: textLayout

        Text{
            id: nameView
            font.pixelSize: Style.textStandartSize
            font.family: Style.textFont
            color: Style.textColor
        }
        Text{
            id: textView
            font.pixelSize: Style.textStandartSize
            font.family: Style.textFont
            color: Style.textColor
        }
    }
}
