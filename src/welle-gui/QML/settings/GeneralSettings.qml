/*
 *    Copyright (C) 2017 - 2026
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
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore

import "../controls"

// General settings: full screen and language
SettingSection {
    id: generalSettings
    objectName: "generalSettings"
    text: qsTr("General")

    property alias enableFullScreenState : enableFullScreen.checked

    Settings {
        property alias enableFullScreenState : generalSettings.enableFullScreenState
        property alias languageValue : languageBox.currentIndex
    }

    Connections {
        target: guiHelper

        function onSetFullScreen(isFullScreen) {
            enableFullScreen.checked = isFullScreen
        }
    }

    SettingSwitch {
        id: enableFullScreen
        objectName: "setting.enableFullScreenState"
        text: qsTr("Full screen mode")
        checked: false
        onCheckedChanged: guiHelper.setMprisFullScreenState(checked)
        Component.onCompleted: guiHelper.setMprisFullScreenState(checked)
    }

    SettingRow {
        title: qsTr("Language") == "Language" ? "Language" : qsTr("Language") + " (Language)"

        SettingComboBox {
            id: languageBox
            objectName: "setting.languageValue"
            model: ListModel {
                id: listModel
                ListElement { label: "Auto"; langCode: "auto" }
                ListElement { label: "Deutsch"; langCode: "de_DE" }
                ListElement { label: "English (GB)"; langCode: "en_GB" }
                ListElement { label: "Français"; langCode: "fr_FR" }
                ListElement { label: "Magyar"; langCode: "hu_HU" }
                ListElement { label: "Italiano"; langCode: "it_IT" }
                ListElement { label: "Nederlands"; langCode: "nl_NL" }
                ListElement { label: "Norsk bokmål"; langCode: "nb_NO" }
                ListElement { label: "Polski"; langCode: "pl_PL" }
                ListElement { label: "Ру́сский"; langCode: "ru_RU" }
                ListElement { label: "Español"; langCode: "es_ES" }
            }
            onCurrentIndexChanged: {
                // Load appropriate settings
                guiHelper.updateTranslator(listModel.get(currentIndex).langCode, Window.window);
            }

            Accessible.name: qsTr("Language")
        }
    }
}
