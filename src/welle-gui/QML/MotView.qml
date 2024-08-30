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
import QtQuick.Dialogs
import QtCore

import "texts"
import "components"

// MOT image
ViewBaseFrame {
    labelText: qsTr("MOT Slide Show")

    property var slideList: []
    property var currentCategoryId: 0
    property var currentCategoryListPos: 0
    property var latestPictureName: ""

    FileDialog {
        id: fileDialog
        title: "Please choose a folder"
        onAccepted: {
            guiHelper.saveMotImages(fileDialog.folder)
        }
    }

    Component.onCompleted: {
        addEntry( qsTr("Save all images"),
                  function() {
                    fileDialog.open()
                })

        _initCategories()
    }

    ListModel {
        id: categoryListModel
    }

    content: Item {
        anchors.fill: parent

        RowLayout {
            anchors.fill: parent

            ColumnLayout {
                Layout.fillHeight: true
                Layout.preferredWidth: Units.dp(180)
                visible: slideList.length > 0

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignLeft

                    model: categoryListModel

                    delegate: Item {
                        width: parent == null ? undefined : parent.width
                        height: Units.dp(20)

                        Rectangle {
                            anchors.fill: parent
                            color: mouse.pressed ? "lightgrey" : "ghostwhite"
                            visible: mouse.pressed || categoryId === currentCategoryId
                        }

                        MouseArea {
                            id: mouse
                            onClicked: _categroryClicked(categoryId)
                            anchors.fill: parent

                            TextStandart {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: Units.dp(10)

                                text: categoryTitle
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.preferredWidth: Units.dp(180)

                    WButton {
                        icon.name: "back"
                        Layout.preferredHeight: Units.dp(20)
                        Layout.fillWidth: true
                        visible: slideList.length > 1 && slideList[currentCategoryListPos][2].length > 1 // visible only if we have more then one picture

                        onPressed: {
                            if(currentCategoryListPos > 0) {
                                categoryListModel.get(currentCategoryListPos).currentPictureIndex--

                                if(categoryListModel.get(currentCategoryListPos).currentPictureIndex < 0)
                                    categoryListModel.get(currentCategoryListPos).currentPictureIndex = slideList[currentCategoryListPos][2].length - 1

                                motImage.source = "image://SLS/" + slideList[currentCategoryListPos][2][categoryListModel.get(currentCategoryListPos).currentPictureIndex][1]
                            }
                        }
                    }

                    WButton {
                        icon.name: "next"
                        Layout.preferredHeight: Units.dp(20)
                        Layout.fillWidth: true
                        visible: slideList.length > 1 && slideList[currentCategoryListPos][2].length > 1 // visible only if we have more then one picture

                        onPressed: {
                            if(currentCategoryListPos > 0) {
                                categoryListModel.get(currentCategoryListPos).currentPictureIndex++

                                categoryListModel.get(currentCategoryListPos).currentPictureIndex =
                                        categoryListModel.get(currentCategoryListPos).currentPictureIndex % slideList[currentCategoryListPos][2].length

                                motImage.source = "image://SLS/" + slideList[currentCategoryListPos][2][categoryListModel.get(currentCategoryListPos).currentPictureIndex][1]
                            }
                        }
                    }
                }

                Label {
                    visible: mainWindow.isExpertView
                    text: currentCategoryListPos !== 0 && slideList[currentCategoryListPos][2].length > 1 ?
                               "Total numbers of sildes: " + slideList[currentCategoryListPos][2].length
                               : ""
                    background: Rectangle { opacity: 0.6; color: "white" }
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignRight

                RowLayout {
                        Image {
                            id: motImage

                            fillMode: Image.PreserveAspectFit
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignTop

                            Connections{
                                target: guiHelper
                                function onMotChanged(pictureName, categoryTitle, categoryId, slideId) {
                                    latestPictureName = pictureName

                                    // Display slide show only of latest tab is select
                                    if(currentCategoryId === 0) {
                                        motImage.source = "image://SLS/" + pictureName
                                    }

                                    // Handle categories if exist
                                    if(categoryTitle !== "" && categoryId !== 0) {

                                        // Check if category exists, if not create it
                                        var foundCategory = false
                                        for (var i = 0; i < slideList.length; ++i) {
                                            if(slideList[i][1] === categoryId)
                                                foundCategory = true
                                        }

                                        // Create new tab
                                        if(!foundCategory) {
                                            //bar.addItem(myTabButton.createObject(bar , {"categoryTitle": categoryTitle, "categoryId": categoryId}))
                                            categoryListModel.append({"categoryTitle": categoryTitle, "categoryId": categoryId, "currentPictureIndex": 0})
                                            slideList.push([categoryTitle, categoryId, []])
                                        }

                                        // Put picture name into category
                                        for (i=0; i < slideList.length; ++i) {
                                            // Find the correct category
                                            if(slideList[i][1] === categoryId) {
                                                // Check if picture is already in list
                                                var foundPicture = false
                                                for( var j=0; j< slideList[i][2].length; j++) {
                                                    if(slideList[i][2][j][0] === slideId) { // Found picture
                                                        // Replace picture name
                                                        slideList[i][2][j][1] = pictureName
                                                        foundPicture = true
                                                        slideListChanged()
                                                    }
                                                }

                                                // Add new picture
                                                if(!foundPicture) {
                                                    slideList[i][2].push([slideId, pictureName])
                                                    slideListChanged()
                                                }
                                            }
                                        }
                                    }
                                }

                                function onMotReseted() {
                                    slideList = [];
                                    categoryListModel.clear()
                                    _initCategories()

                                    motImage.source = "image://SLS/empty"
                                }
                            }

//                            ColumnLayout {
//                                anchors.fill: parent

//                                Item {
//                                    Layout.fillHeight: true
//                                }

//                                Label {
//                                    visible: mainWindow.isExpertView && motImage.source != ""
//                                    text: "image://sls/CategoryID/CategoryTitle/SlideID/ContentName"
//                                    background: Rectangle { opacity: 0.6; color: "white" }
//                                }

//                                Label {
//                                    visible: mainWindow.isExpertView
//                                    text: motImage.source
//                                    background: Rectangle { opacity: 0.6; color: "white" }
//                                }
//                            }
                        }
                    }
            }
        }
    }

    onSlideListChanged: {
        _updateSlide()
    }

    function _categroryClicked(categoryId) {
        console.debug("Select category: " + categoryId)
        currentCategoryId = categoryId
        _updateSlide()
    }

    function _updateSlide() {
        // Handle of latest slide is selected or no category slide show exists
        if(currentCategoryId === 0 && latestPictureName !== "") {
            motImage.source = "image://SLS/" + latestPictureName
            return
        }

        if(slideList.length < 1)
            return;

        // Find the correct category position
        for (var i=0; i < slideList.length; ++i) {
            if(slideList[i][1] === currentCategoryId) {
                currentCategoryListPos = i
                break
            }
        }

        // Get last picture from current selected categroy
        var pictureName = slideList[currentCategoryListPos][2][slideList[currentCategoryListPos][2].length -1][1]

        // Show slide
        motImage.source = "image://SLS/" + pictureName
    }

    function _initCategories() {
        categoryListModel.append({"categoryTitle": "Latest", "categoryId": 0, "currentPictureIndex": 0})
        slideList.push(["Latest", 0, []])
    }

}
