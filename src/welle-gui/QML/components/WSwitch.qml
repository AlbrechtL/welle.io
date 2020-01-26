import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1

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
              color: (mainWindow.Material.theme === Material.Dark ) ? "lightgrey" : (mainWindow.Universal.theme === Universal.Dark ) ? "lightgrey" : TextStyle.textColor
              verticalAlignment: Text.AlignVCenter
              wrapMode: Text.WordWrap
              leftPadding: wSwitch.indicator.width + wSwitch.spacing
          }
}
