import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

// Import custom styles
import "."

Button {
    style: touchStyle

    /* Button Style */
    Component {
        id: touchStyle
        ButtonStyle {
            panel: Item {
                implicitHeight: Units.dp(25)
                //implicitWidth: Units.dp(160)
                BorderImage {
                    anchors.fill: parent
                    antialiasing: true
                    border.bottom: Units.dp(8)
                    border.top: Units.dp(8)
                    border.left: Units.dp(8)
                    border.right: Units.dp(8)
                    anchors.margins: control.pressed ? Units.dp(-4) : 0
                    source: control.pressed ? "../images/button_pressed.png" : "../images/button_default.png"
                    Text {
                        text: control.text
                        anchors.centerIn: parent
                        color: control.enabled ? "white" : "grey"
                        font.pixelSize: Style.textStandartSize
                        font.family: Style.textFont
                        renderType: Text.NativeRendering
                    }
                }
            }
        }
    }
}
