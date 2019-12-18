import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

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
            text: categoryTitle
        }
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
                        motImage.source = "image://motslideshow/" + currentItem.pictureList[0]
                    else
                        motImage.source = "image://motslideshow/" + currentItem.pictureName
                }
            }

            RowLayout {
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
                                motImage.source = "image://motslideshow/" + pictureName

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

                            motImage.source = "image://motslideshow/empty"
                        }
                    }
                }

                ColumnLayout {
                    id: buttonBox
                    Layout.fillHeight: true
                    visible: pictureButtons.count > 1
                    Repeater {
                        id: pictureButtons
                        model: bar.currentIndex === 0 ? 0: bar.currentItem.pictureList.length
                        WButton {
                            property int idx: index
                            text: idx

                            onPressed: {
                                motImage.source = "image://motslideshow/" + bar.currentItem.pictureList[idx]
                            }
                        }
                    }
                }
            }
        }
    }
}
