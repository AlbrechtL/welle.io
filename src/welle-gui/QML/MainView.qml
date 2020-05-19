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
    property bool isStationNameInWindowTitle: false

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

    title: isStationNameInWindowTitle ? radioController.title.trim() + " - welle.io" : "welle.io"

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

        updateTheme()

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

        RowLayout {
            spacing: 20
            anchors.fill: parent

            ToolButton {
                icon.name:  "drawer"

                Accessible.name: qsTr("Stations list")
                Accessible.description: qsTr("Display or hide stations list")

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
                icon.width: Units.dp(20)
                icon.height: Units.dp(20)

                Accessible.name: qsTr("Main menu")
                Accessible.description: qsTr("Show the main menu")

                onClicked: optionsMenu.open()

                WMenu {
                    id: optionsMenu
                    sizeToContents: true
                    x: parent.width - width
                    transformOrigin: Menu.TopRight

                    MenuItem {
                        text: qsTr("Settings")
                        font.pixelSize: TextStyle.textStandartSize
                        onTriggered: {
                            globalSettingsDialog.title = "Settings"
                            globalSettingsDialog.open()
                        }
                    }
                    MenuItem {
                        text: qsTr("Expert Settings")
                        font.pixelSize: TextStyle.textStandartSize
                        onTriggered: {
                            expertSettingsDialog.title = "Expert Settings"
                            expertSettingsDialog.open()
                        }
                    }
                    MenuItem {
                        text: qsTr("About")
                        font.pixelSize: TextStyle.textStandartSize
                        onTriggered: {
                            aboutDialog.title = "About"
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
                WComboBoxList {
                    id: stationListBox
                    textRole: 'trLabel'
                    model: ListModel {
                        id: stationListBoxModel
                        ListElement { label: "All stations"; trLabel: qsTr("All stations"); trContext: "MainView" }
                        ListElement { label: "Favorites"; trLabel: qsTr("Favorites"); trContext: "MainView" }
                    }
                    sizeToContents: true
//                    background: Rectangle {
//                        color: "white"
//                    }

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
                    flat:true
                    onClicked: stationMenu.open()
                    implicitWidth: contentItem.implicitWidth + Units.dp(15)

                    Accessible.name: qsTr("Stations menu")
                    Accessible.description: qsTr("Show stations menu")

                    WMenu {
                        id: stationMenu
                        sizeToContents: true

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
                                stationSettingsDialog.title = "Station settings"
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
                    channelNameText: channelName == "File" ? qsTr("File") : channelName
                    isFavorit: favorit
                    isExpert: isExpertView
                    onClicked: radioController.play(channelName, stationName, stationSId)
                    onFavoritClicked: {
                        var favoritInvert = !favorit
                        stationList.setFavorit(stationSId, channelName, favoritInvert) // Invert favorit

                        if(favoritInvert)
                            favoritsList.addStation(stationName, stationSId, channelName, true)
                        else
                            favoritsList.removeStation(stationSId, channelName);
                    }
                }

                ScrollIndicator.vertical: ScrollIndicator { }
            }

            RowLayout {
                Layout.margins: Units.dp(10)
                visible: isExpertView ? true : false

                Accessible.role: Accessible.ComboBox
                Accessible.name: manualChannelText.text + " " + manualChannelBox.currentText

                TextStandart {
                    id: manualChannelText
                    text: qsTr("Manual channel")
                    Layout.fillWidth: true

                    Accessible.ignored: true
                }

                WComboBox {
                    id: manualChannelBox
                    sizeToContents: true
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

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Add")
        Accessible.description: qsTr("Add a view")

        WMenu {
            id: viewMenu
            sizeToContents: true
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
            id: stationSettingsLoader
            anchors.right: parent.right
            anchors.left: parent.left
            height: item.implicitHeight
            source:  "qrc:/QML/settingpages/ChannelSettings.qml"
            onLoaded: isStationNameInWindowTitle = stationSettingsLoader.item.addStationNameToWindowTitleState
        }
        Connections {
            target: stationSettingsLoader.item
            onAddStationNameToWindowTitleStateChanged : isStationNameInWindowTitle = stationSettingsLoader.item.addStationNameToWindowTitleState
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
            onQQStyleThemeChanged: updateTheme()
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
            // On Linux (KDE?): Hide before we restore 
            // otherwise the window will occasionaly not be brought to the front
            if (Qt.platform.os === "linux" && !active) // Linux Workaround to display the window
                hide()
            showNormal()
            raise() // Stay in foreground
            if (Qt.platform.os === "linux" && !active) // Linux Workaround to display the window
                requestActivate()
        }
    }

    onVisibilityChanged: {
        if(visibility === Window.Minimized)
            guiHelper.tryHideWindow()
    }

    function updateTheme() {
        if (guiHelper.getQQStyle === "Universal") {
            switch(globalSettingsLoader.item.qQStyleTheme) {
                case 0: mainWindow.Universal.theme = Universal.Light; break;
                case 1: mainWindow.Universal.theme = Universal.Dark; break;
                case 2: mainWindow.Universal.theme = Universal.System; break;
            }
        }
        else if (guiHelper.getQQStyle === "Material") {
            switch(globalSettingsLoader.item.qQStyleTheme) {
                case 0: mainWindow.Material.theme = Material.Light; break;
                case 1: mainWindow.Material.theme = Material.Dark; break;
                case 2: mainWindow.Material.theme = Material.System; break;
            }
        }
    }
}
