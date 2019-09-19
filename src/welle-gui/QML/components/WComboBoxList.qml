import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import "../texts"

ComboBox {
    id: comboBox

    font.pixelSize: TextStyle.textStandartSize
    font.family: TextStyle.textFont

    delegate: ItemDelegate {
        width: comboBox.width
        contentItem: TextStandart {
            text: listModel.get(index).label;
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        highlighted: comboBox.highlightedIndex === index
    }
}
