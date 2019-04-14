import QtQuick 2.0
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0

import "components"

GridLayout {
    id: gridLayout
    anchors.fill: parent
    anchors.margins: Units.dp(5)
    columnSpacing: Units.dp(10)
    rowSpacing: Units.dp(10)
    flow: GridLayout.TopToBottom

    property string serialized: ""
    property bool isExpert: false
    property bool isPortrait: false
    property int viewCount: 0

    Settings {
        property alias viewComponents: gridLayout.serialized
    }

    function addComponent(path, index = -1) {
        // Check of component already exisits
        for (var i = 0; i < children.length; ++i)
            if(children[i].sourcePath === path)
                return;

        var component = Qt.createComponent(path);
        var object = component.createObject(gridLayout);
        object.sourcePath = path; // Save path inside component to make a saving possible
        object.isExpert = Qt.binding(function() { return isExpert })

        if(index === -1)
            object.index = viewCount
        else
            object.index = index

        viewCount = viewCount + 1;

        if(index === -1)
            __placeObjects()

        __serialize()
    }

    function onRequestPositionChange(sender, row, column) {
        var index = 0

        // Find object in occupied cell
        for (var i = 0; i < children.length; ++i) {
            var child = children[i]
            if(row === child.Layout.row && column === child.Layout.column) {
               index = child.index
               break
            }
        }

        // Remove sender index from list
        for (var i = 0; i < children.length; ++i) {
            var child = children[i]
            if(child.index > sender.index)
                child.index--
        }

        // Move items down from new index
        for (var i = 0; i < children.length; ++i) {
            var child = children[i]
            if(child.index >= index)
                child.index++
        }

        // Give sender object the new index
        sender.index = index

        // Finally reorder the items
        __placeObjects()

        // Save it
        __serialize()
    }

    onIsExpertChanged: {
        if(!isExpert) {
            // Delete everything except RadioView and MotView
            for (var i = 0; i < children.length; ++i)
                if(!children[i].sourcePath.includes("RadioView") && !children[i].sourcePath.includes("MotView"))
                    children[i].destroy()
        }
    }

    onChildrenChanged: {
        __serialize()
    }

    Component.onCompleted: {
        __deserialize()
    }

    function __serialize() {
        var pathList = []
        var indexList = []
        for (var i = 0; i < children.length; ++i) {
            pathList.push(children[i].sourcePath)
            indexList.push(children[i].index)
        }
        serialized = JSON.stringify([pathList, indexList])
    }

    function __deserialize() {
        if(serialized != "" && serialized != "[\"\"]") {
            var tmp = JSON.parse(serialized)

            var pathList = tmp[0]
            var indexList = tmp[1]

            if(Array.isArray(pathList)) {
                for (var i = 0; i < pathList.length; ++i)
                    if(pathList !== "")
                        addComponent(pathList[i], indexList[i])

                __placeObjects()
            }
            else { // Fall back for old settings
                for (var i = 0; i < tmp.length; ++i)
                    if(tmp !== "")
                        addComponent(tmp[i])
                }
        }
        else {
            __initComponents()
        }
    }

    function __initComponents() {
        addComponent("qrc:/QML/RadioView.qml")
        addComponent("qrc:/QML/MotView.qml")
    }

    function __placeObjects(){
        // Distribute rows and columns equaly
        var rows = Math.ceil(Math.sqrt(viewCount)) - 1

        var rowIndex = 0
        var columnIndex = 0

        for (var i = 0; i < children.length; ++i) {
            var child = __getObjectByIndex(i)

            if(child === undefined)
                for (var j = 0; j < children.length; ++j)
                    console.debug("child undefined with index " + children[j].index)

            // Assign cell to each visual elememnt
            child.Layout.row = rowIndex
            child.Layout.column = columnIndex

            rowIndex++

            // Use next column
            if(rowIndex > rows) {
                columnIndex++
                rowIndex = 0
            }
        }
    }

    function __getObjectByIndex(index) {
        for (var i = 0; i < children.length; ++i)
            if(index === children[i].index)
                return children[i]
    }
}
