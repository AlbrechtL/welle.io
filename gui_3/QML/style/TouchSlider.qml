import QtQuick 2.0
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

// Import custom styles
import "."

ColumnLayout {
    id: masterColumn

    signal valueChanged(double valueGain)

    property alias name: nameView.text

    Layout.preferredWidth: parent.width
    spacing: Units.dp(10)
    opacity: enabled ? 1 : 0.5

    RowLayout {
        Layout.preferredWidth: parent.width

        Text {
            id: nameView
            font.pixelSize: Style.textStandartSize
            font.family: Style.textFont
            color: Style.textColor
            Layout.alignment: Qt.AlignLeft
        }

        Text {
            id: valueView
            text: "Value: " + slider.value.toFixed(2) + " dB"
            font.pixelSize: Style.textStandartSize
            font.family: Style.textFont
            color: Style.textColor
            Layout.alignment: Qt.AlignRight
        }
    }

    Slider {
        id: slider
        style: sliderStyle
        maximumValue: 50
        minimumValue: 0

        Layout.preferredWidth: parent.width
        onValueChanged: masterColumn.valueChanged(value)

        /* Slider Style */
        Component {
            id: sliderStyle
            SliderStyle {
                handle: Rectangle {
                    width: Units.dp(18)
                    height: Units.dp(18)
                    radius: height
                    antialiasing: true
                    color: Qt.lighter("#468bb7", 1.2)
                }

                groove: Item {
                    Rectangle {
                        height: Units.dp(5)
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
}
