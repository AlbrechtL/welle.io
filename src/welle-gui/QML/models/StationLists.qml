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
import QtCore

// The station lists: all stations and the favorites, which of them is
// shown, the names of the ensembles on the channels and MPRIS. Saved in the
// settings. MainView has one instance.
QtObject {
    id: lists

    readonly property StationListModel all: StationListModel {
        id: allStations
        objectName: "stationList"
        type: "all"
        mprisListIndex: lists.shownIndex
        onInfoMessage: (text) => lists.infoMessage(text)
    }

    readonly property StationListModel favorites: StationListModel {
        id: favoriteStations
        objectName: "favoritesList"
        type: "favorites"
        mprisListIndex: lists.shownIndex
    }

    // 0: all stations, 1: favorites
    property int shownIndex: 0
    readonly property StationListModel shown: shownIndex === 0 ? all : favorites

    // Ensemble names by channel, as JSON
    property string knownEnsembleNamesSerialized

    signal infoMessage(string text)

    function showAll() { shownIndex = 0 }
    function showFavorites() { shownIndex = 1 }

    function setFavorite(stationName, sid, channel, favorite) {
        all.setFavorit(sid, channel, favorite)
        if (favorite)
            favorites.addStation(stationName, sid, channel, true)
        else
            favorites.removeStation(sid, channel)
    }

    // Name of the ensemble on a channel, known from earlier reception
    function ensembleName(channel) {
        if (knownEnsembleNamesSerialized === "" || channel === "")
            return ""
        var names = JSON.parse(knownEnsembleNamesSerialized)
        return (channel in names) ? names[channel] : ""
    }

    function updateMpris() {
        guiHelper.updateMprisStationList(shown.serialized, shown.type, shownIndex)
    }

    onShownIndexChanged: updateMpris()
    Component.onCompleted: updateMpris()

    property Settings settings: Settings {
        property alias stationListSerialize: allStations.serialized
        property alias favoritsListSerialize: favoriteStations.serialized
        property alias stationListBoxIndex: lists.shownIndex
        property alias knownEnsembleNamesSerialized: lists.knownEnsembleNamesSerialized
    }

    property Connections radioConnection: Connections {
        target: radioController

        function onNewStationNameReceived(station, sId, channel) {
            lists.all.addStation(station, sId, channel, false)
        }

        function onEnsembleChanged() {
            var ensemble = radioController.ensemble.trim()
            var channel = radioController.channel
            if (ensemble === "")
                return
            var names = lists.knownEnsembleNamesSerialized !== "" ? JSON.parse(lists.knownEnsembleNamesSerialized) : {}
            if (names[channel] !== ensemble) {
                names[channel] = ensemble
                lists.knownEnsembleNamesSerialized = JSON.stringify(names)
            }
        }
    }
}
