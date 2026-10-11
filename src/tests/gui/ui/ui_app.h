/*
 *    Copyright (C) 2026
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
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

// Harness for the automated UI tests: starts the welle-io user interface
// (MainView.qml) with a recording of the test data repository as RAW file
// input, like a user would have configured it, and drives it with real
// mouse and keyboard events.
//
// Elements are found by their objectName. The QML code sets a stable,
// untranslated objectName on everything a test looks at or clicks, see
// src/tests/README.md.

#ifndef UI_APP_H
#define UI_APP_H

#include <functional>
#include <memory>

#include <QJsonObject>
#include <QKeySequence>
#include <QStringList>
#include <QVariantMap>

class QObject;
class QQmlApplicationEngine;
class QQuickItem;
class QQuickWindow;
class QTranslator;
class CGUIHelper;
class CRadioController;

class UiApp
{
public:
    UiApp();
    ~UiApp();

    // --- Test data ---

    // Selects a clip of the manifest. Returns why it can't be used (no test
    // data, clip not unpacked), or an empty string.
    QString useClip(const QString& clipId);

    // The unpacked clip of the manifest with the most audio services, for
    // long lists
    QString useClipWithMostServices();

    const QJsonObject& clip() const { return currentClip; }
    QString clipFile() const { return currentClipFile; }

    // Audio services of the clip, the ones the station list shows ("sid" as
    // number, "label", "type", ... of the manifest)
    QList<QJsonObject> services() const;

    // --- Lifecycle ---

    // Writes settings before start(), e.g. {"enableExpertModeState", true}.
    // start() itself sets the RAW file input with the selected clip.
    static void writeSettings(const QVariantMap& values);

    // Starts the user interface like main.cpp does and waits until the
    // window is shown. The audio volume is 0.
    bool start();

    // Closes everything and starts again with the same settings, like a
    // restart of welle-io
    bool restart();

    void stop();

    QQuickWindow* window() const { return mainWindow; }
    QObject* root() const;
    CRadioController* radio() const { return radioController.get(); }
    CGUIHelper* gui() const { return guiHelper.get(); }

    // --- Finding elements ---

    // First object with this objectName, visible or not. Searches the QML
    // object tree and the visual item tree (popups, list delegates).
    QObject* find(const QString& objectName) const;
    QQuickItem* item(const QString& objectName) const;
    QList<QObject*> findAll(const QString& objectName) const;

    // Like findAll(), only visible items
    QList<QQuickItem*> visibleItems(const QString& objectName) const;

    // Property of a found object, invalid if there is no such object
    QVariant property(const QString& objectName, const char* name) const;

    // Text property of a found object, trimmed
    QString text(const QString& objectName) const;

    bool isVisible(const QString& objectName) const;

    // --- Input ---

    // Scrolls the Flickable around the item so that the item is visible
    void scrollIntoView(QQuickItem* item);

    // Clicks into the center of the item, after scrolling it into view.
    // Fails (returns false) if the item is not visible, its center is
    // outside the window or something else covers it.
    bool click(QQuickItem* item, Qt::MouseButton button = Qt::LeftButton);
    bool click(const QString& objectName);
    bool pressAndHold(QQuickItem* item);
    bool clickAt(QPointF scenePos);
    void key(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    void keySequence(const QKeySequence& sequence);
    bool wheel(QQuickItem* item, int angleDelta);

    // --- Waiting and layout ---

    // Waits until condition() is true, processing events
    static bool waitFor(const std::function<bool()>& condition, int timeout = 5000);
    bool waitVisible(const QString& objectName, int timeout = 5000);
    bool waitHidden(const QString& objectName, int timeout = 5000);

    // Resizes the window and waits until the layout is updated
    void resize(int width, int height);

    // Lets animations (drawers, popups, page transitions) finish
    static void settle(int ms = 400);

    // --- Generic checks ---

    // Visible items that stick out of the window horizontally. Items that
    // are scrolled out of a Flickable or clipped by a parent don't count.
    QStringList itemsOutsideWindow() const;

    // Visible, enabled controls without an accessible name
    QStringList controlsWithoutAccessibleName() const;

    // Saves the window as <WELLE_TEST_SCREENSHOT_DIR>/<name>.png, if the
    // environment variable is set
    void screenshot(const QString& name) const;

    // --- QML warnings ---

    // QML warnings and errors since the last call
    static QStringList takeQmlMessages();

    // Short description of an item for messages: type, objectName, text
    static QString describe(const QObject* object);

private:
    void forEachItem(const std::function<void(QQuickItem*)>& visit) const;

    QJsonObject currentClip;
    QString currentClipFile;

    QVariantMap commandLineOptions;
    QTranslator* translator = nullptr;
    std::unique_ptr<CRadioController> radioController;
    std::unique_ptr<CGUIHelper> guiHelper;
    std::unique_ptr<QQmlApplicationEngine> engine;
    QQuickWindow* mainWindow = nullptr;
};

#endif // UI_APP_H
