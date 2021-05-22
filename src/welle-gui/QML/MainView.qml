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

import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0
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
    }

    header: ToolBar {
        id: overlayHeader

        RowLayout {
            spacing: 5
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
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            Image {
                id: startStopIcon
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter

                height: parent.availableHeight - parent.padding
                fillMode: Image.PreserveAspectFit

                Accessible.role: Accessible.Button
                Accessible.name: (radioController.isPlaying || radioController.isChannelScan) ? qsTr("Stop") : qsTr("Play")
                Accessible.description: radioController.isPlaying ? qsTr("Stop playback") : radioController.isChannelScan ? qsTr("Stop scan") : qsTr("Start playback")
                Accessible.onPressAction: startStopIconMouseArea.clicked(mouse)

                WToolTip {
                    text: (radioController.isPlaying || radioController.isChannelScan) ? qsTr("Stop") : qsTr("Play")
                    visible: startStopIconMouseArea.containsMouse
                }

                Component.onCompleted: { startStopIcon.setStartPlayIcon() }

                MouseArea {
                    id: startStopIconMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (radioController.isPlaying || radioController.isChannelScan) {
                            startStopIcon.stop()
                        } else {
                            startStopIcon.play()
                        }
                    }
                }

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
                    onIsPlayingChanged: {
                        startStopIcon.setStartPlayIcon()
                    }
                    onIsChannelScanChanged: {
                        startStopIcon.setStartPlayIcon()
                    }
                }

                function setStartPlayIcon() {
                    if (radioController.isPlaying || radioController.isChannelScan) {
                        startStopIcon.source = "qrc:/icons/welle_io_icons/20x20/stop.png"
                    } else {
                        startStopIcon.source = "qrc:/icons/welle_io_icons/20x20/play.png"
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
            ColorOverlay {
                id: startStopIconOverlay
                anchors.fill: startStopIcon
                source: startStopIcon
                color: (mainWindow.Material.theme === Material.Dark ) ? "lightgrey" : (mainWindow.Universal.theme === Universal.Dark ) ? "lightgrey" : TextStyle.textColor
            }
            }

            ToolButton {
                id: speakerIconContainer
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                contentItem: Item {
                    // Use 2 Images to switch between speaker & speaker_mute icon (instead of toggle button).
                    // Permits use of color with org.kde.desktop style
                    Image {
                        id: speakerIcon
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter

                        height: speakerIconContainer.availableHeight - speakerIconContainer.padding
                        fillMode: Image.PreserveAspectFit

                        visible: false

                        source: "qrc:/icons/welle_io_icons/20x20@2/speaker.png"

                        WToolTip {
                            text: qsTr("Volume (%1)").arg(volumeLabel.text)
                            visible: speakerIconMouseArea.containsMouse
                        }

                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Volume")
                        Accessible.description: qsTr("Toggle volume slider")
                        Accessible.onPressAction: speakerIconMouseArea.clicked(mouse)
                    }
                    MouseArea {
                        id: speakerIconMouseArea
                        anchors.fill: speakerIcon
                        hoverEnabled: true
                        onClicked: volumePopup.open()
                        onDoubleClicked: (radioController.volume != 0) ? volumeSlider.value = 0 : volumeSlider.value = 1
                        onWheel: (wheel.angleDelta.y > 0) ? volumeSlider.value = volumeSlider.value + 0.1 :  volumeSlider.value = volumeSlider.value - 0.1
                    }
                    Image {
                        id: speakerIconMuted
                        anchors.top: speakerIcon.top
                        anchors.left: speakerIcon.left
                        width: speakerIcon.width
                        height: speakerIcon.height
                        visible: false

                        source: "qrc:/icons/welle_io_icons/20x20@2/speaker_mute.png"
                    }
                    ColorOverlay {
                        id: speakerIconMutedRed
                        visible: false
                        anchors.fill: speakerIconMuted
                        source: speakerIconMuted
                        color: "red"
                    }

                    // We don't display the "speakerIcon" item, but the "speakerIconMaskApplied"
                    // item the right part of which is +/- opacified depending on the volume
                    Item {
                        id: hidingRect
                        anchors.fill: speakerIcon
                        visible: false
                        Rectangle {
                            anchors.right: parent.right
                            color: "green" //Could be any
                            width: speakerIcon.width *0.30
                            height: speakerIcon.height
                            opacity: 1 - volumeSlider.value
                        }
                    }
                    OpacityMask {
                        id: speakerIconMaskApplied
                        anchors.fill: speakerIcon
                        source: speakerIcon
                        maskSource: hidingRect
                        invert: true
                        visible: false
                    }
                    ColorOverlay {
                        id: speakerIconMaskAppliedOverlay
                        anchors.fill: speakerIconMaskApplied
                        source: speakerIconMaskApplied
                        color: (mainWindow.Material.theme === Material.Dark ) ? "lightgrey" : (mainWindow.Universal.theme === Universal.Dark ) ? "lightgrey" : TextStyle.textColor
                    }

                    Popup {
                        id: volumePopup
                        y: speakerIconContainer.y + speakerIconContainer.height
                        x: Math.round(speakerIconContainer.x + (speakerIconContainer.width / 2) - volumePopup.width/2 )

                        parent: Overlay.overlay

                        //modal: true  //if 'true', double click on the speaker icon will not be catched
                        focus: true
                        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                        onOpened: volumeSliderTrigger.restart()
                        onClosed: volumeSliderTrigger.stop()

                        ColumnLayout{
                            Slider {
                                id: volumeSlider

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
                                    if (visible)
                                        volumeSliderTrigger.restart()
                                }

                                Connections {
                                    target: radioController
                                    onVolumeChanged: {
                                        volumeSlider.value = volume
                                    }
                                }

                                Timer {
                                    id: volumeSliderTrigger
                                    interval: 3000
                                    running: false
                                    repeat: false
                                    onTriggered: { volumePopup.close() }
                                }

                                function setVolume(value) {
                                    if (volumeSlider.value != radioController.volume) {
                                        if (value === 0) {
                                            radioController.setVolume(value)
                                            speakerIconMutedRed.visible = true
                                            speakerIconMaskAppliedOverlay.visible = false
                                        } else {
                                            radioController.setVolume(value)
                                            speakerIconMutedRed.visible = false
                                            speakerIconMaskAppliedOverlay.visible = true
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
                    stationNameText: stationName.trim()
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

                Shortcut {
                    context: Qt.ApplicationShortcut
                    autoRepeat: false
                    sequences: ["n", "Media Next"]
                    onActivated: {
                        var channel = radioController.lastChannel[1]
                        var sidHex = radioController.lastChannel[0]
                        var index = stationChannelView.model.getIndexNext(parseInt(sidHex,16), channel)
                        stationChannelView.model.playAtIndex(index)
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

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: false
        sequences: [StandardKey.Quit]
        onActivated: guiHelper.close()
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
