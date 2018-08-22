import QtQuick 2.0

ListModel {
    id: listModel

    property string serialized: ""

    // ToDo not working
    function sortModel()  {
        for(var i=0; i<count; i++) {
            for(var j=0; j<i; j++) {
                if(get(i).stationName.localeCompare(get(j).stationName) === 1)
                    move(i,j,1)
                break
            }
        }
    }

    function addStation(station, channel) {
        // Check if station already exits
        for(var i=0; i<listModel.count; i++)
            if(listModel.get(i).stationName === station)
                return

        listModel.append({"stationName": station, "channelName": channel})
        serialize()
    }

    function clearStations() {
        listModel.clear()
        serialize()
    }

    // Necessary workaround because the settings component doesn't saves models
    function serialize() {
        var listModel_tmp = []
        for (var i = 0; i < listModel.count; ++i)
            listModel_tmp.push(listModel.get(i))
        serialized = JSON.stringify(listModel_tmp)
    }

    function deSerialize() {
        listModel.clear()
        if(serialized != "") {
            var listModel_tmp = JSON.parse(serialized)
            for (var i = 0; i < listModel_tmp.length; ++i)
                listModel.append(listModel_tmp[i])
        }
    }

    Component.onCompleted: {
        deSerialize()
    }
}
