/*
 *    This file is part of the welle.io.
 *    This file was generated with an AI assistant (Claude Code).
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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtCore

import "../controls"
import "../models"

// Expert views as cards in a responsive grid: one column on narrow
// windows, more on wide ones. Cards can be added, moved, maximized and
// removed. The order is saved as the names of the views.
Item {
    id: root
    objectName: "dashboard"

    property bool isExpert: false
    property MotSlideModel slides

    // Names of the shown views, in order
    readonly property var views: _views
    property string maximizedView: ""
    readonly property int columns: maximizedView !== "" ? 1 : Math.max(1, Math.floor((width - Theme.spacingL) / 420))

    // A card shows a message, e.g. a warning (kind "info" or "error")
    signal message(string text, string kind)
    // A card opens a section of the settings (e.g. "receiver")
    signal openSettings(string section)

    readonly property var viewTypes: [
        { name: "serviceOverview", title: qsTr("Service Overview"), file: "ServiceOverviewCard.qml" },
        { name: "serviceDetails", title: qsTr("Service Details"), file: "ServiceDetailsCard.qml" },
        { name: "slideShow", title: qsTr("MOT Slide Show"), file: "SlideShowCard.qml" },
        { name: "spectrum", title: qsTr("Spectrum"), file: "SpectrumCard.qml", properties: { kind: "spectrum" } },
        { name: "gain", title: qsTr("Receiver gain"), file: "GainCard.qml" },
        { name: "impulseResponse", title: qsTr("Impulse Response"), file: "SpectrumCard.qml", properties: { kind: "impulseResponse" } },
        { name: "constellation", title: qsTr("Constellation Diagram"), file: "ConstellationCard.qml" },
        { name: "nullSymbol", title: qsTr("Null Symbol"), file: "SpectrumCard.qml", properties: { kind: "nullSymbol" } },
        { name: "console", title: qsTr("Console Output"), file: "ConsoleCard.qml" },
        { name: "rawRecorder", title: qsTr("RAW Recorder"), file: "RawRecorderCard.qml" }
    ]
    // The views without expert mode
    readonly property var defaultViews: ["serviceOverview", "slideShow"]

    // Files of welle.io 2.7 and before, in the saved grid
    readonly property var legacyFiles: ({
        "RadioView.qml": "serviceOverview", "ServiceDetails.qml": "serviceDetails", "MotView.qml": "slideShow",
        "SpectrumGraph.qml": "spectrum", "ImpulseResponseGraph.qml": "impulseResponse",
        "ConstellationGraph.qml": "constellation", "NullSymbolGraph.qml": "nullSymbol",
        "TextOutputView.qml": "console", "RawRecorder.qml": "rawRecorder"
    })

    property var _views: []
    property string _serialized: ""
    property string _legacySerialized: ""

    Settings {
        property alias dashboardViews: root._serialized
        // Grid of welle.io 2.7 and before: [[paths], [rows], [columns]]
        property alias viewComponents: root._legacySerialized
    }

    Component.onCompleted: _load()

    onIsExpertChanged: {
        if (!isExpert) {
            // Only the views of the normal mode stay
            maximizedView = ""
            _setViews(_views.filter(name => defaultViews.includes(name)))
        }
    }

    function viewType(name) {
        return viewTypes.find(type => type.name === name)
    }

    function hasView(name) {
        return _views.includes(name)
    }

    function addView(name) {
        if (hasView(name) || !viewType(name))
            return
        maximizedView = ""
        _setViews(_views.concat([name]))
    }

    function removeView(name) {
        if (maximizedView === name)
            maximizedView = ""
        _setViews(_views.filter(view => view !== name))
    }

    // Moves a view earlier (direction -1) or later (1)
    function moveView(name, direction) {
        var from = _views.indexOf(name)
        var to = from + direction
        if (from < 0 || to < 0 || to >= _views.length)
            return
        var views = _views.slice()
        views.splice(from, 1)
        views.splice(to, 0, name)
        _setViews(views)
    }

    function toggleMaximize(name) {
        maximizedView = (maximizedView === name) ? "" : name
    }

    function _setViews(views) {
        // Keep the cards that stay: move them in the model instead of
        // creating them again
        for (var i = viewModel.count - 1; i >= 0; i--) {
            if (!views.includes(viewModel.get(i).name))
                viewModel.remove(i)
        }
        for (i = 0; i < views.length; i++) {
            var found = -1
            for (var j = 0; j < viewModel.count; j++) {
                if (viewModel.get(j).name === views[i])
                    found = j
            }
            if (found < 0)
                viewModel.insert(i, { name: views[i] })
            else if (found !== i)
                viewModel.move(found, i, 1)
        }
        _views = views
        _serialized = JSON.stringify(views)
    }

    // A saved view: its name, or the file path of welle.io 2.7 and before
    function _nameOf(view) {
        if (viewType(view))
            return view
        var file = String(view).split("/").pop()
        return legacyFiles[file] || ""
    }

    function _load() {
        var views = []
        try {
            if (_serialized !== "") {
                views = JSON.parse(_serialized)
            } else if (_legacySerialized !== "") {
                var legacy = JSON.parse(_legacySerialized)
                if (Array.isArray(legacy) && Array.isArray(legacy[0])) {
                    // Order of the old grid: column by column, top to bottom
                    var cells = legacy[0].map((path, i) => ({ path: path, row: legacy[1][i], column: legacy[2][i] }))
                    cells.sort((a, b) => a.column - b.column || a.row - b.row)
                    views = cells.map(cell => cell.path)
                } else if (Array.isArray(legacy)) {
                    views = legacy
                }
            }
        } catch (error) {
            console.warn("Cannot read the saved expert views: " + error)
        }

        // Not filtered by isExpert: the expert mode setting may not be
        // applied yet, that would lose the saved views
        views = views.map(_nameOf).filter((name, i, all) => name !== "" && all.indexOf(name) === i)
        if (views.length === 0)
            views = defaultViews
        _setViews(views)
    }

    ListModel {
        id: viewModel
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        contentHeight: grid.implicitHeight + 2 * Theme.spacingL + 72 // Room for the add button
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollBar.vertical: ScrollBar {}

        GridLayout {
            id: grid
            x: Theme.spacingL
            y: Theme.spacingL
            width: flickable.width - 2 * Theme.spacingL
            columns: root.columns
            columnSpacing: Theme.spacingL
            rowSpacing: Theme.spacingL

            Repeater {
                model: viewModel

                Loader {
                    id: slot

                    required property string name
                    required property int index

                    readonly property bool isMaximized: root.maximizedView === name

                    visible: root.maximizedView === "" || isMaximized
                    Layout.row: isMaximized ? 0 : Math.floor(index / root.columns)
                    Layout.column: isMaximized ? 0 : index % root.columns
                    Layout.fillWidth: true
                    Layout.fillHeight: true // Cards of a row have the same height
                    Layout.preferredWidth: 1 // Same width for all columns
                    Layout.preferredHeight: isMaximized ? flickable.height - 2 * Theme.spacingL
                                                        : (item ? item.cardHeight : 320)

                    Component.onCompleted: {
                        var type = root.viewType(name)
                        var properties = Object.assign({ objectName: "card." + name, viewName: name, dashboard: root },
                                                       type.properties || {})
                        setSource(type.file, properties)
                    }

                    onLoaded: {
                        item.index = Qt.binding(() => slot.index)
                        item.count = Qt.binding(() => viewModel.count)
                    }
                }
            }
        }
    }

    RoundButton {
        id: addButton
        objectName: "addViewButton"
        x: parent.width - width - Theme.spacingL
        y: parent.height - height - Theme.spacingL
        visible: root.isExpert
        icon.name: "add"
        icon.width: 24
        icon.height: 24
        width: 56
        height: 56
        highlighted: true
        Material.elevation: Theme.elevation(6)
        onClicked: addMenu.open()

        Accessible.name: qsTr("Add")
        Accessible.description: qsTr("Add a view")

        AutoSizeMenu {
            id: addMenu
            objectName: "addViewMenu"
            y: -height - Theme.spacingS
            x: addButton.width - width
            transformOrigin: Menu.BottomRight

            Instantiator {
                model: root.viewTypes

                MenuItem {
                    required property var modelData
                    objectName: "addView." + modelData.name
                    text: modelData.title
                    enabled: !root.hasView(modelData.name)
                    onTriggered: root.addView(modelData.name)
                }

                onObjectAdded: (index, object) => addMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => addMenu.removeItem(object)
            }
        }
    }
}
