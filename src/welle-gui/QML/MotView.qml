import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3

import "texts"
import "components"

// MOT image
ViewBaseFrame {
    labelText: qsTr("MOT Slide Show")

    // The component is like a factory for MyTabButtons now.
    Component {
        id: myTabButton

        TabButton {
            property string categoryTitle
            property int categoryId: 0
            property var pictureList: []
            property var slideIdList: []
            property int currentPictureIndex: 0
            text: categoryTitle
        }
    }

    Component.onCompleted: {
        addEntry( qsTr("Save all images"),
                  function() {
                    guiHelper.saveMotImages()
                })
    }

    content: Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent

            TabBar {
                Layout.fillWidth: true
                id: bar
                visible: count > 1

                TabButton {
                    id: latestPicture
                    property string pictureName
                    text: qsTr("Latest")
                }

                onCurrentIndexChanged: {
                    if(currentIndex > 0)
                        motImage.source = "image://SLS/" + currentItem.pictureList[0]
                    else
                        motImage.source = "image://SLS/" + currentItem.pictureName
                }
            }

            RowLayout {
                WButton {
                    icon.name: "back"
                    visible: bar.currentIndex > 0 && bar.currentItem.pictureList.length > 1 // visible only if we have more then one picture

                    onPressed: {
                        if(bar.currentIndex > 0) {
                            bar.currentItem.currentPictureIndex--

                            if(bar.currentItem.currentPictureIndex < 0)
                                bar.currentItem.currentPictureIndex = bar.currentItem.pictureList.length - 1

                            motImage.source = "image://SLS/" + bar.currentItem.pictureList[bar.currentItem.currentPictureIndex]
                        }
                    }
                }

                Image {
                    id: motImage

                    fillMode: Image.PreserveAspectFit
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Connections{
                        target: guiHelper
                        onMotChanged:{
                            latestPicture.pictureName = pictureName

                            // Display slide show only of latest tab is select
                            if(bar.currentIndex === 0)
                                motImage.source = "image://SLS/" + pictureName

                            if(categoryTitle !== "" && categoryId !== 0) {
                                // Check if category exists, if not create it
                                var foundCategory = false
                                for (var i = 1; i < bar.contentChildren.length; ++i) {
                                    if(bar.contentChildren[i].categoryId === categoryId)
                                        foundCategory = true
                                }
                                // Create new tab
                                if(!foundCategory) {
                                    bar.addItem(myTabButton.createObject(bar , {"categoryTitle": categoryTitle, "categoryId": categoryId}))
                                }

                                // Put picture name into category
                                for (i = 1; i < bar.contentChildren.length; ++i) {
                                    // Find the correct category
                                    if(bar.contentChildren[i].categoryId === categoryId) {
                                        // Check if picture is already in list
                                        var foundPicture = false
                                        for( var j=0; j< bar.contentChildren[i].pictureList.length; j++) {
                                            if(bar.contentChildren[i].slideIdList[j] === slideId) { // Found picture
                                                bar.contentChildren[i].pictureList[j] = pictureName
                                                foundPicture = true
                                            }
                                        }
                                        if(!foundPicture) {
                                            bar.contentChildren[i].slideIdList.push(slideId)
                                            bar.contentChildren[i].pictureList.push(pictureName)
                                            bar.contentChildren[i].pictureListChanged()
                                        }
                                    }
                                }
                            }
                        }

                        onMotReseted:{
                            // Delete tabs
                            for (var i = 1; i < bar.contentChildren.length; ++i) {
                                bar.contentChildren[i].destroy()
                            }

                            motImage.source = "image://SLS/empty"
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent

                        Item {
                            Layout.fillHeight: true
                        }

                        Label {
                            visible: mainWindow.isExpertView
                            text: bar.currentIndex != 0 ?
                                       "Picture: " + (bar.currentItem.currentPictureIndex+1) + " / " + bar.currentItem.pictureList.length
                                       : " "
                            background: Rectangle { opacity: 0.6; color: "white" }
                        }

                        Label {
                            visible: mainWindow.isExpertView && motImage.source != ""
                            text: "image://sls/CategoryID/CategoryTitle/SlideID/ContentName"
                            background: Rectangle { opacity: 0.6; color: "white" }
                        }

                        Label {
                            visible: mainWindow.isExpertView
                            text: motImage.source
                            background: Rectangle { opacity: 0.6; color: "white" }
                        }
                    }
                }

                WButton {
                    icon.name: "next"
                    visible: bar.currentIndex > 0 && bar.currentItem.pictureList.length > 1 // visible only if we have more then one picture

                    onPressed: {
                        if(bar.currentIndex > 0) {
                            bar.currentItem.currentPictureIndex++
                            bar.currentItem.currentPictureIndex = bar.currentItem.currentPictureIndex % bar.currentItem.pictureList.length

                            motImage.source = "image://SLS/" + bar.currentItem.pictureList[bar.currentItem.currentPictureIndex]
                        }
                    }
                }
            }
        }
    }
}
