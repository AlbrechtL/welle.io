/*
 *    Copyright (C) 2017 - 2026
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    The recorder card was reworked with the help of an AI assistant
 *    (Claude Code).
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
import QtQuick.Dialogs
import QtQuick.Layouts
import QtCore

import "../controls"

// Records the I/Q samples of the input into a file, which the RAW file input
// can play again. Optionally the recording stops after a duration.
DashboardCard {
    id: card

    labelText: qsTr("RAW Recorder")
    cardHeight: column.implicitHeight + 72

    // Durations in s, 0: until stopped
    readonly property var durations: [0, 10, 30, 60, 300, 600, 1800]
    property int duration: 0

    readonly property real bytesPerSecond: radioController.recorderBytesPerSecond
    readonly property real estimatedBytes: duration * bytesPerSecond
    readonly property real recordedSeconds: bytesPerSecond > 0 ? radioController.recordedBytes / bytesPerSecond : 0
    // Updated when the folder changes and when a recording starts or ends
    readonly property real freeBytes: {
        radioController.recordFolder
        radioController.isRecording
        return radioController.freeDiskSpace()
    }
    property string lastFile: ""

    Settings {
        property alias rawRecorderDuration: card.duration
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("Please choose a folder")
        currentFolder: card.fileUrl(radioController.recordFolder)
        onAccepted: radioController.recordFolder = selectedFolder.toString()
    }

    Connections {
        target: radioController
        function onRecordingFinished(file, reason, isError) {
            if (!isError)
                card.lastFile = file
            if (!card.dashboard)
                return
            if (isError)
                card.dashboard.message(reason, "error")
            else
                card.dashboard.message((reason !== "" ? reason + " " : "") + qsTr("Saved: %1").arg(file), "info")
        }
    }

    Component.onCompleted: {
        addEntry(qsTr("Open folder"), function() { Qt.openUrlExternally(fileUrl(radioController.recordFolder)) })
    }

    function fileUrl(path) {
        return path.startsWith("/") ? "file://" + path : "file:///" + path
    }

    function formatBytes(bytes) {
        if (bytes >= 1e9)
            return Number(bytes / 1e9).toLocaleString(Qt.locale(), "f", 1) + " GB"
        return Number(bytes / 1e6).toLocaleString(Qt.locale(), "f", bytes < 1e7 ? 1 : 0) + " MB"
    }

    function formatTime(seconds) {
        var s = Math.floor(seconds)
        var m = Math.floor(s / 60)
        s = s % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }

    function durationLabel(seconds) {
        if (seconds === 0)
            return qsTr("Until stopped")
        return seconds < 60 ? qsTr("%1 s").arg(seconds) : qsTr("%1 min").arg(seconds / 60)
    }

    content: ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.spacingS

        Label {
            objectName: "recorderCard.format"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
            text: !radioController.recorderAvailable
                  ? qsTr("The input device can't be recorded")
                  : radioController.isInputRunning || radioController.isRecording
                    ? qsTr("Format %1, 2.048 MS/s, %2 per second")
                          .arg(radioController.recorderFormat).arg(card.formatBytes(card.bytesPerSecond))
                    : qsTr("Play a station or tune to a channel to record")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.bodyMedium
            color: Material.secondaryTextColor
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
            spacing: Theme.spacingM
            enabled: radioController.recorderAvailable && !radioController.isRecording

            Label {
                Layout.fillWidth: true
                text: qsTr("Duration")
                elide: Text.ElideRight
                font.pixelSize: Theme.bodyLarge
                color: parent.enabled ? Material.foreground : Material.hintTextColor
            }

            SettingComboBox {
                id: durationBox
                objectName: "recorderCard.duration"
                model: card.durations.map(seconds => card.durationLabel(seconds))
                currentIndex: Math.max(0, card.durations.indexOf(card.duration))
                onActivated: (index) => card.duration = card.durations[index]

                Accessible.name: qsTr("Duration")
            }
        }

        Label {
            objectName: "recorderCard.estimate"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
            visible: radioController.recorderAvailable
            text: card.duration > 0
                  ? qsTr("Estimated file size: %1").arg(card.formatBytes(card.estimatedBytes))
                  : qsTr("Estimated file size: %1 per minute").arg(card.formatBytes(60 * card.bytesPerSecond))
            wrapMode: Text.Wrap
            font.pixelSize: Theme.bodyMedium
        }

        Label {
            objectName: "recorderCard.lowSpace"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
            visible: radioController.recorderAvailable && card.freeBytes >= 0
                     && card.freeBytes < Math.max(card.estimatedBytes, 60 * card.bytesPerSecond)
            text: qsTr("Only %1 free in the folder").arg(card.formatBytes(card.freeBytes))
            wrapMode: Text.Wrap
            font.pixelSize: Theme.bodySmall
            color: Theme.warning
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingM

            Button {
                objectName: "recorderCard.record"
                text: radioController.isRecording ? qsTr("Stop") : qsTr("Record")
                icon.name: radioController.isRecording ? "stop" : "record"
                icon.color: radioController.isRecording ? Material.foreground : Theme.error
                enabled: radioController.isRecording
                         || (radioController.recorderAvailable && radioController.isInputRunning
                             && !radioController.isChannelScan)
                onClicked: {
                    if (radioController.isRecording)
                        radioController.stopRecording()
                    else
                        radioController.startRecording(card.duration)
                }
            }

            Label {
                objectName: "recorderCard.status"
                Layout.fillWidth: true
                visible: radioController.isRecording
                text: (radioController.recordDuration > 0
                       ? card.formatTime(card.recordedSeconds) + " / " + card.formatTime(radioController.recordDuration)
                       : card.formatTime(card.recordedSeconds))
                      + "  ·  " + card.formatBytes(radioController.recordedBytes)
                elide: Text.ElideRight
                font.pixelSize: Theme.bodyLarge
            }
        }

        ProgressBar {
            objectName: "recorderCard.progress"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
            visible: radioController.isRecording && radioController.recordDuration > 0
            value: radioController.recordDuration > 0 ? card.recordedSeconds / radioController.recordDuration : 0
        }

        Label {
            objectName: "recorderCard.dropped"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
            visible: radioController.recordDroppedBytes > 0
            text: qsTr("%1 lost, the disk is too slow").arg(card.formatBytes(radioController.recordDroppedBytes))
            wrapMode: Text.Wrap
            font.pixelSize: Theme.bodySmall
            color: Theme.error
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            spacing: Theme.spacingM

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Folder")
                    elide: Text.ElideRight
                    font.pixelSize: Theme.bodyLarge
                }

                Label {
                    objectName: "recorderCard.folder"
                    Layout.fillWidth: true
                    text: radioController.recordFolder
                    elide: Text.ElideMiddle
                    font.pixelSize: Theme.bodySmall
                    color: Material.secondaryTextColor
                }
            }

            Button {
                objectName: "recorderCard.changeFolder"
                text: qsTr("Change")
                icon.name: "folder_open"
                flat: true
                enabled: !radioController.isRecording
                onClicked: folderDialog.open()
            }
        }

        Label {
            objectName: "recorderCard.lastFile"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
            visible: card.lastFile !== "" && !radioController.isRecording
            text: qsTr("Saved: %1").arg(card.lastFile.substring(card.lastFile.lastIndexOf("/") + 1))
            elide: Text.ElideMiddle
            font.pixelSize: Theme.bodySmall
            color: Material.secondaryTextColor
        }
    }
}
