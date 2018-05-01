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
        spacing: Units.dp(20)

        Switch {
            id: enableHwAGC
            text: qsTr("Hardware RF gain (not tested)")
            height: 24
            Layout.fillHeight: true
            objectName: "enableHwAGC"
            //visible: (cppRadioController.isHwAGCSupported && enableExpertMode.checked) ? true : false
            //visible: false // This switch is only for debug purposes and not tested, so disable it by default
            visible: true
            onClicked: {
                cppGUI.inputEnableHwAGCChanged(checked)
            }
        }
    }
}
