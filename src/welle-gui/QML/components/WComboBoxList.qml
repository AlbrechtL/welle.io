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
            text: typeof trContext !== "undefined" ? qsTranslate(trContext, label) : label
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        highlighted: comboBox.highlightedIndex === index
    }

    TextMetrics {
        id: textMetrics
    }

    onCurrentIndexChanged: {
        // Update the translation of selected item label, otherwise
        // it is not updated (in the case we previously changed the language)
        updateTrLabel()
    }

    onModelChanged: {
        computeComboBoxWidth()
    }

    function computeComboBoxWidth() {
        textMetrics.font = comboBox.font
        for(var i = 0; i < model.count; i++){
            var label;
            if (typeof model.get(i).trContext !== "undefined") {
                label = qsTranslate(model.get(i).trContext, model.get(i).label);
            } else {
                label = model.get(i).label;
            }
            textMetrics.text = label
            modelWidth = Math.max(textMetrics.width, modelWidth)
        }
        //console.debug("ComboBox width: " + modelWidth)
    }

    function updateTrLabel() {
        if (model) {
            var item = model.get(currentIndex)
            if (item.trLabel && item.trContext) {
                var trLbl = qsTranslate(item.trContext, item.label);
                //console.debug("[Translate] Index: " + currentIndex + " - Value:" + trLbl)
                model.setProperty(currentIndex, "trLabel", trLbl);
            }
        }
    }

    Connections {
        target: guiHelper
        onTranslationFinished: {
            modelWidth = 0;
            computeComboBoxWidth()

            // Update the translation of selected item label, otherwise
            // it stays in the previous language
            updateTrLabel()
        }
    }
}
