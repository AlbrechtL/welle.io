import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

// Import custom styles
import "."

ProgressBar{
    property alias text: textView.text

    style: progressBarStyle
    implicitWidth: parent.width
    Text {
        id: textView
        font.pixelSize: Style.textStandartSize
        font.family: Style.textFont
        color: Style.textColor
        anchors.centerIn: parent
    }

    /* ProgressBar Style */
    Component {
        id: progressBarStyle
        ProgressBarStyle {
            panel: Rectangle {
                implicitHeight: Units.dp(25)
                implicitWidth: Units.dp(400)
                color: "#444"
                opacity: 0.8
                Rectangle {
                    antialiasing: true
                    radius: Units.dp(1)
                    color: "#468bb7"
                    height: parent.height
                    width: parent.width * control.value / control.maximumValue
                }
            }
        }
    }
}

