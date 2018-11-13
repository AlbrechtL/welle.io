import QtQuick 2.2
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.1

// Import custom styles
import "texts"
import "components"
import "expertviews"

Item {
    implicitHeight: layout.implicitHeight
    Layout.preferredHeight: Units.dp(400)
    Layout.preferredWidth: Units.dp(400)
    Layout.fillWidth: true
    Layout.fillHeight: true

    GridLayout {
        id: layout
        anchors.top: parent.top
        flow: GridLayout.LeftToRight
        columns: (parent.width / Units.dp(400)).toFixed(0)
    }

    function __addComponent(path) {
        var component = Qt.createComponent(path);
        var object = component.createObject(layout);
    }
}
