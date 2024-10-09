/*
 *    Copyright (C) 2017 - 2021
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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QtCore

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
            var filePath_tmp = fileDialog.selectedFile.toString()
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

