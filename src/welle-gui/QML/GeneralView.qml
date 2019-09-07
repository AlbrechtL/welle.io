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

    Settings {
        property alias viewComponents: gridLayout.serialized
    }

    function addComponent(path, row, column) {
        // Check of component already exists
        for (var i = 0; i < children.length; ++i)
            if(children[i].sourcePath === path)
                return;

        var rows = Math.ceil(Math.sqrt(children.length + 1)) -  1
        var foundCell = false

        var rowIndex = 0
        var columnIndex = 0

        if(row === -1 && column === -1) {
            // Find next free cell
            while(!foundCell) {
                 if(__checkCell(rowIndex, columnIndex) === undefined) {
//                     console.debug("Found cell row: " + rowIndex + " column: " + columnIndex)
                     foundCell = true
                     break
                 }

                rowIndex++

               // Use next column
               if(rowIndex > rows && !isPortrait) {
                   columnIndex++
                   rowIndex = 0
               }
            }
        }
        else {
            rowIndex = row
            columnIndex = column
        }

        // Create new view
        console.debug("Creating component: " + path)
        var component = Qt.createComponent(path);
        var object = component.createObject(gridLayout);
        object.sourcePath = path; // Save path inside component to make a saving possible
        object.isExpert = Qt.binding(function() { return isExpert })

        // Assign cell
        object.Layout.row = rowIndex
        object.Layout.column = columnIndex

        // Save view
        __serialize()
    }

    function onRequestPositionChange(sender, row, column) {
        var cellChild = __checkCell(row, column)

        // Flip position if cell is used
        if(cellChild !== undefined) {
            cellChild.Layout.row = sender.Layout.row
            cellChild.Layout.column = sender.Layout.column
        }

        sender.Layout.row = row
        sender.Layout.column = column

        // Save view
        __serialize()
    }

    function onItemRemove(sender) {

    }

    function onRequestMaximize(sender, isMaximize) {
        if(isMaximize === true) {
            for(var i = 0; i < children.length; ++i)
                if(children[i] !== sender)
                   children[i].visible = false
        }
        else {
            for(var i = 0; i < children.length; ++i)
                children[i].visible = true
        }
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
        var rowList = []
        var columnList = []
        for (var i = 0; i < children.length; ++i) {
            pathList.push(children[i].sourcePath)
            rowList.push(children[i].Layout.row)
            columnList.push(children[i].Layout.column)
        }
        serialized = JSON.stringify([pathList, rowList, columnList])
    }

    function __deserialize() {
        //console.debug("Serialized components: " + serialized)
        if(serialized != "" && serialized != "[\"\"]"  && serialized != "[[\"\"],[0],[0]]") {
            var tmp = JSON.parse(serialized)

            var pathList = tmp[0]
            var rowList = tmp[1]
            var columnList = tmp[2]

            if(Array.isArray(pathList)) {
                for (var i = 0; i < pathList.length; ++i)
                    if(pathList !== "")
                        addComponent(pathList[i], rowList[i], columnList[i])
            }
            else { // Fall back for old settings
                for (var i = 0; i < tmp.length; ++i)
                    if(tmp !== "")
                        addComponent(tmp[i], -1, -1)
                }
        }
        else {
            __initComponents()
        }
    }

    function __initComponents() {
        addComponent("qrc:/QML/RadioView.qml", -1, -1)
        addComponent("qrc:/QML/MotView.qml", -1, -1)
    }

    function __checkCell(row, column) {
        // Check if cell is already in use
        for (var i = 0; i < children.length; ++i) {
            if(row === children[i].Layout.row && column === children[i].Layout.column)
                return children[i]
        }

        return undefined
    }
}
