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
import QtQuick.Controls
import QtQuick.Layouts


// Import custom styles
import "../texts"
import "../components"

ViewBaseFrame {
    labelText: qsTr("I/Q RAW Recorder (experimental)")

    property bool isStart: false
    property int ringeBufferSize: 0

    content:  ColumnLayout {
        RowLayout {
            TextStandart {
                text: qsTr("Ring buffer length [s]")
            }

            WTumbler {
                id: ringeBufferSetting
                model: [5, 10, 60, 120, 240]


                onCurrentIndexChanged: {
                     ringeBufferSize = parseInt(ringeBufferSetting.currentItem.text) * 2048 * 1024
                }
            }

            WButton {
                text: isStart ? qsTr("Save ring buffer") : qsTr("Init")

                onPressed: {
                    if(!isStart)
                        radioController.initRecorder(ringeBufferSize)
                    else
                        radioController.triggerRecorder("")

                    isStart = !isStart
                }
            }
        }

        TextStandart {
            text: qsTr("Ring buffer size (roughly): ") + (ringeBufferSize / 1000000 * 2).toFixed(0) + " MB"
        }
    }
}
