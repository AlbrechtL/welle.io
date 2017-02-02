import QtQuick 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "style"

Item {
    id: infoPage

    Flickable {
        anchors.fill: parent
        contentHeight: layout.implicitHeight
        contentWidth: showLicenseText.contentWidth

        ColumnLayout {
            id: layout
            anchors.fill: parent
            anchors.margins: Units.dp(20)
            spacing: Units.dp(15)
            Layout.maximumWidth: 20

            TextStandart {
                text: "Information"
                Layout.alignment: Qt.AlignLeft
                font.pixelSize: Style.textHeadingSize
            }

            TextStandart {
                id: showVersionText
                text: "welle.io version"
                Layout.alignment: Qt.AlignLeft
                objectName: "showVersionText"
            }

            TextStandart {
                text: "Licenses"
                Layout.alignment: Qt.AlignLeft
                font.pixelSize: Style.textHeadingSize
            }

            TextStandart {
                id: showGraphLicense
                text: "showGraphLicense"
                Layout.alignment: Qt.AlignLeft
                objectName: "showGraphLicense"
                wrapMode: Text.Wrap
                width: infoPage.width
                textFormat: Text.PlainText
            }

            TextStandart {
                id: showLicenseText
                text: "showLicenseText"
                Layout.alignment: Qt.AlignLeft
                objectName: "showLicenseText"
                wrapMode: Text.WrapAnywhere
                width: infoPage.width
                textFormat: Text.PlainText
            }
        }
    }
}
