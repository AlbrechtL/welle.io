import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "../texts"
import "../components"

SettingSection {
    spacing: Units.dp(20)
    text: qsTr("rtl-sdr settings")

    Settings {
        property alias enableHwAGCState: enableHwAGC.checked
    }

    Component.onCompleted: {
        radioController.setHwAGC(enableHwAGC.checked)
    }

    Switch {
        id: enableHwAGC
        text: qsTr("Hardware RF gain (not tested)")
        onClicked: {
            //radioController.setHwAGC(enableHwAGC.checked)
        }
    }
}
