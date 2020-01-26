pragma Singleton

import QtQuick 2.0
import "../components"

QtObject {
    // Text sizes
    property int textRadioStation: Units.em(2.3)
    property int textTitleSize: Units.em(2)
    property int textHeadingSize: Units.em(1.6)
    property int textStandartSize: Units.em(1.3)
    property int textRadioInfo: Units.em(1.2)
    property int textStation: Units.em(0.9)

    // Text font and color
    property string textFont: "Arial"
    //property string textFont: "Times"
    //property color textColor: "white"
    property color textColor: "black"
}
