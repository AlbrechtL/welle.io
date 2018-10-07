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
    rows: isPortrait ? -1 : Math.ceil(Math.sqrt(children.length))

    property string serialized: ""
    property bool isExpert: false
    property bool isPortrait: false

    Settings {
        property alias viewComponents: gridLayout.serialized
    }


    function addComponent(path) {
        // Check of component already exisits
        for (var i = 0; i < children.length; ++i)
            if(children[i].sourcePath === path)
                return;

        var component = Qt.createComponent(path);
        var object = component.createObject(gridLayout);
        object.sourcePath = path; // Save path inside component to make a saving possible
        object.isExpert = Qt.binding(function() { return isExpert })
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
        var tmp = []
        for (var i = 0; i < children.length; ++i)
            tmp.push(children[i].sourcePath)
        serialized = JSON.stringify(tmp)
    }

    function __deserialize() {
        if(serialized != "" && serialized != "[\"\"]") {
            var tmp = JSON.parse(serialized)
            for (var i = 0; i < tmp.length; ++i)
                if(tmp[i] !== "")
                    addComponent(tmp[i])
        }
        else {
            __initComponents()
        }
    }

    function __initComponents() {
        addComponent("qrc:/QML/RadioView.qml")
        addComponent("qrc:/QML/MotView.qml")
    }
}
