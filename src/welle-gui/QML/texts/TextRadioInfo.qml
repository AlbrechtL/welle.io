import QtQuick 2.0

// Import custom styles
import "."

Text {
    font.pixelSize: TextStyle.textRadioInfo
    font.family: TextStyle.textFont
    //color: TextStyle.textColor

    Accessible.role: Accessible.StaticText
    Accessible.name: text
}
