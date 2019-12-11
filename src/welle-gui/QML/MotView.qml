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
    //            anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                Layout.fillWidth: true


                Connections{
                    target: guiHelper
                    onMotChanged:{
                        motImage.source = "image://motslideshow/image_" + Math.random()
                    }
                }

                Connections{
                    target: radioController
                    onCategoryTitleChanged: {
                        // WIP
                        bar.addItem(myTabButton.createObject(bar , {"text": title}))
                    }
                }
            }
        }
    }
}
