import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "texts"
import "components"
import "expertviews"

Item {
    height: Units.dp(400)
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.preferredWidth: Units.dp(400)

    Flow {
        id: layout
        anchors.fill: parent

        StationInformation {
        }

        Spectrum {
        }
    }
}
