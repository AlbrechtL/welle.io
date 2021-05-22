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
 
import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1

// Import custom styles
import "../texts"
import "../components"

Switch {
    id: wSwitch

    font.pixelSize: TextStyle.textStandartSize
    font.family: TextStyle.textFont
    height: Units.dp(24)

    contentItem: Text {
              text: wSwitch.text
              font: wSwitch.font
              color: (mainWindow.Material.theme === Material.Dark ) ? "lightgrey" : (mainWindow.Universal.theme === Universal.Dark ) ? "lightgrey" : TextStyle.textColor
              verticalAlignment: Text.AlignVCenter
              wrapMode: Text.WordWrap
              leftPadding: wSwitch.indicator.width + wSwitch.spacing
          }
}
