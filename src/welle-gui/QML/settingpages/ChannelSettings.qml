import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

import "../texts"
import "../components"

Item {
    id: channelSettingsPage
    implicitHeight: layout.implicitHeight

    property alias addStationNameToWindowTitleState : addStationNameToWindowTitle.checked

    Settings {
        property alias enableLastPlayedStationState : enableLastPlayedStation.checked
        property alias addStationNameToWindowTitleState : channelSettingsPage.addStationNameToWindowTitleState
    }

    ColumnLayout{
        id: layout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Units.dp(20)

        WSwitch {
            id: enableLastPlayedStation
            text: qsTr("Automatic start playing last station")
            checked: false
            Layout.fillWidth: true
            onClicked: radioController.setAutoPlay(checked, radioController.lastChannel[1], radioController.lastChannel[0])
        }

        WSwitch {
            id: addStationNameToWindowTitle
            text: qsTr("Display station name in the window title")
            checked: false
            Layout.fillWidth: true
        }
    }
}
