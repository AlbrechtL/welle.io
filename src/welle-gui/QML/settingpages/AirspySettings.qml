import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "../texts"
import "../components"

SettingSection {
    spacing: Units.dp(20)
    text: qsTr("Airspy settings")

    Settings {
        property alias airspyEnableBiasTeeState: enableBiasTee.checked
    }

    function initDevice(isAutoDevice) {
        if(!isAutoDevice)
            guiHelper.openAirspy()

        guiHelper.setBiasTeeAirspy(enableBiasTee.checked)
    }

    Switch {
        id: enableBiasTee
        text: qsTr("Enable bias tee")
        onClicked: {
            guiHelper.setBiasTeeAirspy(checked)
        }
    }
}
