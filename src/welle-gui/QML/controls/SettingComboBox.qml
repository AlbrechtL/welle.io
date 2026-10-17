/*
 *    Copyright (C) 2017 - 2026
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
import QtQuick.Controls.Material
import QtQuick.Layouts

// Combo box of the settings, as wide as its longest entry. The model is a
// list of strings or a ListModel with the role "label". Labels of a
// ListModel are translated in translationContext, e.g.
//   ListElement { label: QT_TRANSLATE_NOOP("Settings", "Dark") }
// so that they follow a language change in the settings.
ComboBox {
    id: comboBox

    property string translationContext: "Settings"

    // Changes when the language changes, to translate the labels again
    property int _translation: 0

    // A ListModel, otherwise a list of strings (which is no JS Array in
    // QML, but has a length)
    readonly property bool _isListModel: !!model && typeof model.get === "function"

    function labelAt(index) {
        if (index < 0 || !model)
            return ""
        _translation // Dependency for the bindings that call this function
        return _isListModel ? qsTranslate(translationContext, model.get(index).label) : String(model[index])
    }

    function _count() {
        return !model ? 0 : _isListModel ? model.count : model.length
    }

    displayText: labelAt(currentIndex)
    implicitContentWidthPolicy: ComboBox.WidestText
    Layout.preferredWidth: Math.max(implicitWidth, _widest + leftPadding + rightPadding + indicator.width + spacing)
    popup.Material.elevation: Theme.elevation(8)

    delegate: ItemDelegate {
        required property int index
        objectName: "comboItem." + index
        width: comboBox.width
        text: comboBox.labelAt(index)
        highlighted: comboBox.highlightedIndex === index
    }

    // The settings shouldn't trigger the keyboard shortcuts of the window
    Keys.onShortcutOverride: (event) => {
        if (event.key === Qt.Key_M || event.key === Qt.Key_S || event.key === Qt.Key_P || event.key === Qt.Key_N)
            event.accepted = true
    }

    // Width of the longest translated label
    property real _widest: 0

    function _measure() {
        var width = 0
        for (var i = 0; i < _count(); i++) {
            metrics.text = labelAt(i)
            width = Math.max(width, metrics.advanceWidth)
        }
        _widest = Math.ceil(width)
    }

    TextMetrics {
        id: metrics
        font: comboBox.font
    }

    onModelChanged: _measure()
    // E.g. entries appended to a ListModel
    onCountChanged: _measure()
    Component.onCompleted: _measure()

    Connections {
        target: guiHelper
        function onTranslationFinished() {
            comboBox._translation++
            comboBox._measure()
        }
    }
}
