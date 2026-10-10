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

// Keyboard shortcuts of the window: playback, volume, stations 1 to 20,
// back and quit
Item {
    id: shortcuts

    property Player player
    property bool canGoBack: false

    signal back
    // A station was played with a shortcut, index in the shown list
    signal stationPlayed(int index)

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: false
        sequences: [StandardKey.Quit]
        onActivated: guiHelper.close()
    }

    Shortcut {
        // Back from sub pages. Popups and the drawer handle Esc themselves.
        context: Qt.WindowShortcut
        autoRepeat: false
        sequences: ["Esc", "Back"]
        enabled: shortcuts.canGoBack
        onActivated: shortcuts.back()
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: false
        sequences: ["Media Pause", "Toggle Media Play/Pause", "S"]
        onActivated: shortcuts.player.togglePlay()
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: false
        sequences: ["Media Stop"]
        onActivated: if (shortcuts.player.isActive) shortcuts.player.stop()
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: false
        sequences: ["Media Play"]
        onActivated: if (!radioController.isPlaying) shortcuts.player.play()
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: true
        sequences: ["Ctrl+Up", "Volume Up"]
        onActivated: shortcuts.player.changeVolume(0.01)
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: true
        sequences: ["Ctrl+Down", "Volume Down"]
        onActivated: shortcuts.player.changeVolume(-0.01)
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: false
        sequences: ["m", "Volume Mute"]
        onActivated: shortcuts.player.toggleMute()
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: false
        sequences: ["n", "Media Next"]
        onActivated: shortcuts.stationPlayed(shortcuts.player.playNext(1))
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        autoRepeat: false
        sequences: ["p", "Media Previous"]
        onActivated: shortcuts.stationPlayed(shortcuts.player.playNext(-1))
    }

    // Station 1 to 20 of the shown list
    Repeater {
        model: [["F1", "1"], ["F2", "2"], ["F3", "3"], ["F4", "4"], ["F5", "5"],
                ["F6", "6"], ["F7", "7"], ["F8", "8"], ["F9", "9"], ["F10", "0"],
                ["F11", "Ctrl+1"], ["F12", "Ctrl+2"], ["Ctrl+3"], ["Ctrl+4"], ["Ctrl+5"],
                ["Ctrl+6"], ["Ctrl+7"], ["Ctrl+8"], ["Ctrl+9"], ["Ctrl+0"]]

        Item {
            id: stationShortcut
            required property var modelData
            required property int index

            Shortcut {
                context: Qt.ApplicationShortcut
                autoRepeat: false
                sequences: stationShortcut.modelData
                onActivated: {
                    shortcuts.player.playAtIndex(stationShortcut.index)
                    shortcuts.stationPlayed(stationShortcut.index)
                }
            }
        }
    }
}
