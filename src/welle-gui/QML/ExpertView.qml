import QtQuick 2.2
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.1

// Import custom styles
import "texts"
import "components"
import "expertviews"

Item {
    implicitHeight: layout.implicitHeight
    Layout.preferredHeight: Units.dp(400)
    Layout.preferredWidth: Units.dp(400)
    Layout.fillWidth: true
    Layout.fillHeight: true

    GridLayout {
        id: layout
        anchors.top: parent.top
        flow: GridLayout.LeftToRight
        columns: (parent.width / Units.dp(400)).toFixed(0)

//        RoundButton {
//            text: "\u002b" // Unicode character '+'
//            onClicked: optionsMenu.open()

//            Menu {
//                id: optionsMenu
//                transformOrigin: Menu.TopRight

//                MenuItem {
//                    text: qsTr("Service Information")
//                    font.pixelSize: TextStyle.textStandartSize
//                    onTriggered: __addComponent("qrc:/QML/expertviews/StationInformation.qml")
//                }

//                MenuItem {
//                    text: qsTr("Spectrum")
//                    font.pixelSize: TextStyle.textStandartSize
//                    onTriggered: __addComponent("qrc:/QML/expertviews/SpectrumGraph.qml")
//                }

//                MenuItem {
//                    text: qsTr("Impulse Response")
//                    font.pixelSize: TextStyle.textStandartSize
//                    onTriggered: __addComponent("qrc:/QML/expertviews/ImpulseResponseGraph.qml")
//                }

//                MenuItem {
//                    text: qsTr("Constellation Diagram")
//                    font.pixelSize: TextStyle.textStandartSize
//                    onTriggered: __addComponent("qrc:/QML/expertviews/ConstellationGraph.qml")
//                }

//                MenuItem {
//                    text: qsTr("Null Symbol")
//                    font.pixelSize: TextStyle.textStandartSize
//                    onTriggered: __addComponent("qrc:/QML/expertviews/NullSymbolGraph.qml")
//                }

//                MenuItem {
//                    text: qsTr("Console Output")
//                    font.pixelSize: TextStyle.textStandartSize
//                    onTriggered: __addComponent("qrc:/QML/expertviews/TextOutputView.qml")
//                }
//            }
//        }
    }

    function __addComponent(path) {
        var component = Qt.createComponent(path);
        var object = component.createObject(layout);
    }
}
