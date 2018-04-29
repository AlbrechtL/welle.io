import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1

// Import custom styles
import "."

ColumnLayout {
    id: masterColumn

    signal valueChanged(double valueGain)

    property alias name: nameView.text
    property alias maximumValue: slider.to
    property alias showCurrentValue: valueView.text
    property alias currentValue: slider.value


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
            text: "Value: "
            font.pixelSize: Style.textStandartSize
            font.family: Style.textFont
            color: Style.textColor
            Layout.alignment: Qt.AlignRight
        }
    }

    Slider {
        id: slider
        from: 100
        to: 0
        value: 0
        stepSize: 1

        Layout.preferredWidth: parent.width
        onValueChanged: masterColumn.valueChanged(value)

    }
}
