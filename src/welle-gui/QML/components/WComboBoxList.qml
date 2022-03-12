/*
 *    Copyright (C) 2017 - 2021
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../texts"

ComboBox {
    id: comboBox

    property bool sizeToContents
    property int modelWidth
    width: (sizeToContents) ? modelWidth + 2*leftPadding + 2*rightPadding : implicitWidth
    Layout.preferredWidth: width

    font.pixelSize: TextStyle.textStandartSize

    delegate: ItemDelegate {
        width: comboBox.width
        contentItem: TextStandart {
            text: typeof trContext !== "undefined" ? qsTranslate(trContext, label) : label
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        highlighted: comboBox.highlightedIndex === index
    }

    Keys.onShortcutOverride: {
        if (event.key == Qt.Key_M || event.key == Qt.Key_S || event.key == Qt.Key_P || event.key == Qt.Key_N)
            event.accepted = true
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
        function onTranslationFinished() {
            modelWidth = 0;
            computeComboBoxWidth()

            // Update the translation of selected item label, otherwise
            // it stays in the previous language
            updateTrLabel()
        }
    }
}
