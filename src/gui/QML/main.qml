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
import Qt.labs.settings 1.0

// Import custom styles
import "style"

ApplicationWindow {
    id: mainWindow
    visible: true
    width: Units.dp(700)
    height: Units.dp(500)
    visibility: settingsPage.enableFullScreenState ? "FullScreen" : "Windowed"

    Settings {
        property alias width : mainWindow.width
        property alias height : mainWindow.height
        property alias radioInformationViewWidth: radioInformationView.width
        property alias expertViewWidth: expertView.width
    }

    SettingsPage{
        id:settingsPage
    }

    InfoPage{
        id: infoPage
        anchors.topMargin: Units.dp(10)
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
            Behavior on opacity { NumberAnimation{} }
            Image {
                anchors.verticalCenter: parent.verticalCenter
                source: stackView.depth > 1 ? "images/navigation_previous_item.png" : "images/icon-settings.png"
                height: stackView.depth > 1 ? Units.dp(20) : Units.dp(23)
                fillMode: Image.PreserveAspectFit
            }
            MouseArea {
                id: backmouse
                scale: 1
                anchors.fill: parent
                anchors.margins: Units.dp(-20)
                onClicked: {
                    if(stackView.depth > 1)
                        stackView.pop();
                    else
                        stackView.push(settingsPage);
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
            text: "01.01.2016 00:00"
            id: dateTimeDisplay
        }

        Rectangle {
            id: infoButton
            width: stackView.depth > 1 ? Units.dp(40) : 0
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            antialiasing: true
            radius: Units.dp(4)
            color: backmouse.pressed ? "#222" : "transparent"
            Image {
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                source: stackView.depth > 1 ? "images/icon-info.png" : ""
                anchors.rightMargin: Units.dp(20)
                height: Units.dp(23)
                fillMode: Image.PreserveAspectFit
            }
            MouseArea {
                id: exitmouse
                scale: 1
                anchors.fill: parent
                anchors.margins: Units.dp(-20)
                onClicked:
                    if(stackView.depth > 2)
                      stackView.pop();
                    else
                      stackView.push(infoPage);
            }
        }
    }

    GridLayout {
        id: gridLayout
        anchors.fill: parent
        anchors.margins: Units.dp(10)
        rowSpacing: Units.dp(20)
        columnSpacing: Units.dp(20)
        flow:  width > height ? GridLayout.LeftToRight : GridLayout.TopToBottom

        StackView {
            id: stackView
            clip: true
            Layout.alignment: Qt.AlignBottom
            Layout.minimumWidth: Units.dp(350)
            Layout.minimumHeight: Units.dp(150)
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
                ListView {
                    property alias showChannelState: settingsPage.showChannelState
                    anchors.rightMargin: 0
                    anchors.bottomMargin: 0
                    anchors.leftMargin: 0
                    anchors.topMargin: 0
                    model: cppGUI.stationModel
                    anchors.fill: parent
                    delegate: StationDelegate {
                        stationNameText: modelData.stationName
                        channelNameText: modelData.channelName
                        onClicked: cppGUI.channelClick(modelData.stationName, modelData.channelName)
                    }
                }
            }
        }

        Rectangle {
            color: "grey"
            width: Units.dp(1)
            Layout.fillHeight: true
            visible: gridLayout.flow == GridLayout.LeftToRight ? true : false
        }

        Rectangle {
            color: "grey"
            height: Units.dp(1)
            Layout.fillWidth: true
            visible: gridLayout.flow != GridLayout.LeftToRight ? true : false
        }

        SplitView {
            id: radioInformationView
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
                    width: parent.width
                    height: parent.width * (sourceSize.height/sourceSize.width) // Scale MOT image with the correct aspect
                }
            }
        }

        Rectangle {
            color: "grey"
            width: Units.dp(1)
            Layout.fillHeight: true
            visible: (gridLayout.flow == GridLayout.LeftToRight) && (settingsPage.enableExpertModeState) ? true : false
        }

        Rectangle {
            color: "grey"
            height: Units.dp(1)
            Layout.fillWidth: true
            visible: (gridLayout.flow != GridLayout.LeftToRight) && (settingsPage.enableExpertModeState) ? true : false
        }

        ExpertView{
            id: expertView
            Layout.fillWidth: true
            Layout.fillHeight: true
            width: Units.dp(400)
            visible: settingsPage.enableExpertModeState ? true : false
        }
    }

    ErrorMessagePopup {
      id: errorMessagePopup
    }

    Connections{
        target: cppGUI
        onMotChanged:{
            motImage.source = "image://motslideshow/image_" + Math.random()
        }

        onShowErrorMessage:{
            errorMessagePopup.text = Text;
            errorMessagePopup.open();
        }

        onSetGUIData:{
            dateTimeDisplay.text = GUIData.DateTime
        }
    }
}
