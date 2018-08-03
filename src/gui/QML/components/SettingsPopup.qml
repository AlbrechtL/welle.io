import QtQuick 2.0
import QtQuick.Controls 2.3

Popup {
    modal: true
    focus: true
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    contentWidth: contentItem.implicitWidth
    contentHeight: contentItem.implicitHeight
    clip: true
}
