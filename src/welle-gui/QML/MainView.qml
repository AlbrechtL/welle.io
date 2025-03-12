/*
 *    Copyright (C) 2017 - 2021
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Window
import Qt5Compat.GraphicalEffects
import QtCore

import "texts"
import "settingpages"
import "components"

ApplicationWindow {
    id: mainWindow

    property bool isExpertView: false
    property bool isFullScreen: false
    property bool isLoaded: false
    property bool isStationNameInWindowTitle: false
    property string knownEnsembleNamesSerialized

    StationListModel { id: stationList ; type: "all"}
    StationListModel { id: favoritsList ; type: "favorites"}

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

    visible: true // According to https://bugreports.qt.io/browse/QTBUG-35244
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

        // Show error message if one occurred during startup
        if(errorMessagePopup.text != "")
            errorMessagePopup.open();

        updateTheme()

        guiHelper.updateMprisStationList(stationChannelView.model.serialized,
                                         stationChannelView.model.type,
                                         stationListBox.currentIndex)

        isLoaded = true
    }

    Settings {
        property alias width : mainWindow.width
        property alias height : mainWindow.height
        property alias stationListSerialize: stationList.serialized
        property alias favoritsListSerialize: favoritsList.serialized
        property alias stationListBoxIndex: stationListBox.currentIndex
        property alias volume: volumeSlider.value
        property alias knownEnsembleNamesSerialized: mainWindow.knownEnsembleNamesSerialized
    }

    header: ToolBar {
        id: overlayHeader

        RowLayout {
            anchors.fill: parent

            ToolButton {
                icon.name: "drawer"

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
                        // For some reason the dawer will be closed before it is opened
                        // Disable closing
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
                id: startStopIcon
                implicitWidth: icon.width + Units.dp(20)
                icon.name: "stop"

                onClicked: {
                    if (radioController.isPlaying || radioController.isChannelScan) {
                        startStopIcon.stop()
                    } else {
                        startStopIcon.play()
                    }
                }

                Accessible.role: Accessible.Button
                Accessible.name: (radioController.isPlaying || radioController.isChannelScan) ? qsTr("Stop") : qsTr("Play")
                Accessible.description: radioController.isPlaying ? qsTr("Stop playback") : radioController.isChannelScan ? qsTr("Stop scan") : qsTr("Start playback")
                Accessible.onPressAction: startStopIconMouseArea.clicked(mouse)

                WToolTip {
                    text: (radioController.isPlaying || radioController.isChannelScan) ? qsTr("Stop") : qsTr("Play")
                    visible: startStopIcon.hovered
                }

                Component.onCompleted: { startStopIcon.setStartPlayIcon() }

                Shortcut {
                    context: Qt.ApplicationShortcut
                    autoRepeat: false
                    sequences: ["Media Pause", "Toggle Media Play/Pause", "S"]
                    onActivated: startStopIconMouseArea.clicked(0)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut
                    autoRepeat: false
                    sequences: ["Media Stop"]
                    onActivated: if (radioController.isPlaying || radioController.isChannelScan) startStopIcon.stop()
                }
                Shortcut {
                    context: Qt.ApplicationShortcut
                    autoRepeat: false
                    sequences: ["Media Play"]
                    onActivated: if (!radioController.isPlaying) startStopIcon.play()
                }

                Connections {
                    target: radioController
                    function onIsPlayingChanged() {
                        startStopIcon.setStartPlayIcon()
                    }
                    function onIsChannelScanChanged() {
                        startStopIcon.setStartPlayIcon()
                    }
                }

                function setStartPlayIcon() {
                    if (radioController.isPlaying || radioController.isChannelScan) {
                        startStopIcon.icon.name = "stop"
                    } else {
                        startStopIcon.icon.name = "play"
                    }
                }

                function play() {
                    var channel = radioController.lastChannel[1]
                    var sidHex = radioController.lastChannel[0]
                    stationList.play(channel, sidHex)
                }

                function stop() {
                    if (radioController.isPlaying)
                        radioController.stop();
                    else if (radioController.isChannelScan)
                        radioController.stopScan()
                }
            }

            ToolButton {
                id: speakerIconContainer
                implicitWidth: icon.width + Units.dp(24)
                icon.name: "speaker"

                onPressAndHold: volumePopup.open()
                onClicked: {
                    if(radioController.volume !== 0) {
                        volumeSlider.valueBeforeMute = volumeSlider.value
                        volumeSlider.value = 0
                    }
                    else {
                        volumeSlider.value = volumeSlider.valueBeforeMute
                    }
                }

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Volume")
                Accessible.description: qsTr("Toggle volume slider")
                Accessible.onPressAction: speakerIconMouseArea.clicked(mouse)

                WToolTip {
                    text: qsTr("Click for mute, long click for volume slider")
                    visible: speakerIconContainer.hovered
                }

                Popup {
                    id: volumePopup
                    y: speakerIconContainer.y + speakerIconContainer.height
                    x: Math.round(speakerIconContainer.x + (speakerIconContainer.width / 2) - volumePopup.width/2 )

                    parent: Overlay.overlay

                    focus: true
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                    ColumnLayout{
                        Slider {
                            id: volumeSlider

                            property real valueBeforeMute: 1

                            Layout.alignment: Qt.AlignCenter

                            height: 100
                            orientation: Qt.Vertical
                            snapMode: Slider.SnapAlways
                            wheelEnabled: true

                            from: 0
                            to: 1
                            stepSize: 0.01
                            value: radioController.volume

                            onValueChanged: {
                                setVolume(value)
                            }

                            Connections {
                                target: radioController
                                function onVolumeChanged(volume) {
                                    volumeSlider.value = volume
                                }
                            }

                            function setVolume(value) {
                                if (volumeSlider.value != radioController.volume) {
                                    if (value === 0) {
                                        radioController.setVolume(value)
                                        speakerIconContainer.icon.color = "red"
                                    } else {
                                        radioController.setVolume(value)
                                        speakerIconContainer.icon.color = undefined
                                    }
                                }
                            }
                        }

                        TextStandart {
                            id: volumeLabel
                            Layout.alignment: Qt.AlignCenter

                            font.pixelSize: Units.em(0.8)
                            text: Math.round(volumeSlider.value*100) + "%"

                            Accessible.description: qsTr("Volume set to %1").arg(text)
                        }

                        Shortcut {
                            context: Qt.ApplicationShortcut
                            autoRepeat: true
                            sequences: ["Ctrl+Up", "Volume Up"]
                            onActivated: {
                                volumeSlider.visible = true
                                volumeSlider.value = volumeSlider.value + volumeSlider.stepSize
                            }
                        }

                        Shortcut {
                            context: Qt.ApplicationShortcut
                            autoRepeat: true
                            sequences: ["Ctrl+Down", "Volume Down"]
                            onActivated: {
                                volumeSlider.visible = true
                                volumeSlider.value = volumeSlider.value - volumeSlider.stepSize
                            }
                        }

                        Shortcut {
                            context: Qt.ApplicationShortcut
                            autoRepeat: false
                            sequences: ["m", "Volume Mute"]
                            onActivated: {
                                volumeSlider.visible = true
                                volumeSlider.value = !(volumeSlider.value)
                            }
                        }
                    }
                }
            }
            ToolButton {
                icon.name: "menu"
                implicitWidth: icon.width + Units.dp(20)

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
        // For some reason the dawer will be closed before it is opened
        // Enable closing again
        onOpened: closePolicy = Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Rectangle {
            anchors.fill: parent
            color: (mainWindow.Universal.theme === Universal.Dark ) ? "dimgrey" : "white"
        }

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

                    onCurrentIndexChanged: {
                        switch(currentIndex) {
                        case 0: stationChannelView.model = stationList; break;
                        case 1: stationChannelView.model = favoritsList; break;
                        }
                        guiHelper.updateMprisStationList(stationChannelView.model.serialized,
                                                         stationChannelView.model.type,
                                                         stationListBox.currentIndex)
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
                            onTriggered:  {
                                radioController.startScan()
                            }
                        }

                        MenuItem {
                            id: stopStationScanItem
                            text: qsTr("Stop station scan")
                            font.pixelSize: TextStyle.textStandartSize
                            enabled: false
                            onTriggered:  {
                                radioController.stopScan()
                            }
                        }

                        MenuItem {
                            text: qsTr("Clear station list")
                            font.pixelSize: TextStyle.textStandartSize
                            onTriggered: stationList.clearStations()
                        }

                        MenuItem {
                            id: stationSettingsItem
                            text: qsTr("Station settings")
                            font.pixelSize: TextStyle.textStandartSize
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

            Button {
                id: startStationScanButton
                text: qsTr("Start station scan")
                visible: (stationChannelView.count || stationListBox.currentIndex != 0) ? false : true
                onClicked:  {
                    radioController.startScan()
                }
                Layout.margins: Units.dp(10)
            }

            ListView {
                id: stationChannelView
                model: stationList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                delegate: StationDelegate {
                    stationNameText: stationName.trim()
                    stationSIdValue: stationSId
                    channelNameText: channelName == "File" ? qsTr("File") : channelName
                    availableChannelNamesText: channelName == "File" ? "" : availableChannelNames
                    knownEnsembleNamesSerialized: mainWindow.knownEnsembleNamesSerialized
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
                    onSetDefaultChannel: {
                        stationList.setDefaultChannel(stationSId, newDefaultChannel)
                        radioController.play(newDefaultChannel, stationName, stationSId)
                    }
                }

                ScrollBar.vertical: ScrollBar { }

                Shortcut {
                    context: Qt.ApplicationShortcut
                    autoRepeat: false
                    sequences: ["n", "Media Next"]
                    onActivated: {
                        var channel = radioController.lastChannel[1]
                        var sidHex = radioController.lastChannel[0]
                        var index = stationChannelView.model.getIndexNext(parseInt(sidHex,16), channel)
                        stationChannelView.model.playAtIndex(index)
                        stationChannelView.currentIndex = index
                    }
                }
                Shortcut {
                    context: Qt.ApplicationShortcut
                    autoRepeat: false
                    sequences: ["p", "Media Previous"]
                    onActivated: {
                        var channel = radioController.lastChannel[1]
                        var sidHex = radioController.lastChannel[0]
                        var index = stationChannelView.model.getIndexPrevious(parseInt(sidHex,16), channel)
                        stationChannelView.model.playAtIndex(index)
                        stationChannelView.currentIndex = index
                    }
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F1", "1"]
                    onActivated: stationChannelView.model.playAtIndex(0)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F2", "2"]
                    onActivated: stationChannelView.model.playAtIndex(1)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F3", "3"]
                    onActivated: stationChannelView.model.playAtIndex(2)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F4", "4"]
                    onActivated: stationChannelView.model.playAtIndex(3)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F5", "5"]
                    onActivated: stationChannelView.model.playAtIndex(4)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F6", "6"]
                    onActivated: stationChannelView.model.playAtIndex(5)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F7", "7"]
                    onActivated: stationChannelView.model.playAtIndex(6)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F8", "8"]
                    onActivated: stationChannelView.model.playAtIndex(7)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F9", "9"]
                    onActivated: stationChannelView.model.playAtIndex(8)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F10", "0"]
                    onActivated: stationChannelView.model.playAtIndex(9)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F11", "Ctrl+1"]
                    onActivated: stationChannelView.model.playAtIndex(10)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["F12", "Ctrl+2"]
                    onActivated: stationChannelView.model.playAtIndex(11)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["Ctrl+3"]
                    onActivated: stationChannelView.model.playAtIndex(12)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["Ctrl+4"]
                    onActivated: stationChannelView.model.playAtIndex(13)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["Ctrl+5"]
                    onActivated: stationChannelView.model.playAtIndex(14)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["Ctrl+6"]
                    onActivated: stationChannelView.model.playAtIndex(15)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["Ctrl+7"]
                    onActivated: stationChannelView.model.playAtIndex(16)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["Ctrl+8"]
                    onActivated: stationChannelView.model.playAtIndex(17)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["Ctrl+9"]
                    onActivated: stationChannelView.model.playAtIndex(18)
                }
                Shortcut {
                    context: Qt.ApplicationShortcut; autoRepeat: false; sequences: ["Ctrl+0"]
                    onActivated: stationChannelView.model.playAtIndex(19)
                }
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
                    enabled: globalSettingsLoader.item.device != 5 // disable when RAW file is used
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
        id: addButton
        text: "\u002b" // Unicode character '+'
        onClicked: viewMenu.open()
        x: parent.width - width - Units.dp(10)
        y: parent.height - height - Units.dp(10)
        visible: isExpertView

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
            height: progress < 1 ? undefined : item.implicitHeight
            source:  "qrc:/QML/settingpages/ChannelSettings.qml"
            onLoaded: isStationNameInWindowTitle = stationSettingsLoader.item.addStationNameToWindowTitleState
        }
        Connections {
            target: stationSettingsLoader.item
            function onAddStationNameToWindowTitleStateChanged() {isStationNameInWindowTitle = stationSettingsLoader.item.addStationNameToWindowTitleState}
        }
    }

    WDialog {
        id: globalSettingsDialog

        content: Loader {
            id: globalSettingsLoader
            anchors.right: parent.right
            anchors.left: parent.left
            height: progress < 1 ? undefined : item.implicitHeight
            source:  "qrc:/QML/settingpages/GlobalSettings.qml"
            onLoaded : isFullScreen = globalSettingsLoader.item.enableFullScreenState
        }

        Connections {
            target: globalSettingsLoader.item
            function onEnableFullScreenStateChanged() {isFullScreen = globalSettingsLoader.item.enableFullScreenState}
            function onQQStyleThemeChanged() {updateTheme()}
        }
    }

    WDialog {
        id: expertSettingsDialog

        content: Loader {
            id: expertSettingsLoader
            anchors.right: parent.right
            anchors.left: parent.left
            height: progress < 1 ? undefined : item.implicitHeight
            source:  "qrc:/QML/settingpages/ExpertSettings.qml"
            onLoaded: isExpertView = expertSettingsLoader.item.enableExpertModeState
        }

        Connections {
            target: expertSettingsLoader.item
            function onEnableExpertModeStateChanged() {isExpertView = expertSettingsLoader.item.enableExpertModeState}
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

        function onShowErrorMessage(Text) {
            errorMessagePopup.text = Text;

            if(mainWindow.isLoaded)
                errorMessagePopup.open();
        }

        function onShowInfoMessage(Text) {
            infoMessagePopup.text = Text;
            infoMessagePopup.open();
        }

        function onScanStopped() {
            startStationScanItem.enabled = true
            stopStationScanItem.enabled = false
            startStationScanButton.enabled = true
        }

        function onScanProgress() {
            startStationScanItem.enabled = false
            stopStationScanItem.enabled = true
            startStationScanButton.enabled = false
        }

        function onNewStationNameReceived(station, sId, channel) {stationList.addStation(station, sId, channel, false)}

        function onEnsembleChanged() {
            var ensemble = radioController.ensemble.trim()
            var channel = radioController.channel
            if(ensemble != "") {
                var knownEnsembleNames = {}
                if(knownEnsembleNamesSerialized != "")
                    knownEnsembleNames = JSON.parse(knownEnsembleNamesSerialized)

                if (!(channel in knownEnsembleNames) || knownEnsembleNames[channel] !== ensemble) {
                    knownEnsembleNames[channel] = ensemble; // Put new name into dict
                    knownEnsembleNamesSerialized = JSON.stringify(knownEnsembleNames)
                }
            }
        }
    }

    Connections {
        target: guiHelper

        function onMinimizeWindow() {hide()}
        function onMaximizeWindow() {showMaximized()}
        function onRestoreWindow() {
            // On Linux (KDE?): Hide before we restore 
            // otherwise the window will occasionally not be brought to the front
            if (Qt.platform.os === "linux" && !active) // Linux Workaround to display the window
                hide()
            showNormal()
            raise() // Stay in foreground
            if (Qt.platform.os === "linux" && !active) // Linux Workaround to display the window
                requestActivate()
        }
    }

    onVisibilityChanged: function(visibility) {
        if(visibility === Window.Minimized)
            guiHelper.tryHideWindow()
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: false
        sequences: [StandardKey.Quit]
        onActivated: guiHelper.close()
    }

    function updateTheme() {
        switch(globalSettingsLoader.item.qQStyleTheme) {
            case 0: mainWindow.Universal.theme = Universal.Light; break;
            case 1: mainWindow.Universal.theme = Universal.Dark; break;
            case 2: mainWindow.Universal.theme = Universal.System; break;
        }
    }
}
