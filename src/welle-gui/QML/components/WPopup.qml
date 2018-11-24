import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1

Popup {
    id: popup
    default property alias content: placeholder.data

    modal: true
    focus: true
    clip: true

    padding: Units.dp(10)

    // Place in the middle of window
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    width: parent.width < Units.dp(400) ? parent.width : Units.dp(400)
    height: parent.height < placeholder.height + padding ? parent.height : placeholder.height + padding

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: placeholder.height

        ColumnLayout {
            id: placeholder
            width: popup.availableWidth
            height: childrenRect.height
        }

        ScrollBar.vertical: ScrollBar { }
    }
}
