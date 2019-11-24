import QtQuick 2.7
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import "../texts"

ComboBox {
    id: comboBox

    property bool sizeToContents
    property int modelWidth
    width: (sizeToContents) ? modelWidth + 2*leftPadding + 2*rightPadding : implicitWidth
    Layout.preferredWidth: width

    font.pixelSize: TextStyle.textStandartSize
    font.family: TextStyle.textFont

    delegate: ItemDelegate {
        width: comboBox.width
        contentItem: TextStandart {
            text: modelData
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        highlighted: comboBox.highlightedIndex === index
    }

    TextMetrics {
        id: textMetrics
    }

    onModelChanged: {
        textMetrics.font = comboBox.font
        for(var i = 0; i < model.length; i++){
            textMetrics.text = model[i]
            modelWidth = Math.max(textMetrics.width, modelWidth)
        }
    }
}
