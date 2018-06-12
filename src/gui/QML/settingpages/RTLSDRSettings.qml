import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

import "../texts"
import "../components"

Item {
    // Necessary for Flickable
    implicitHeight: layout.implicitHeight

    Settings {
        property alias enableHwAGCState: enableHwAGC.checked
    }

    Component.onCompleted: {
        cppGUI.inputEnableHwAGCChanged(enableHwAGC.checked)
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
                    cppGUI.inputEnableHwAGCChanged(checked)
                }
            }
        }
    }
}
