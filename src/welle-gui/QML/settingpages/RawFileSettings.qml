import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.0
import Qt.labs.settings 1.0

// Import custom styles
import "../texts"
import "../components"

SettingSection {
    text: qsTr("RAW file settings")

    property string filePath
    property bool isLoaded : false

    Settings {
        property alias rawFile: filePath.text
        property alias rawFileFormat: fileFormat.currentIndex
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        onAccepted: {
            var filePath_tmp = fileDialog.fileUrl.toString()
            filePath.text = __getPath(filePath_tmp)
        }
    }

    ColumnLayout {
        spacing: Units.dp(20)

        RowLayout {
            spacing: Units.dp(5)

            WButton {
                id: openFile
                text: qsTr("Open RAW file")
                Layout.fillWidth: true
                onClicked: {
                    if(Qt.platform.os == "android") {
                        filePath.text = qsTr("Currently shown under Android")
                        __openDevice()
                    }
                    else {
                        fileDialog.open()
                    }
                }
            }

            WComboBox {
                id: fileFormat
                model: [ "auto", "u8", "s8", "s16le", "s16be", "cf32"];
            }
        }

        RowLayout {
            spacing: Units.dp(5)
            TextStandart {
                text: qsTr("Selected file:")
            }

            TextStandart {
                id: filePath
                Layout.fillWidth: true
                wrapMode: Text.WrapAnywhere
            }
        }
    }

    function initDevice(isAutoDevice) {
        if(!isAutoDevice)
            __openDevice()
        isLoaded = true
    }

    onVisibleChanged: {
        if(visible == false && isLoaded)
            __openDevice()
    }

    function __openDevice() {
        if(Qt.platform.os == "android")
            guiHelper.openRawFile(fileFormat.currentText)
        else
            guiHelper.openRawFile(filePath.text, fileFormat.currentText)
    }

    function __getPath(urlString) {
        var s
        if (urlString.startsWith("file:///")) {
            var k = urlString.charAt(9) === ':' ? 8 : 7
            s = urlString.substring(k)
        } else {
            s = urlString
        }
        return decodeURIComponent(s);
    }
}

