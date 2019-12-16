import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.13

import "components"

// MOT image
ViewBaseFrame {
    labelText: qsTr("MOT Slide Show")

    // The component is like a factory for MyTabButtons now.
    // Use myTabButton.createObject(parent, jsobject-with-property-assignments) to create instances.
    Component {
        id: myTabButton
        TabButton {
        }
    }

    content: Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            TabBar {
                Layout.fillWidth: true
                id: bar
            }

            Image {
                id: motImage
                fillMode: Image.PreserveAspectFit
                Layout.fillWidth: true


                Connections{
                    target: guiHelper
                    onMotChanged:{
                        //motImage.source = "image://motslideshow/image_" + Math.random()
                        motImage.source = "image://motslideshow/" + content_name
                    }

                    onCategoryTitleChanged: {
                        var foundCategory = false
                        for (var i = 0; i < bar.contentChildren.length; ++i) {
                            if(bar.contentChildren[i].text === title)
                                foundCategory = true;
                        }
                        if(!foundCategory)
                            bar.addItem(myTabButton.createObject(bar , {"text": title}))
                    }
                }
            }
        }
    }
}
