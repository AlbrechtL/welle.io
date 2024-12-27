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

ListModel {
    property string serialized: ""
    property string type: ""

    function addStation(station, sId, channel, favorit) {
        // Check if station already exists
        for (var i=0; i<count; i++) {
            if (get(i).stationSId === sId) {
                var availableChannelNames = get(i).availableChannelNames
                if(!availableChannelNames.includes(channel)) {
                    get(i).availableChannelNames = availableChannelNames + "," + channel
                    serialize()
                }
                return;
            }
        }

        append({"stationName": station, "stationSId": sId, "channelName": channel, "availableChannelNames": channel, "favorit": favorit})
        sort()
        serialize()
    }

    function removeStation(sId, channel) {
        for(var i=0; i<count; i++)
            if(get(i).stationSId === sId && get(i).channelName === channel) {
                remove(i)
                serialize()
                return
            }
    }

    function clearStations() {
        clear()
        serialize()
    }

    function setFavorit(sId, channel, favorit) {
        for(var i=0; i<count; i++)
            if(get(i).stationSId === sId && get(i).channelName === channel) {
                get(i).favorit = favorit
                serialize()
                return
            }
    }

    function setDefaultChannel(sId, newDefaultChannel) {
        for(var i=0; i<count; i++)
            if(get(i).stationSId === sId) {
                get(i).channelName = newDefaultChannel
                serialize()
                return
            }
    }

    function sort() {
        // Simple basic bubble sort implementation
        for(var n=count; n>1; --n) {
            for(var i=0; i<n-1; ++i) {
                // Sort in alphabetical order
                if(get(i).stationName.localeCompare(get(i+1).stationName) >= 1)
                    move(i,i+1,1)
            }
        }
    }

    function getStationName(sIdDec, channel) {
        for(var i=0; i<count; i++) {
            if(get(i).stationSId == sIdDec && get(i).channelName === channel) {
                return get(i).stationName
            }
        }
    }

    function getIndex(sIdDec, channel) {
        for(var i=0; i<count; i++) {
            if(get(i).stationSId == sIdDec && get(i).channelName === channel) {
                return i
            }
        }
    }

    function getIndexPrevious(sIdDec, channel) {
        for(var i=0; i<count; i++) {
            if(get(i).stationSId == sIdDec && get(i).channelName === channel) {
                return Math.max(0, i-1)
            }
        }
        console.debug("Station '" + sIdDec + "' not found in this list. Returning index 0")
        return 0
    }

    function getIndexNext(sIdDec, channel) {
        for(var i=0; i<count; i++) {
            if(get(i).stationSId == sIdDec && get(i).channelName === channel) {
                return Math.min(count-1, i+1)
            }
        }
        console.debug("Station '" + sIdDec + "' not found in this list. Returning index 0")
        return 0
    }

    function play(channel, sidHex) {
        var sidDec = parseInt(sidHex,16);
        var stationName = getStationName(sidDec, channel)
        //console.debug("stationName: " + stationName + " channel: " + channel + " sidHex: "+ sidHex)
        if (!channel || !sidHex || !stationName) {
            infoMessagePopup.text = qsTr("Last played station not found.\nSelect a station to start playback.");
            infoMessagePopup.open();
        } else {
            radioController.play(channel, stationName, sidDec)
        }
    }

    function playAtIndex(index) {
        if (index < count) {
            //console.debug("stationName: " + get(index).stationName + " channel: " + get(index).channelName + " sidDec: " + get(index).stationSId)
            radioController.play(get(index).channelName, get(index).stationName, get(index).stationSId)
        }
    }

    // Necessary workaround because the settings component doesn't saves models
    function serialize() {
        var tmp = []
        for (var i = 0; i < count; ++i)
            tmp.push(get(i))
        serialized = JSON.stringify(tmp)
        guiHelper.updateMprisStationList(serialized, type, stationListBox.currentIndex)
    }

    function deSerialize() {
        clear()
        if(serialized != "") {
            var tmp = JSON.parse(serialized)
            for (var i = 0; i < tmp.length; ++i) {
                if(!("availableChannelNames" in tmp[i])) // Migration for welle.io 2.6 and below
                    tmp[i].availableChannelNames = tmp[i].channelName
                append(tmp[i])
            }
        }
    }

    Component.onCompleted: {
        deSerialize()
    }
}
