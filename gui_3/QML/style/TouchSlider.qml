import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

// Import custom styles
import "."

Slider {
    anchors.margins: Units.dp(20)
    style: sliderStyle
    value: 1.0

    /* Slider Style */
    Component {
        id: sliderStyle
        SliderStyle {
            handle: Rectangle {
                width: Units.dp(15)
                height: Units.dp(15)
                radius: height
                antialiasing: true
                color: Qt.lighter("#468bb7", 1.2)
            }

            groove: Item {
                implicitWidth: Units.dp(200)
                Rectangle {
                    height: Units.dp(4)
                    width: parent.width
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#444"
                    opacity: 0.8
                    Rectangle {
                        antialiasing: true
                        radius: 1
                        color: "#468bb7"
                        height: parent.height
                        width: parent.width * control.value / control.maximumValue
                    }
                }
            }
        }
    }
}
