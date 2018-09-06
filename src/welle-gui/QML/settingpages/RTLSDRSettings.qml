import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "../texts"
import "../components"

Item {
    implicitHeight: layout.implicitHeight
    implicitWidth:  layout.implicitWidth

    Settings {
        property alias enableHwAGCState: enableHwAGC.checked
    }

    Component.onCompleted: {
        radioController.setHwAGC(enableHwAGC.checked)
    }

    ColumnLayout{
        id: layout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        Layout.preferredWidth: Units.dp(100)
        spacing: Units.dp(20)

        SettingSection {
            text: qsTr("Gain")
            isNotFirst: false

            Switch {
                id: enableHwAGC
                text: qsTr("Hardware RF gain (not tested)")
                onClicked: {
                    radioController.setHwAGC(enableHwAGC.checked)
                }
            }
        }
    }
}
