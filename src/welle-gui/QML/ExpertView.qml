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
    }

    function __addComponent(path) {
        var component = Qt.createComponent(path);
        var object = component.createObject(layout);
    }
}
