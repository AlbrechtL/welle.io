/*
 *    Copyright (C) 2017 - 2026
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
import QtQuick.Controls.Material
import QtQuick.Window
import QtCore

import "app"
import "controls"
import "models"
import "pages"

// Main window: app bar, navigation drawer and the pages on a StackView.
// The start page is HomePage (station list and player). The state that
// several parts share (station lists, playback, slide show) is created
// here and given to them as properties.
ApplicationWindow {
    id: mainWindow
    objectName: "mainWindow"

    readonly property bool isExpertView: settingsPage.expertSettings.enableExpertModeState
    readonly property bool isFullScreen: settingsPage.generalSettings.enableFullScreenState
    readonly property bool isStationNameInWindowTitle: settingsPage.stationSettings.addStationNameToWindowTitleState
    readonly property bool darkTheme: Material.theme === Material.Dark

    // Responsive layout, see Theme
    readonly property string layoutMode: width < Theme.compactWidth ? "compact"
                                       : width < Theme.expandedWidth ? "medium" : "expanded"

    readonly property bool canGoBack: stack.depth > 1 || homePage.detailOpen

    // For the UI tests
    readonly property alias stations: stationLists
    readonly property alias motSlides: slideShowModel

    property bool isLoaded: false
    // Error message that arrived before the window was ready
    property string pendingErrorMessage: ""

    StationLists {
        id: stationLists
        onInfoMessage: (text) => snackbar.show(text, "info")
    }

    Player {
        id: playback
        stations: stationLists
    }

    MotSlideModel {
        id: slideShowModel
    }

    // Full screen on Android and on small screens
    readonly property bool _fillScreen: Screen.desktopAvailableWidth < 960 || Screen.desktopAvailableHeight < 640
                                        || Qt.platform.os === "android"

    width: _fillScreen ? Screen.desktopAvailableWidth : 960
    height: _fillScreen ? Screen.desktopAvailableHeight : 640
    minimumWidth: Math.min(360, Screen.desktopAvailableWidth)
    minimumHeight: Math.min(400, Screen.desktopAvailableHeight)

    title: isStationNameInWindowTitle ? radioController.title.trim() + " - welle.io" : "welle.io"

    visible: true // According to https://bugreports.qt.io/browse/QTBUG-35244
    visibility: isFullScreen ? Window.FullScreen : Window.Windowed

    Material.accent: Theme.accents[Math.max(0, Math.min(settingsPage.appearanceSettings.accentColor, Theme.accents.length - 1))].color

    Binding {
        target: Theme
        property: "dark"
        value: mainWindow.darkTheme
    }

    Binding {
        target: Theme
        property: "effects"
        value: mainWindow.contentItem.GraphicsInfo.api !== GraphicsInfo.Software
    }

    Settings {
        property alias width: mainWindow.width
        property alias height: mainWindow.height
        property alias volume: playback.volume
        property alias stationListWidth: homePage.listWidth
    }

    Component.onCompleted: {
        console.debug("os: " + Qt.platform.os)
        console.debug("desktopAvailableWidth: " + Screen.desktopAvailableWidth)
        console.debug("desktopAvailableHeight: " + Screen.desktopAvailableHeight)
        console.debug("orientation: " + Screen.orientation)
        console.debug("devicePixelRatio: " + Screen.devicePixelRatio)
        console.debug("pixelDensity: " + Screen.pixelDensity)

        // Android: always full screen, also if the settings say otherwise
        if (Qt.platform.os === "android") {
            mainWindow.width = Screen.desktopAvailableWidth
            mainWindow.height = Screen.desktopAvailableHeight
        }

        updateTheme()
        isLoaded = true

        // Show error message if one occurred during startup
        if (pendingErrorMessage !== "")
            snackbar.show(pendingErrorMessage, "error")
    }

    function updateTheme() {
        switch (settingsPage.appearanceSettings.qQStyleTheme) {
        case 0: mainWindow.Material.theme = Material.Light; break
        case 1: mainWindow.Material.theme = Material.Dark; break
        case 2: mainWindow.Material.theme = Material.System; break
        }
    }

    // Snackbar message, kind "info" or "error"
    function showMessage(text, kind) {
        snackbar.show(text, kind)
    }

    // --- Navigation ---

    function navigate(destination) {
        switch (destination) {
        case "all":
        case "favorites":
            stack.pop(null)
            homePage.detailOpen = false
            if (destination === "all")
                stationLists.showAll()
            else
                stationLists.showFavorites()
            break
        case "settings":
            openPage(settingsPage)
            break
        case "expertSettings":
            openSettings("expert")
            break
        case "stationSettings":
            openSettings("stations")
            break
        case "about":
            openPage(aboutPage)
            break
        case "exit":
            guiHelper.close()
            break
        }
    }

    // Shows a page on top of the home page
    function openPage(page) {
        if (stack.currentItem === page)
            return
        if (stack.depth > 1)
            stack.pop(null, StackView.Immediate)
        stack.push(page)
    }

    // Shows the settings page scrolled to a section
    function openSettings(section) {
        if (stack.currentItem === settingsPage) {
            settingsPage.showSection(section)
        } else {
            settingsPage.pendingSection = section
            openPage(settingsPage)
        }
    }

    function goBack() {
        if (stack.depth > 1)
            stack.pop()
        else if (homePage.detailOpen)
            homePage.detailOpen = false
    }

    header: AppBar {
        title: stack.depth > 1 && stack.currentItem && stack.currentItem.title ? stack.currentItem.title
               : homePage.detailOpen ? homePage.detailTitle : "welle.io"
        player: playback
        canGoBack: mainWindow.canGoBack
        compact: mainWindow.layoutMode === "compact"
        onOpenNavigation: navDrawer.open()
        onBack: mainWindow.goBack()
    }

    NavDrawer {
        id: navDrawer
        current: stack.currentItem === settingsPage ? "settings"
                 : stack.currentItem === aboutPage ? "about"
                 : stationLists.shownIndex === 0 ? "all" : "favorites"
        allStationsCount: stationLists.all.count
        favoritesCount: stationLists.favorites.count
        onNavigate: (destination) => mainWindow.navigate(destination)
    }

    StackView {
        id: stack
        objectName: "stackView"
        anchors.fill: parent
        initialItem: homePage
    }

    HomePage {
        id: homePage
        layoutMode: mainWindow.layoutMode
        isExpert: mainWindow.isExpertView
        stations: stationLists
        player: playback
        slides: slideShowModel
        onOpenStationSettings: mainWindow.navigate("stationSettings")
        onShowAllStations: mainWindow.navigate("all")
        onMessage: (text, kind) => snackbar.show(text, kind)
    }

    // Created at startup, see SettingsPage
    SettingsPage {
        id: settingsPage
        visible: false
    }

    AboutPage {
        id: aboutPage
        visible: false
    }

    Snackbar {
        id: snackbar
        bottomOffset: homePage.miniPlayerVisible && stack.depth === 1 ? 64 : 0
    }

    AppShortcuts {
        player: playback
        canGoBack: mainWindow.canGoBack && !navDrawer.opened
        onBack: mainWindow.goBack()
        onStationPlayed: (index) => homePage.stationListPane.view.currentIndex = index
    }

    Connections {
        target: settingsPage.appearanceSettings
        function onQQStyleThemeChanged() { mainWindow.updateTheme() }
    }

    Connections {
        target: radioController

        function onShowErrorMessage(text) {
            if (mainWindow.isLoaded)
                snackbar.show(text, "error")
            else
                mainWindow.pendingErrorMessage = text
        }

        function onShowInfoMessage(text) {
            snackbar.show(text, "info")
        }
    }

    Connections {
        target: guiHelper

        function onMinimizeWindow() { mainWindow.hide() }
        function onMaximizeWindow() { mainWindow.showMaximized() }
        function onRestoreWindow() {
            // On Linux (KDE?): Hide before we restore
            // otherwise the window will occasionally not be brought to the front
            if (Qt.platform.os === "linux" && !mainWindow.active)
                mainWindow.hide()
            mainWindow.showNormal()
            mainWindow.raise() // Stay in foreground
            if (Qt.platform.os === "linux" && !mainWindow.active)
                mainWindow.requestActivate()
        }
    }

    onVisibilityChanged: (visibility) => {
        if (visibility === Window.Minimized)
            guiHelper.tryHideWindow()
    }
}
