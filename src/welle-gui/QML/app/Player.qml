/*
 *    This file is part of the welle.io.
 *    This file was generated with an AI assistant (Claude Code).
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

import "../models"

// Playback and volume: play/stop the last station, next/previous station
// of the shown list, volume and mute. MainView has one instance, the app
// bar, the mini player and the keyboard shortcuts use it.
QtObject {
    id: player

    property StationLists stations

    // The receiver plays or scans
    readonly property bool isActive: radioController.isPlaying || radioController.isChannelScan

    property real volume: radioController.volume
    property real volumeBeforeMute: 1

    onVolumeChanged: {
        if (volume !== radioController.volume)
            radioController.setVolume(volume)
    }

    property Connections radioConnection: Connections {
        target: radioController
        function onVolumeChanged(volume) { player.volume = volume }
    }

    function setVolume(value) {
        volume = Math.max(0, Math.min(1, Math.round(value * 100) / 100))
    }

    function changeVolume(delta) {
        setVolume(volume + delta)
    }

    function toggleMute() {
        if (volume !== 0) {
            volumeBeforeMute = volume
            setVolume(0)
        } else {
            setVolume(volumeBeforeMute > 0 ? volumeBeforeMute : 1)
        }
    }

    function togglePlay() {
        if (isActive)
            stop()
        else
            play()
    }

    // Plays the last station again
    function play() {
        stations.all.play(radioController.lastChannel[1], radioController.lastChannel[0])
    }

    function stop() {
        if (radioController.isPlaying)
            radioController.stop()
        else if (radioController.isChannelScan)
            radioController.stopScan()
    }

    function playAtIndex(index) {
        stations.shown.playAtIndex(index)
    }

    // Plays the next (direction 1) or previous (-1) station of the shown
    // list and returns its index
    function playNext(direction) {
        var model = stations.shown
        var channel = radioController.lastChannel[1]
        var sid = parseInt(radioController.lastChannel[0], 16)
        var index = direction > 0 ? model.getIndexNext(sid, channel) : model.getIndexPrevious(sid, channel)
        model.playAtIndex(index)
        return index
    }
}
