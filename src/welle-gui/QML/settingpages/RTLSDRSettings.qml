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
        property alias rtlSdrEnableBiasTeeState: enableBiasTee.checked
    }

    WSwitch {
        id: enableBiasTee
        Layout.fillWidth: true
        text: qsTr("Enable bias tee (not from all dongles supported)")
        onClicked: {
            guiHelper.setBiasTeeRtlSdr(checked)
        }
    }

    function initDevice(isAutoDevice) {
        if(!isAutoDevice)
            guiHelper.openRtlSdr()

        guiHelper.setBiasTeeRtlSdr(enableBiasTee.checked)
    }
}
