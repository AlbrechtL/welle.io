import QtQuick 2.0

ListModel {
    property string serialized: ""
    property string type: ""

    function addStation(station, sId, channel, favorit) {
        // Check if station already exists
        for (var i=0; i<count; i++) {
            if (get(i).stationSId === sId && get(i).channelName === channel) {
                get(i).stationName = station;
                return;
            }
        }

        append({"stationName": station, "stationSId": sId, "channelName": channel, "favorit": favorit})
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
            for (var i = 0; i < tmp.length; ++i)
                append(tmp[i])
        }
    }

    Component.onCompleted: {
        deSerialize()
    }
}
