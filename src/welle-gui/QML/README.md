# welle-io user interface (QML)

Qt Quick Controls with the Material 3 style. `main.cpp` loads `MainView.qml`
from the resources (`resources.qrc`).

## Structure

| Folder | Content |
|---|---|
| `MainView.qml` | The window: app bar, navigation drawer, pages on a StackView, snackbar. Creates the shared state and wires everything together |
| `app/` | Parts of the window: `AppBar`, `NavDrawer`, `Snackbar`, `AppShortcuts` (keyboard shortcuts) and `Player` (play/stop, next/previous, volume) |
| `models/` | State without user interface: `StationLists` (all stations, favorites, shown list, ensemble names, MPRIS), `StationListModel`, `MotSlideModel` (slide show) |
| `controls/` | Reusable building blocks: `Theme` (design tokens, singleton), chips, setting rows, `SettingComboBox`, `AutoSizeMenu` |
| `pages/` | Pages of the StackView: `HomePage`, `SettingsPage`, `AboutPage` |
| `home/` | The home page: station list, now playing (`StationCard`, `SlideShow`), mini player |
| `settings/` | Sections of the settings page; `settings/devices/` the settings of the input devices |
| `expert/` | Expert mode: `Dashboard`, `DashboardCard` and the cards (one `SpectrumCard` for spectrum, impulse response and null symbol) |

Folders import each other with relative paths, e.g. `import "../controls"`.

## Conventions

* **No global ids.** Components get what they need as properties
  (`stations`, `player`, `slides`) instead of using `mainWindow`. The shared
  state (`StationLists`, `Player`, `MotSlideModel`) exists once in
  `MainView` and is passed down.
* **Design tokens** (spacing, radii, font sizes, status colors, breakpoints)
  come from `Theme`; colors of the style from the `Material` attached
  properties. No hard-coded colors or sizes.
* **Elevation** through `Theme.elevation(level)`: it is 0 with Qt Quick's
  software renderer, which can't draw the Material shadows.
* **objectName** on everything the UI tests look at or click, without
  translated text, e.g. `nowPlaying.station`, `setting.<QSettings key>`. See
  `src/tests/README.md`.
* **Accessible names** on every control without text (icon buttons, sliders,
  combo boxes).
* **Settings keys** (`Settings { property alias ... }`) must not change, they
  are the saved settings of existing installations. The expert views are
  saved by name (`Dashboard.viewTypes`); `Dashboard.legacyFiles` converts the
  layout of welle.io 2.7.
* **Translations**: `qsTr()` uses the file name as context. Combo box entries
  use `QT_TRANSLATE_NOOP("Settings", "...")` and `SettingComboBox`
  translates them, so they follow a language change. After moving or renaming
  files, run `lupdate` (see `welle-gui.pro`).
* New QML files are added to `resources.qrc` and, for a new folder, to the
  wildcards in `welle-gui.pro`.

---

*This file was generated with an AI assistant (Claude Code).*
