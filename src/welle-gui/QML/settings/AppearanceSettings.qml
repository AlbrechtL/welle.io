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

// Appearance: theme and accent color
SettingSection {
    id: appearanceSettings
    objectName: "appearanceSettings"
    text: qsTr("Appearance")

    property alias qQStyleTheme : qQStyleTheme.currentIndex
    property alias accentColor : accentColorBox.currentIndex

    Settings {
        property alias qQStyleTheme: qQStyleTheme.currentIndex
        property alias accentColor: accentColorBox.currentIndex
    }

    SettingRow {
        title: qsTr("Theme")

        SettingComboBox {
            id: qQStyleTheme
            objectName: "setting.qQStyleTheme"
            model: ListModel {
                id: themeListModel
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Light") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Dark") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "System") }
            }

            Accessible.name: qsTr("Theme")
        }
    }

    SettingRow {
        title: qsTr("Accent color")

        SettingComboBox {
            id: accentColorBox
            objectName: "setting.accentColor"
            currentIndex: Theme.defaultAccent
            model: ListModel {
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Teal") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Indigo") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Deep Orange") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Purple") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Blue") }
                ListElement { label: QT_TRANSLATE_NOOP("Settings", "Green") }
            }

            Accessible.name: qsTr("Accent color")
        }
    }
}
