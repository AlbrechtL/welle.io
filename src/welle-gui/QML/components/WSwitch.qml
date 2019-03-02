import QtQuick 2.6
import QtQuick.Controls 2.0

// Import custom styles
import "../texts"
import "../components"

Switch {
    id: wSwitch

    font.pixelSize: TextStyle.textStandartSize
    font.family: TextStyle.textFont
    height: Units.dp(24)

    contentItem: Text {
              text: wSwitch.text
              font: wSwitch.font
              verticalAlignment: Text.AlignVCenter
              wrapMode: Text.WordWrap
              leftPadding: wSwitch.indicator.width + wSwitch.spacing
          }
}
