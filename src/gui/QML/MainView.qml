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
    visible: true

    signal stationClicked()
    property alias isExpertView: globalSettings.enableExpertModeState

    function getTitle(){
        if(listStackView.depth > 1  && listStackView.currentItem.currentItem)
            return listStackView.currentItem.currentItem.text + " " + qsTr("Settings")
        else
            return "welle.io"
    }

//    property bool isLandscape: true
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

    readonly property bool inPortrait: mainWindow.width < mainWindow.height

    onWidthChanged: {
        // Set drawer width to half of windows width
        if(moveDrawer.x > width)
            moveDrawer.x = width / 2;
    }

    visibility: globalSettings.enableFullScreenState ? Window.FullScreen : Window.Windowed

    Component.onCompleted: {
        console.debug("os: " + Qt.platform.os)
        console.debug("desktopAvailableWidth: " + Screen.desktopAvailableWidth)
        console.debug("desktopAvailableHeight: " + Screen.desktopAvailableHeight)
        console.debug("orientation: " + Screen.orientation)
        console.debug("devicePixelRatio: " + Screen.devicePixelRatio)
        console.debug("pixelDensity: " + Screen.pixelDensity)

        // Reset window settings if OS is Android (it is a little hacky)
        if(Qt.platform.os == "android") {
            mainWindow.width = getWidth()
            mainWindow.height = getHeight()
        }

        if (cppRadioController.GUIData.Status === -1) {
            console.debug("error: " + cppRadioController.ErrorMsg)
            errorMessagePopup.text = cppRadioController.ErrorMsg
            errorMessagePopup.open()
        }

        // Set drawer width to half of windows width if it is not defined
        if(moveDrawer.x === 0)
            moveDrawer.x = mainWindow.width / 2;
    }

//    Keys.onEscapePressed: stackView.pop(null);

    Settings {
        property alias width : mainWindow.width
        property alias height : mainWindow.height
        property alias leftDrawerWidth: moveDrawer.x
    }

    GlobalSettings {
        id: globalSettings
        visible: false
    }

    header: ToolBar {
        id: overlayHeader
        Material.foreground: "white"

        RowLayout {
            spacing: 20
            anchors.fill: parent

            ToolButton {
                icon.name: listStackView.depth > 1 ? "back" : "drawer"
                visible: inPortrait || listStackView.depth > 1
                onClicked: {
                    if(listStackView.depth > 1) {
                        listStackView.pop(StackView.Immediate)
                        stackView.pop(null, StackView.Immediate)
                    }
                    else {
                        if (drawer.visible)
                            drawer.close()
                        else
                            drawer.open()
                    }
                }
            }

            Label {
                id: titleLabel
                text: getTitle()
                font.pixelSize: 20
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
                        onTriggered:  {
                            listStackView.push(stettingsList, StackView.Immediate)
                            drawer.open()
                        }
                    }
                    MenuItem {
                        text: qsTr("Expert Settings")
                        visible: isExpertView
                        height: isExpertView ? implicitHeight : 0
                        onTriggered: expertView.openSettings()
                    }
                    MenuItem {
                        text: qsTr("About")
                        onTriggered: aboutDialog.open()
                    }
                    MenuItem {
                        text: qsTr("Exit")
                        onTriggered: cppGUI.close()
                    }
                }
            }
        }
    }

    Component {
        id: stettingsList

        ListView {
            id: listView
            focus: true
            currentIndex: -1

            delegate: ItemDelegate {
                width: parent.width
                text: model.title
                font.pixelSize: TextStyle.textStandartSize
                font.family: TextStyle.textFont
                highlighted: ListView.isCurrentItem
                onClicked: {
                    if(listView.currentIndex != index) {
                        listView.currentIndex = index

                        switch(index) {
                            case 0: stackView.replace(globalSettings);  break
                            default: stackView.replace(model.source);
                        }
                    }
                }
            }

            model: ListModel {
                ListElement { title: qsTr("General"); }
                ListElement { title: qsTr("Channels");  source: "qrc:/src/gui/QML/settingpages/ChannelSettings.qml" }
                ListElement { title: qsTr("RTL-SDR"); source: "qrc:/src/gui/QML/settingpages/RTLSDRSettings.qml" }
                ListElement { title: qsTr("RTL-TCP"); source: "qrc:/src/gui/QML/settingpages/RTLTCPSettings.qml" }
                ListElement { title: qsTr("SoapySDR"); source: "qrc:/src/gui/QML/settingpages/SoapySDRSettings.qml" }
            }

            Component.onCompleted: {
                // Load first settings page
                listView.currentIndex = 0
                stackView.push(globalSettings, StackView.Immediate)
            }
        }
    }

    Drawer {
        id: drawer

        y: overlayHeader.height
        width: moveDrawer.x
        height: mainWindow.height - overlayHeader.height

        modal: inPortrait
        interactive: inPortrait
        position: inPortrait ? 0 : 1
        visible: !inPortrait

        StackView {
            id: listStackView
            anchors.fill: parent

            // Implements back key navigation
            focus: true
            Keys.onReleased: if (event.key === Qt.Key_Back && listStackView.depth > 1) {
                                 listStackView.pop();
                                 event.accepted = true;
                             }

            initialItem: Item {
                width: parent.width
                height: parent.height
                TextStandart {
                    x: Units.dp(5)
                    y: Units.dp(5)
                    text: qsTr("Station list is empty")
                    visible: stationChannelView.count ? false : true
                }

                ListView {
                   id: stationChannelView
                   model: cppGUI.stationModel
                   anchors.fill: parent
                   delegate: StationDelegate {
                       stationNameText: modelData.stationName
                       channelNameText: modelData.channelName
                       showChannelName: isExpertView
                       onClicked: {
                           if(modelData.channelName !== "") {
                               mainWindow.stationClicked()
                               cppGUI.channelClick(modelData.stationName, modelData.channelName)
                           }
                       }
                   }

                    ScrollIndicator.vertical: ScrollIndicator { }
                }
            }
        }

        // Make it possible to change the lists width
        Rectangle {
            id: moveDrawer
            width: Units.dp(5)
            height: parent.height
//          color: "red"
//          border.color: "red"
            opacity: 0

            MouseArea {
                anchors.fill: parent
                cursorShape : Qt.SizeHorCursor
                drag.target: moveDrawer
                drag.axis: Drag.XAxis
                drag.minimumX: 0
                drag.maximumX: mainWindow.width
            }
        }
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        anchors.leftMargin: !inPortrait ? drawer.width: undefined

        function setContentHeight() {
            contentHeight = stackView.currentItem.implicitHeight > parent.height ? stackView.currentItem.implicitHeight : parent.height
        }

        onHeightChanged: setContentHeight()
        onWidthChanged: setContentHeight()
        Component.onCompleted: setContentHeight()

        ScrollBar.vertical: ScrollBar { }

        StackView {
            id: stackView
            anchors.fill: parent
            anchors.margins: Units.dp(10)

            // Implements back key navigation
            focus: true

            initialItem: Item {
                id: firstItem
                // Necessary for Flickable
                implicitHeight: isExpertView ?
                                    inPortrait ?
                                        expertView.implicitHeight + columnLayout.implicitHeight :
                                        expertView.implicitHeight
                                    : undefined

                GridLayout {
                    id: gridLayout
                    anchors.fill: parent
                    flow: inPortrait ? GridLayout.TopToBottom : GridLayout.LeftToRight
                    columnSpacing: Units.dp(10)
                    rowSpacing: Units.dp(10)

                    ColumnLayout {
                        id: columnLayout
                        Layout.alignment: Qt.AlignTop

                        // Radio information
                        RadioView {
                            id: radioView
                            Layout.fillWidth: (!isExpertView || inPortrait) ? true : false
                        }

                        // MOT image
                        Rectangle {
                           id: motImageRec
                           Layout.preferredWidth: Units.dp(320)
                           Layout.preferredHeight: width * 0.75
                           Layout.maximumWidth: Layout.preferredWidth * 2
                           Layout.fillWidth: (!isExpertView || inPortrait) ? true : false

                           Image {
                               id: motImage
                               anchors.fill: parent
                               fillMode: Image.PreserveAspectFit

                               Connections{
                                   target: cppGUI
                                   onMotChanged:{
                                       motImage.source = "image://motslideshow/image_" + Math.random()

                                   }
                               }
                           }
                        }
                    }

                    ExpertView{
                        id: expertView
                        visible: isExpertView
                    }
                }
            }
        }
    }

    Popup {
        id: aboutDialog
        modal: true
        focus: true
        x: (mainWindow.width - width) / 2
        y: 0
        width: Math.min(mainWindow.width, mainWindow.height) / 3 * 2
        contentHeight: mainWindow.height - (overlayHeader.height * 2)
        clip: true

        contentItem: InfoPage{
            id: infoPage
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
        target: cppRadioController

        onShowErrorMessage:{
            errorMessagePopup.text = Text;
            errorMessagePopup.open();
        }

        onShowInfoMessage:{
            infoMessagePopup.text = Text;
            infoMessagePopup.open();
        }
    }

    Connections {
        target: cppGUI

        onMinimizeWindow: hide()
        onMaximizeWindow: showMaximized()
        onRestoreWindow: {
            showNormal()
            raise() // Stay in foreground
        }
    }

    onVisibilityChanged: {
        if(visibility === Window.Minimized)
            cppGUI.tryHideWindow()
    }
}
