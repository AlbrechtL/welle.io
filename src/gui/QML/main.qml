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

import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "style"

ApplicationWindow {
    id: mainWindow
    visible: true
    property bool isLandscape: true
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

    visibility: settingsPage.enableFullScreenState ? "FullScreen" : "Windowed"

    Component.onCompleted: {
        console.debug("os: " + Qt.platform.os)
        console.debug("desktopAvailableWidth: " + Screen.desktopAvailableWidth)
        console.debug("desktopAvailableHeight: " + Screen.desktopAvailableHeight)
        console.debug("orientation: " + Screen.orientation)
        console.debug("devicePixelRatio: " + Screen.devicePixelRatio)
        console.debug("pixelDensity: " + Screen.pixelDensity)

        if (cppRadioController.GUIData.Status === -1) {
            console.debug("error: " + cppRadioController.ErrorMsg)
            errorMessagePopup.text = cppRadioController.ErrorMsg
            errorMessagePopup.open()
        }
    }

    property int stackViewDepth
    signal stackViewPush(Item item)
    signal stackViewPop()
    signal stackViewComplete()
    signal stationClicked()
    property alias isExpertView: settingsPage.enableExpertModeState

    Settings {
        property alias width : mainWindow.width
        property alias height : mainWindow.height
    }

    onIsExpertViewChanged: {
        if(stackViewDepth > 1)
        {
            if(isExpertView == true)
                infoMessagePopup.text = qsTr("Expert mode is enabled")
            else
                infoMessagePopup.text = qsTr("Expert mode is disabled")
            infoMessagePopup.open()
        }
    }

    SettingsPage{
        id:settingsPage
    }

    InfoPage{
        id: infoPage
    }

    Rectangle {
        x: 0
        color: "#212126"
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
        anchors.fill: parent
    }

    toolBar: BorderImage {
        id: toolBar_
        border.bottom: Units.dp(10)
        source: "images/toolbar.png"
        width: parent.width
        height: Units.dp(40)

        Rectangle {
            id: backButton
            width: Units.dp(60)
            anchors.left: parent.left
            anchors.leftMargin: Units.dp(20)
            anchors.verticalCenter: parent.verticalCenter
            antialiasing: true
            radius: Units.dp(4)
            color: backmouse.pressed ? "#222" : "transparent"
            property bool isSettings: false
            Behavior on opacity { NumberAnimation{} }
            Image {
                anchors.verticalCenter: parent.verticalCenter
                source: parent.isSettings ? "images/navigation_previous_item.png" : "images/icon-settings.png"
                height: parent.isSettings ? Units.dp(20) : Units.dp(23)
                fillMode: Image.PreserveAspectFit
            }
            MouseArea {
                id: backmouse
                scale: 1
                anchors.fill: parent
                anchors.margins: Units.dp(-20)
                onClicked: {
                    if(mainWindow.isLandscape && settingsPage.is3D){
                        backButton.isSettings = !backButton.isSettings
                    } else {
                        if(stackViewDepth > 1){
                            stackViewPop()
                            if(stackViewDepth === 1)
                                backButton.isSettings = false
                        } else {
                            stackViewPush(settingsPage)
                            backButton.isSettings = true
                        }
                    }
                }
            }
        }

        TextTitle {
            x: backButton.x + backButton.width + Units.dp(20)
            anchors.verticalCenter: parent.verticalCenter
            text: "welle.io"
        }

        TextStandart {
            x: mainWindow.width - width - Units.dp(5) - infoButton.width
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: Units.dp(5)
            text: cppRadioController.DateTime
            id: dateTimeDisplay
        }

        Rectangle {
            id: infoButton
            width: backButton.isSettings ? Units.dp(40) : 0
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            antialiasing: true
            radius: Units.dp(4)
            color: backmouse.pressed ? "#222" : "transparent"
            property bool isInfoPage: false
            Image {
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                source: backButton.isSettings ? "images/icon-info.png" : ""
                anchors.rightMargin: Units.dp(20)
                height: Units.dp(23)
                fillMode: Image.PreserveAspectFit
            }
            MouseArea {
                id: infomouse
                scale: 1
                anchors.fill: parent
                anchors.margins: Units.dp(-20)
                enabled: backButton.isSettings ? true : false
                onClicked: {
                    if(mainWindow.isLandscape && settingsPage.is3D){
                        infoButton.isInfoPage = !infoButton.isInfoPage
                    } else {
                        if(stackViewDepth > 2){
                            stackViewPop()
                            infoButton.isInfoPage = false
                        } else {
                            stackViewPush(infoPage)
                            infoButton.isInfoPage = true
                        }
                    }
                }
            }
        }
    }

    Loader {
        id:mainViewLoader
        anchors.fill: parent
        Layout.margins: Units.dp(10);
        sourceComponent: {
            if(mainWindow.width > mainWindow.height){
                mainWindow.isLandscape = true;
                if(settingsPage.is3D){
                    if(isExpertView)
                        return landscapeViewExpert3D
                    else
                        return landscapeView3D
                } else {
                    if(isExpertView)
                        return landscapeViewExpert
                    else
                        return landscapeView
                }
            } else {
                mainWindow.isLandscape = false;
                if(isExpertView)
                    return portraitViewExpert
                else
                    return portraitView
            }
        }
    }

    Component {
        id:channelBrowser
        ChannelBrowser{
            Connections {
                target: backmouse
                onClicked: {
                    if(infoPageVisible){
                        infoPageVisible = false
                        backButton.isSettings = true
                    } else {
                        settingsVisible = backButton.isSettings
                    }
                }
            }
            Connections {
                target: infomouse
                onClicked: {
                    infoPageVisible = infoButton.isInfoPage
                }
            }
            Component.onCompleted: {
                settingsVisible = backButton.isSettings
                enableFullScreenState = settingsPage.enableFullScreenState
                enableExpertModeState = settingsPage.enableExpertModeState
                enableAGCState = settingsPage.enableAGCState
                manualGainState = settingsPage.manualGainState
                is3D = settingsPage.is3D
            }
            onEnableAGCStateChanged: {
                settingsPage.enableAGCState = enableAGCState
            }
            onEnableExpertModeStateChanged: {
                settingsPage.enableExpertModeState = enableExpertModeState

            }
            onEnableFullScreenStateChanged: {
                settingsPage.enableFullScreenState = enableFullScreenState
            }
            onManualGainStateChanged: {
                settingsPage.manualGainState = manualGainState
            }
            onIs3DChanged: {
                settingsPage.is3D = is3D
            }
        }
    }
    Component {
        id: landscapeView3D

        Loader {
            id: stationView
            Layout.minimumWidth: Units.dp(350)
            Layout.margins: Units.dp(10)
            sourceComponent: channelBrowser
        }
    }
    Component {
        id: landscapeViewExpert3D

        SplitView {
            id: splitView
            anchors.fill: parent
            orientation: Qt.Horizontal

            Loader {
                id: stationView
                Layout.minimumWidth: Units.dp(350)
                Layout.margins: Units.dp(10)
                sourceComponent: channelBrowser
            }
            Loader {
                id: expertViewLoader
                Layout.margins: Units.dp(10)
                Layout.fillWidth: true
                sourceComponent: expertView
            }

            Settings {
                property alias expertStationViewWidth: stationView.width
                property alias expertViewWidth: expertViewLoader.width
            }
        }
    }

    Component {
        id: landscapeView

        SplitView {
            id: splitView
            anchors.fill: parent
            orientation: Qt.Horizontal

            Loader {
                id: stationView
                Layout.minimumWidth: Units.dp(350)
                Layout.margins: Units.dp(10)
                sourceComponent: stackViewMain
            }
            Loader {
                id: radioInformationViewLoader
                Layout.preferredWidth: Units.dp(400)
                Layout.margins: Units.dp(10)
                sourceComponent: radioInformationView
            }

            Settings {
                property alias stationViewWidth: stationView.width
            }
        }
    }

    Component {
        id: landscapeViewExpert

        SplitView {
            id: splitView
            anchors.fill: parent
            orientation: Qt.Horizontal

            Loader {
                id: stationView
                Layout.minimumWidth: Units.dp(350)
                Layout.margins: Units.dp(10)
                sourceComponent: stackViewMain
            }
            Loader {
                id: radioInformationViewLoader
                Layout.preferredWidth: Units.dp(400)
                Layout.margins: Units.dp(10)
                sourceComponent: radioInformationView
            }
            Loader {
                id: expertViewLoader
                Layout.margins: Units.dp(10)
                Layout.fillWidth: true
                sourceComponent: expertView
            }

            Settings {
                property alias expertStationViewWidth: stationView.width
                property alias expertViewWidth: expertViewLoader.width
                property alias radioInformationViewWidth: radioInformationViewLoader.width
            }
        }
    }

    Component {
        id: portraitView

        Item {
            SwipeView {
                id: view
                anchors.fill: parent
                anchors.margins: Units.dp(10)
                spacing: Units.dp(10)

                Loader {
                    sourceComponent: stackViewMain
                }
                Loader {
                    sourceComponent: radioInformationView
                }

                Connections {
                    target: mainWindow
                    onStationClicked: view.currentIndex = 1
                }
                Connections {
                    target: backmouse
                    onClicked: {
                        if(view.currentIndex > 0)
                        {
                            stackViewComplete()
                            view.currentIndex = 0
                        }
                    }

                }
                Connections {
                    target: infomouse
                    onClicked: {
                        if(view.currentIndex > 0)
                        {
                            stackViewComplete()
                            view.currentIndex = 0
                        }
                    }
                }
            }

            TouchPageIndicator {
                id: indicator

                count: view.count
                currentIndex: view.currentIndex
                visible: stackViewDepth == 1 ? true : false
            }
        }
    }

    Component {
        id: portraitViewExpert

        Item {
            SwipeView {
                id: view
                anchors.fill: parent
                anchors.margins: Units.dp(10)
                spacing: Units.dp(10)

                Loader {
                    sourceComponent: stackViewMain
                }
                Loader {
                    sourceComponent: radioInformationView
                }
                Loader {
                    sourceComponent: expertView
                }

                Connections {
                    target: mainWindow
                    onStationClicked: view.currentIndex = 1
                }
                Connections {
                    target: backmouse
                    onClicked: {
                        if(view.currentIndex > 0)
                        {
                            stackViewComplete()
                            view.currentIndex = 0
                        }
                    }

                }
                Connections {
                    target: infomouse
                    onClicked: {
                        if(view.currentIndex > 0)
                        {
                            stackViewComplete()
                            view.currentIndex = 0
                        }
                    }
                }
            }

            TouchPageIndicator {
                id: indicator

                count: view.count
                currentIndex: view.currentIndex
                visible: stackViewDepth == 1 ? true : false
            }
        }
    }

    Component {
        id: stackViewMain

        StackView {
            id: stackView
            clip: true
            Layout.alignment: Qt.AlignBottom
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Implements back key navigation
            focus: true
            Keys.onReleased: if (event.key === Qt.Key_Back && stackView.depth > 1) {
                                 stackView.pop();
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

                    ScrollBar.vertical: ScrollBar { }
                }
            }

            onDepthChanged: mainWindow.stackViewDepth = depth

            Connections {
                target: mainWindow
                onStackViewPush: push(item)
                onStackViewPop: pop()
                onStackViewComplete: completeTransition()
            }
        }
    }

    // radioInformationView
    Component {
        id: radioInformationView

        SplitView {
            orientation: Qt.Vertical
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: Units.dp(320)
            Layout.minimumHeight: Units.dp(100)

            // Radio
            RadioView {}

            // MOT image
            Rectangle {
                id: motImageRec
                color: "#212126"
                Image {
                    id: motImage
                    anchors.topMargin: Units.dp(5)
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
    }

    // expertView
    Component {
        id: expertView

        ExpertView{
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: Units.dp(400)
            width: Units.dp(400)
        }
    }

    MessagePopup {
        id: errorMessagePopup
        x: mainWindow.width/2 - width/2
        y: mainWindow.height  - toolBar_.height - height
        revealedY: mainWindow.height - toolBar.height - height
        hiddenY: mainWindow.height
        color: "#8b0000"
    }

    MessagePopup {
        id: infoMessagePopup
        x: mainWindow.width/2 - width/2
        y: mainWindow.height  - toolBar_.height - height
        revealedY: mainWindow.height - toolBar.height - height
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
}
