import QtQuick 2.0
import QtQuick.Layouts 1.3

import "components"

// MOT image
ViewBaseFrame {
    labelText: qsTr("MOT Slide Show")

    Image {
        id: motImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit

        Connections{
            target: guiHelper
            onMotChanged:{
                motImage.source = "image://motslideshow/image_" + Math.random()
            }
        }
    }
}
