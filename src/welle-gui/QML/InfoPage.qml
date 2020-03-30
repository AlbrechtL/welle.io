import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0

// Import custom styles
import "texts"

Item {
    id: infoPage

    ColumnLayout {
        anchors.fill: parent

        TabBar {
            id: mainBar
            width: parent.width
            Layout.fillWidth: true

            TabButton {
                font.capitalization: Font.MixedCase
                text: qsTr("Versions")
                width: implicitWidth
            }
            TabButton {
                font.capitalization: Font.MixedCase
                text: qsTr("Authors")
                width: implicitWidth
            }
            TabButton {
                font.capitalization: Font.MixedCase
                text: qsTr("Thanks")
                width: implicitWidth
            }
            TabButton {
                font.capitalization: Font.MixedCase
                text: qsTr("Licenses")
                width: implicitWidth
            }

            onCurrentIndexChanged: {
                displayPage()
            }

            Connections {
                target: guiHelper
                onTranslationFinished: {
                    // Update the InfoPage otherwise it won't be re-translated
                    // if we change language in the GUI
                    displayPage()
                }
            }
        }

        TabBar {
            id: licensesBar
            width: parent.width
            Layout.fillWidth: true

            Repeater {
                model: ["GPL-2", "LGPL-2.1", "BSD-3-Clause (kiss_fft)"]
                TabButton {
                    font.capitalization: Font.MixedCase
                    text: modelData
                    width: implicitWidth
                }
            }

            onCurrentIndexChanged: {
                displayPage()
            }
        }

        Flickable {
            id: infoPageFlickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: fileContent.implicitHeight

            clip: true

            TextStandart {
                id: fileContent
                text: guiHelper.getInfoPage("Versions")
                Layout.alignment: Qt.AlignLeft
                wrapMode: Text.Wrap
                width: parent.width - scrollbar.width
                textFormat: Text.PlainText
            }

            ScrollBar.vertical: ScrollBar { id: scrollbar}
        }
    }

    function displayPage() {
        switch(mainBar.currentIndex) {
            case 0:
                fileContent.text = guiHelper.getInfoPage("Versions")
                licensesBar.visible = false
                break
            case 1:
                fileContent.text = guiHelper.getInfoPage("Authors")
                licensesBar.visible = false
                break
            case 2:
                fileContent.text = guiHelper.getInfoPage("Thanks")
                licensesBar.visible = false
                break
            case 3:
                switch(licensesBar.currentIndex) {
                    case 0: fileContent.text = guiHelper.getInfoPage("GPL-2"); break;
                    case 1: fileContent.text = guiHelper.getInfoPage("LGPL-2.1"); break;
                    case 2: fileContent.text = guiHelper.getInfoPage("BSD-3-Clause"); break;
                    default: fileContent.text = ""; break;
                }
                licensesBar.visible = true
                break
            default:
                fileContent.text = ""; break;
        }
        infoPageFlickable.contentY = 0
    }

}
