import QtQuick 2.0

ListModel {
    property string serialized: ""

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

    function removeStation(sId) {
        for(var i=0; i<count; i++)
            if(get(i).stationSId === sId) {
                remove(i)
                serialize()
                return
            }
    }

    function clearStations() {
        clear()
        serialize()
    }

    function setFavorit(sId, favorit) {
        for(var i=0; i<count; i++)
            if(get(i).stationSId === sId) {
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
                if(get(i).stationName.localeCompare(get(i+1).stationName) === 1)
                    move(i,i+1,1)
            }
        }
    }

    // Necessary workaround because the settings component doesn't saves models
    function serialize() {
        var tmp = []
        for (var i = 0; i < count; ++i)
            tmp.push(get(i))
        serialized = JSON.stringify(tmp)
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
