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

// Base class of the UI tests. Every test function fails if QML reported a
// warning or an error while it ran.

#ifndef UI_TEST_CASE_H
#define UI_TEST_CASE_H

#include <QJSValue>
#include <QObject>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtTest>

#include "expectations.h"
#include "ui_app.h"

// Selects the clip and starts the user interface. Skips the test if the
// test data is missing. For initTestCase().
#define UI_START_WITH_CLIP(app, clipId)                                     \
    do {                                                                    \
        hideDebugOutput();                                                  \
        const QString missing_ = (app).useClip(clipId);                     \
        if (!missing_.isEmpty())                                            \
            QSKIP(qPrintable(missing_));                                    \
        QVERIFY2((app).start(), "MainView.qml could not be loaded");        \
    } while (false)

// Fails with the list of problems if the list isn't empty
#define UI_VERIFY_EMPTY(list, what)                                          \
    do {                                                                    \
        const QStringList list_ = (list);                                   \
        QVERIFY2(list_.isEmpty(), qPrintable(QString(what) + ":\n  " + list_.join("\n  "))); \
    } while (false)

class UiTestCase : public QObject
{
    Q_OBJECT

protected:
    UiApp app;

    // Stations of the list that is shown (all or favorites): SId -> name
    QMap<quint32, QString> shownStations() const
    {
        QMap<quint32, QString> result;
        QObject* model = shownStationModel();
        if (!model)
            return result;
        const int count = model->property("count").toInt();
        for (int i = 0; i < count; i++) {
            QJSValue station;
            QMetaObject::invokeMethod(model, "get", Q_RETURN_ARG(QJSValue, station), Q_ARG(int, i));
            result[station.property("stationSId").toUInt()] = station.property("stationName").toString().trimmed();
        }
        return result;
    }

    // Row of a station in the list, scrolled into view. Null if the station
    // isn't in the shown list.
    QQuickItem* stationRow(quint32 sid)
    {
        QObject* view = app.item("stationListView");
        QObject* model = shownStationModel();
        if (!view || !model)
            return nullptr;
        const int count = model->property("count").toInt();
        for (int i = 0; i < count; i++) {
            QJSValue station;
            QMetaObject::invokeMethod(model, "get", Q_RETURN_ARG(QJSValue, station), Q_ARG(int, i));
            if (station.property("stationSId").toUInt() == sid) {
                QMetaObject::invokeMethod(view, "positionViewAtIndex", Q_ARG(int, i), Q_ARG(int, 4 /* ListView.Contain */));
                UiApp::settle(100);
                break;
            }
        }
        for (QQuickItem* row : app.visibleItems("stationRow")) {
            if (row->property("sid").toUInt() == sid)
                return row;
        }
        return nullptr;
    }

    // Shows a station list with the navigation drawer, like a user:
    // "all" or "favorites"
    bool showList(const QString& list)
    {
        QObject* drawer = app.find("navDrawer");
        if (!drawer || !app.click("navButton"))
            return false;
        if (!UiApp::waitFor([&] { return drawer->property("position").toReal() >= 1.0; }))
            return false;
        UiApp::settle(100);
        if (!app.click("navItem." + list))
            return false;
        const bool closed = UiApp::waitFor([&] { return !drawer->property("visible").toBool(); });
        UiApp::settle(100);
        return closed;
    }

    // Waits until a page transition is finished
    bool waitForPage()
    {
        QObject* stack = app.find("stackView");
        const bool ready = stack && UiApp::waitFor([&] { return !stack->property("busy").toBool(); });
        UiApp::settle(100);
        return ready;
    }

    // Scans with the button of the empty station list and waits until all
    // services of the clip are in the list
    bool scanStations()
    {
        if (!app.click("scanButton"))
            return false;
        const qsizetype expected = app.services().size();
        return UiApp::waitFor([&] { return shownStations().size() >= expected; }, 30000);
    }

private:
    QObject* shownStationModel() const
    {
        QObject* view = app.item("stationListView");
        return view ? view->property("model").value<QObject*>() : nullptr;
    }

private slots:
    void cleanup()
    {
        UI_VERIFY_EMPTY(UiApp::takeQmlMessages(), "QML warnings");
    }

    void cleanupTestCase()
    {
        app.stop();
    }
};

#endif // UI_TEST_CASE_H
