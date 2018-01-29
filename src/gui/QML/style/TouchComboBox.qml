import QtQuick 2.6
import QtQuick.Controls 2.0

// Import custom styles
import "."

ComboBox {
    id: control

    font {
        pixelSize: Style.textStandartSize
        family: Style.textFont
    }

    delegate: ItemDelegate {
            width: control.width
            contentItem: Text {
                text: modelData
                color: Style.textColor
                font: control.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            highlighted: control.highlightedIndex === index

            background: Rectangle
            {
                 color: (highlighted || pressed) ? "#468bb7" : "#515154"

                 radius: Units.dp(2)
                 opacity: pressed ? 0.7 : 1
                 focus: true
            }
        }

        indicator: Canvas {
            id: canvas
            x: control.width - width - control.rightPadding
            y: control.topPadding + (control.availableHeight - height) / 2
            width: Units.dp(12)
            height: Units.dp(8)
            contextType: "2d"

            Connections {
                target: control
                onPressedChanged: canvas.requestPaint()
            }

            onPaint: {
                context.reset();
                context.moveTo(0, 0);
                context.lineTo(width, 0);
                context.lineTo(width / 2, height);
                context.closePath();
                context.fillStyle = Style.textColor;
                context.fill();
            }
        }

        contentItem: Text {
            leftPadding: Units.dp(12)
            rightPadding: control.indicator.width + control.spacing

            text: control.displayText
            font: control.font
            color: Style.textColor
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            implicitWidth: Units.dp(120)
            implicitHeight: Units.dp(40)
            border.color: control.pressed ? "#285c72" : "#88888a"
            border.width: control.pressed ? Units.dp(3) : Units.dp(1)
            color: control.pressed ? "#468bb7" : "#515154"
            radius: Units.dp(2)
        }

        popup: Popup {
            y: control.height - Units.dp(1)
            width: control.width
            implicitHeight: contentItem.implicitHeight
            padding: Units.dp(1)

            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: control.popup.visible ? control.delegateModel : null
                currentIndex: control.highlightedIndex

                ScrollIndicator.vertical: ScrollIndicator { }
            }

            background: Rectangle {
                border.color: "#88888a"
                color: "#515154"
                radius: Units.dp(2)
            }
        }
}
