/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Window 2.2
import Qt.labs.settings 1.0

import "texts"
import "settingpages"
import "components"

ApplicationWindow {
    id: mainWindow

    property bool isExpertView: false
    property bool isFullScreen: false
    property bool isLoaded: false

    StationListModel { id: stationList }
    StationListModel { id: favoritsList }

    readonly property bool inPortrait: mainWindow.width < mainWindow.height

    function getWidth() {
        if(Screen.desktopAvailableWidth < Units.dp(700)
                || Screen.desktopAvailableHeight < Units.dp(500)
                || Qt.platform.os == "android") // Always full screen on Android
            return Screen.desktopAvailableWidth
        else
            return Units.dp(700)
    }

    function getHeight() {
        if(Screen.desktopAvailableHeight < Units.dp(500)
                || Screen.desktopAvailableWidth < Units.dp(700)
                || Qt.platform.os == "android")  // Always full screen on Android
            return Screen.desktopAvailableHeight
        else
            return Units.dp(500)
    }

    width: getWidth()
    height: getHeight()

    visibility: isFullScreen ? Window.FullScreen : Window.Windowed

    Component.onCompleted: {
        console.debug("os: " + Qt.platform.os)
        console.debug("desktopAvailableWidth: " + Screen.desktopAvailableWidth)
        console.debug("desktopAvailableHeight: " + Screen.desktopAvailableHeight)
        console.debug("orientation: " + Screen.orientation)
        console.debug("devicePixelRatio: " + Screen.devicePixelRatio)
        console.debug("pixelDensity: " + Screen.pixelDensity)

        // Reset window settings if OS is Android (it is a little bit hacky)
        if(Qt.platform.os == "android") {
            mainWindow.width = getWidth()
            mainWindow.height = getHeight()
        }

        // Show error message if one occured during startup
        if(errorMessagePopup.text != "")
            errorMessagePopup.open();

        isLoaded = true
    }

    Settings {
        property alias width : mainWindow.width
        property alias height : mainWindow.height
        property alias stationListSerialize: stationList.serialized
        property alias favoritsListSerialize: favoritsList.serialized
        property alias stationListBoxIndex: stationListBox.currentIndex
    }

    header: ToolBar {
        id: overlayHeader
        Material.foreground: "white"

        RowLayout {
            spacing: 20
            anchors.fill: parent

            ToolButton {
                icon.name:  "drawer"
                onClicked: {
                    if (stationDrawer.visible)
                    {
                        stationDrawer.close()
                    }
                    else
                    {
                        // Workaround for touch displays. (Discovered with Windows 10)
                        // For some reason the dawer will be closed before it is openend
                        // Disbale closing
                        stationDrawer.closePolicy = Popup.NoAutoClose

                        // Open drawer
                        stationDrawer.open()
                    }
                }
            }

            Label {
                id: titleLabel
                text: "welle.io"
                font.pixelSize: Units.dp(20)
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }

            ToolButton {
                icon.name: "menu"
                onClicked: optionsMenu.open()

                Menu {
                    id: optionsMenu
                    x: parent.width - width
                    transformOrigin: Menu.TopRight

                    MenuItem {
                        text: qsTr("Settings")
                        font.pixelSize: TextStyle.textStandartSize
                        onTriggered: {
                            globalSettingsDialog.title = text
                            globalSettingsDialog.open()
                        }
                    }
                    MenuItem {
                        text: qsTr("Expert Settings")
                        font.pixelSize: TextStyle.textStandartSize
                        onTriggered: {
                            expertSettingsDialog.title = text
                            expertSettingsDialog.open()
                        }
                    }
                    MenuItem {
                        text: qsTr("About")
                        font.pixelSize: TextStyle.textStandartSize
                        onTriggered: {
                            aboutDialog.title = text
                            aboutDialog.open()
                        }
                    }
                    MenuItem {
                        text: qsTr("Exit")
                        font.pixelSize: TextStyle.textStandartSize
                        onTriggered: guiHelper.close()
                    }
                }
            }
        }
    }

    Drawer {
        id: stationDrawer

        y: overlayHeader.height
        width: mainWindow.width < 1.5 * implicitWidth ? mainWindow.width : Units.dp(320)
        height: mainWindow.height - overlayHeader.height

        modal: inPortrait
        interactive: inPortrait
        position: inPortrait ? 0 : 1
        visible: !inPortrait

        // Workaround for touch displays. (Discovered with Windows 10)
        // For some reason the dawer will be closed before it is openend
        // Enable closing again
        onOpened: closePolicy = Popup.CloseOnEscape | Popup.CloseOnPressOutside

        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: Units.dp(5)

            RowLayout {
                WComboBox {
                    id: stationListBox
                    Layout.preferredWidth: Units.dp(200)
                    background: Rectangle { color: "white" }

                    model:  [qsTr("All stations"), qsTr("Favorites")]

                    onCurrentIndexChanged: {
                        switch(currentIndex) {
                        case 0: stationChannelView.model = stationList; break;
                        case 1: stationChannelView.model = favoritsList; break;
                        }
                    }
                }

                Item { // Dummy element for filling
                    Layout.fillWidth: true
                }

                Button {
                    id: menuButton
                    icon.name: "menu"
                    icon.height: Units.dp(15)
                    icon.width: Units.dp(15)
                    background: Rectangle {
                        color: menuButton.pressed ? "lightgrey" : "white"
                        opacity: menuButton.pressed ? 100 : 0
                    }
                    onClicked: stationMenu.open()
                    implicitWidth: contentItem.implicitWidth + Units.dp(15)

                    Menu {
                        id: stationMenu

                        MenuItem {
                            id: startStationScanItem
                            text: qsTr("Start station scan")
                            font.pixelSize: TextStyle.textStandartSize
                            font.family: TextStyle.textFont
                            onTriggered:  {
                                startStationScanItem.enabled = false
                                stopStationScanItem.enabled = true
                                radioController.startScan()
                            }
                        }

                        MenuItem {
                            id: stopStationScanItem
                            text: qsTr("Stop station scan")
                            font.pixelSize: TextStyle.textStandartSize
                            font.family: TextStyle.textFont
                            enabled: false
                            onTriggered:  {
                                startStationScanItem.enabled = true
                                stopStationScanItem.enabled = false
                                radioController.stopScan()
                            }
                        }

                        MenuItem {
                            text: qsTr("Clear station list")
                            font.pixelSize: TextStyle.textStandartSize
                            font.family: TextStyle.textFont
                            onTriggered: stationList.clearStations()
                        }

                        MenuItem {
                            id: stationSettingsItem
                            text: qsTr("Station settings")
                            font.pixelSize: TextStyle.textStandartSize
                            font.family: TextStyle.textFont
                            onTriggered: {
                                stationSettingsDialog.title = text
                                stationSettingsDialog.open()
                            }
                        }
                    }
                }
            }

            TextStandart {
                text: qsTr("No stations in list")
                visible: stationChannelView.count ? false : true
                Layout.margins: Units.dp(10)
            }

            ListView {
                id: stationChannelView
                model: stationList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                delegate: StationDelegate {
                    stationNameText: stationName
                    stationSIdValue: stationSId
                    channelNameText: channelName
                    isFavorit: favorit
                    isExpert: isExpertView
                    onClicked: radioController.play(channelName, stationName, stationSId)
                    onFavoritClicked: {
                        var favoritInvert = !favorit
                        stationList.setFavorit(stationSId, favoritInvert) // Invert favorit

                        if(favoritInvert)
                            favoritsList.addStation(stationName, stationSId, channelName, true)
                        else
                            favoritsList.removeStation(stationSId);
                    }
                }

                ScrollIndicator.vertical: ScrollIndicator { }
            }

            RowLayout {
                Layout.margins: Units.dp(10)
                visible: isExpertView ? true : false

                TextStandart {
                    text: qsTr("Manual channel")
                    Layout.fillWidth: true
                }

                WComboBox {
                    id: manualChannelBox
                    model: ["5A", "5B", "5C", "5D",
                        "6A", "6B", "6C", "6D",
                        "7A", "7B", "7C", "7D",
                        "8A", "8B", "8C", "8D",
                        "9A", "9B", "9C", "9D",
                        "10A", "10B", "10C", "10D",
                        "11A", "11B", "11C", "11D",
                        "12A", "12B", "12C", "12D",
                        "13A", "13B", "13C", "13D", "13E", "13F",
                        "LA", "LB", "LC", "LD",
                        "LE", "LF", "LG", "LH",
                        "LI", "LJ", "LK", "LL",
                        "LM", "LN", "LO", "LP"]

                    Layout.preferredHeight: Units.dp(25)
                    Layout.preferredWidth: Units.dp(130)
                    onActivated: {
                        radioController.setManualChannel(model[index])
                    }
                }
            }
        }
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        anchors.leftMargin: (!inPortrait && stationDrawer.opened) ? stationDrawer.width: undefined

        GeneralView {
            id: generalView
            isExpert: isExpertView
            isPortrait: inPortrait
        }
    }

    RoundButton {
        text: "\u002b" // Unicode character '+'
        onClicked: viewMenu.open()
        x: parent.width - width - Units.dp(10)
        y: parent.height - height - Units.dp(10)
        visible: isExpertView
        palette.button: "darkorange"

        Menu {
            id: viewMenu
            transformOrigin: Menu.TopRight

            MenuItem {
                text: qsTr("Service Overview")
                font.pixelSize: TextStyle.textStandartSize
                onTriggered: generalView.addComponent("qrc:/QML/RadioView.qml", -1 ,-1)
            }

            MenuItem {
                text: qsTr("Service Details")
                font.pixelSize: TextStyle.textStandartSize
                onTriggered: generalView.addComponent("qrc:/QML/expertviews/ServiceDetails.qml", -1 ,-1)
            }

            MenuItem {
                text: qsTr("MOT Slide Show")
                font.pixelSize: TextStyle.textStandartSize
                onTriggered: generalView.addComponent("qrc:/QML/MotView.qml", -1 ,-1)
            }

            MenuItem {
                text: qsTr("Spectrum")
                font.pixelSize: TextStyle.textStandartSize
                onTriggered: generalView.addComponent("qrc:/QML/expertviews/SpectrumGraph.qml", -1 ,-1)
            }

            MenuItem {
                text: qsTr("Impulse Response")
                font.pixelSize: TextStyle.textStandartSize
                onTriggered: generalView.addComponent("qrc:/QML/expertviews/ImpulseResponseGraph.qml", -1 ,-1)
            }

            MenuItem {
                text: qsTr("Constellation Diagram")
                font.pixelSize: TextStyle.textStandartSize
                onTriggered: generalView.addComponent("qrc:/QML/expertviews/ConstellationGraph.qml", -1 ,-1)
            }

            MenuItem {
                text: qsTr("Null Symbol")
                font.pixelSize: TextStyle.textStandartSize
                onTriggered: generalView.addComponent("qrc:/QML/expertviews/NullSymbolGraph.qml", -1 ,-1)
            }

            MenuItem {
                text: qsTr("Console Output")
                font.pixelSize: TextStyle.textStandartSize
                onTriggered: generalView.addComponent("qrc:/QML/expertviews/TextOutputView.qml", -1 ,-1)
            }

            MenuItem {
                text: qsTr("RAW Recorder")
                font.pixelSize: TextStyle.textStandartSize
                onTriggered: generalView.addComponent("qrc:/QML/expertviews/RawRecorder.qml", -1 ,-1)
            }
        }
    }

    WDialog {
        id: aboutDialog

        contentItem: InfoPage{
            id: infoPage
        }
    }

    WDialog {
        id: stationSettingsDialog
        content: Loader {
            anchors.right: parent.right
            anchors.left: parent.left
            height: item.implicitHeight
            source:  "qrc:/QML/settingpages/ChannelSettings.qml"
        }
    }

    WDialog {
        id: globalSettingsDialog

        content: Loader {
            id: globalSettingsLoader
            anchors.right: parent.right
            anchors.left: parent.left
            height: item.implicitHeight
            source:  "qrc:/QML/settingpages/GlobalSettings.qml"
            onLoaded : isFullScreen = globalSettingsLoader.item.enableFullScreenState
        }

        Connections {
            target: globalSettingsLoader.item
            onEnableFullScreenStateChanged : isFullScreen = globalSettingsLoader.item.enableFullScreenState
        }
    }

    WDialog {
        id: expertSettingsDialog

        content: Loader {
            id: expertSettingsLoader
            anchors.right: parent.right
            anchors.left: parent.left
            height: item.implicitHeight
            source:  "qrc:/QML/settingpages/ExpertSettings.qml"
            onLoaded: isExpertView = expertSettingsLoader.item.enableExpertModeState
        }

        Connections {
            target: expertSettingsLoader.item
            onEnableExpertModeStateChanged : isExpertView = expertSettingsLoader.item.enableExpertModeState
        }
    }

    MessagePopup {
        id: errorMessagePopup
        x: mainWindow.width/2 - width/2
        y: mainWindow.height  - overlayHeader.height - height
        revealedY: mainWindow.height - overlayHeader.height - height
        hiddenY: mainWindow.height
        color: "#8b0000"
    }

    MessagePopup {
        id: infoMessagePopup
        x: mainWindow.width/2 - width/2
        y: mainWindow.height  - overlayHeader.height - height
        revealedY: mainWindow.height - overlayHeader.height - height
        hiddenY: mainWindow.height
        color:  "#468bb7"
        onOpened: closeTimer.running = true;
        Timer {
            id: closeTimer
            interval: 1 * 5000 // 5 s
            repeat: false
            onTriggered: {
                infoMessagePopup.close()
            }
        }
    }

    Connections{
        target: radioController

        onShowErrorMessage:{
            errorMessagePopup.text = Text;

            if(mainWindow.isLoaded)
                errorMessagePopup.open();
        }

        onShowInfoMessage:{
            infoMessagePopup.text = Text;
            infoMessagePopup.open();
        }

        onScanStopped:{
            startStationScanItem.enabled = true
            stopStationScanItem.enabled = false
        }

        onScanProgress:{
            startStationScanItem.enabled = false
            stopStationScanItem.enabled = true
        }

        onNewStationNameReceived: stationList.addStation(station, sId, channel, false)
    }

    Connections {
        target: guiHelper

        onMinimizeWindow: hide()
        onMaximizeWindow: showMaximized()
        onRestoreWindow: {
            showNormal()
            raise() // Stay in foreground
        }
    }

    onVisibilityChanged: {
        if(visibility === Window.Minimized)
            guiHelper.tryHideWindow()
    }
}
