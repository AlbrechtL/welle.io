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
            if (filePath.text != __getPath(filePath_tmp)) {
                filePath.text = __getPath(filePath_tmp)
                __openDevice()
            }
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
                sizeToContents: true
                model: [ "auto", "u8", "s8", "s16le", "s16be", "cf32"];
                onCurrentIndexChanged: {
                     if (isLoaded)
                         __openDevice()
                 }
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

    function __openDevice() {
        // Don't use fileFormat.currentText because if __openDevice is called
        // from fileFormat.onCurrentIndexChanged the value of currentText is incorrect
        var fileFormatText = fileFormat.model[fileFormat.currentIndex];

        if(Qt.platform.os == "android")
            guiHelper.openRawFile(fileFormatText)
        else {
            console.debug("RAWFile path: " + filePath.text)
            console.debug("RAWFile format set to: " + fileFormatText)
            guiHelper.openRawFile(filePath.text, fileFormatText)
        }
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

