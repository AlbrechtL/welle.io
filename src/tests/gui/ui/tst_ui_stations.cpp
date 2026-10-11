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

// gui.ui.stations: the station list. Empty state, scan from the menu,
// station logos, favorites, the lists after a restart, clearing the list,
// and a long list of the real recording with the most services.

#include <QBuffer>
#include <QImage>

#include "gui_helper.h"
#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_test_case.h"

static const QString clipId = "odr_dabmod";

class UiStationsTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();

    void showsEmptyState();
    void scansFromMenu();
    void showsStationLogos();
    void addsAndRemovesFavorites();
    void keepsListsAfterRestart();
    void clearsList();
    void longList();

private:
    // Opens the menu of the station list and clicks an entry
    bool menuEntry(const QString& objectName)
    {
        return app.click("stationListMenuButton") && app.waitVisible(objectName) && app.click(objectName);
    }

    // The logo image in a station row, if it is shown and loaded
    static QQuickItem* shownLogo(QQuickItem* row)
    {
        if (!row)
            return nullptr;
        for (QQuickItem* logo : row->findChildren<QQuickItem*>("stationLogo")) {
            if (logo->isVisible() && logo->property("status").toInt() == 1) // Image.Ready
                return logo;
        }
        return nullptr;
    }

    int count(const QString& model) const
    {
        QObject* object = app.find(model);
        return object ? object->property("count").toInt() : -1;
    }
};

void UiStationsTests::initTestCase()
{
    UI_START_WITH_CLIP(app, clipId);
}

void UiStationsTests::showsEmptyState()
{
    QVERIFY(app.isVisible("stationListEmpty"));
    QVERIFY(app.isVisible("scanButton"));
    QVERIFY(!app.isVisible("stationListView"));

    // Nothing played yet: no slide, only the "Latest" slide category,
    // which is not shown
    QCOMPARE(app.visibleItems("slideCategory").size(), 0);
    QObject* slides = app.root()->property("motSlides").value<QObject*>();
    QCOMPARE(slides->property("categories").value<QObject*>()->property("count").toInt(), 1);
    app.screenshot("stations-empty");

    // Favorites: an explanation, no scan button, a button back to all
    // stations
    QVERIFY(showList("favorites"));
    QCOMPARE(app.text("stationListTitle"), QString("Favorites"));
    QTRY_VERIFY(app.isVisible("stationListEmpty"));
    QVERIFY(!app.isVisible("scanButton"));
    app.screenshot("stations-favorites-empty");
    QVERIFY(app.click("showAllStationsButton"));
    QTRY_COMPARE(app.text("stationListTitle"), QString("All stations"));
    QVERIFY(app.isVisible("scanButton"));

    qInfo() << "The empty lists show an explanation and the scan button";
}

void UiStationsTests::scansFromMenu()
{
    QVERIFY(menuEntry("startScanItem"));
    const qsizetype expected = app.services().size();
    QTRY_VERIFY_WITH_TIMEOUT(shownStations().size() >= expected, 30000);
    QTRY_VERIFY(!app.isVisible("stationListEmpty"));
    QVERIFY(!app.isVisible("scanProgress"));

    for (const QJsonObject& service : app.services())
        QCOMPARE(shownStations().value(service["sid"].toInt()), service["label"].toString());
    QTRY_COMPARE(app.text("stationListCount").toInt(), int(app.services().size()));
    QTRY_COMPARE(app.text("navBadge.all").toInt(), int(app.services().size()));
    QCOMPARE(app.text("navBadge.favorites").toInt(), 0);

    qInfo().noquote() << "The scan from the menu found" << shownStations().size() << "stations";
}

void UiStationsTests::showsStationLogos()
{
    // No logos yet: every station has the letter avatar
    for (const QJsonObject& service : app.services())
        QVERIFY(!shownLogo(stationRow(service["sid"].toInt())));

    // A logo as the backend delivers it by SPI
    QImage image(128, 128, QImage::Format_ARGB32);
    image.fill(Qt::red);
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    QVERIFY(image.save(&buffer, "PNG"));

    const quint32 sid = app.services().first()["sid"].toInt();
    emit app.radio()->serviceLogoReceived(sid, png);

    QTRY_VERIFY(shownLogo(stationRow(sid)));
    for (const QJsonObject& service : app.services().mid(1))
        QVERIFY(!shownLogo(stationRow(service["sid"].toInt())));

    // Stored for the next start
    const QString file = app.gui()->stationLogos->directory() + "/" + QString::number(sid, 16).toUpper() + ".png";
    QVERIFY2(QFile::exists(file), qPrintable(file));
    app.screenshot("stations-logo");

    qInfo() << "A received logo is shown left of the station name";
}

void UiStationsTests::addsAndRemovesFavorites()
{
    const QJsonObject service = app.services().first();
    const quint32 sid = service["sid"].toInt();

    // Star in the list of all stations
    QQuickItem* row = stationRow(sid);
    QVERIFY(row);
    QQuickItem* star = nullptr;
    for (QObject* child : row->findChildren<QObject*>("favoriteButton"))
        star = qobject_cast<QQuickItem*>(child);
    QVERIFY(star);
    QVERIFY(app.click(star));
    QTRY_COMPARE(count("stationList"), int(app.services().size()));
    QTRY_VERIFY(stationRow(sid)->property("isFavorit").toBool());

    // Shown in the favorites
    QTRY_COMPARE(app.text("navBadge.favorites").toInt(), 1);
    QVERIFY(showList("favorites"));
    QTRY_COMPARE(shownStations().size(), 1);
    QCOMPARE(shownStations().value(sid), service["label"].toString());
    app.screenshot("stations-favorites");

    // Playing from the favorites
    QVERIFY(app.click(stationRow(sid)));
    QTRY_COMPARE_WITH_TIMEOUT(app.radio()->property("service").toUInt(), sid, 10000);

    // The star in the favorites removes it again
    row = stationRow(sid);
    for (QObject* child : row->findChildren<QObject*>("favoriteButton"))
        star = qobject_cast<QQuickItem*>(child);
    QVERIFY(app.click(star));
    QTRY_COMPARE(shownStations().size(), 0);
    QVERIFY(app.isVisible("stationListEmpty"));

    // Again a favorite, for the restart test
    QVERIFY(showList("all"));
    row = stationRow(sid);
    for (QObject* child : row->findChildren<QObject*>("favoriteButton"))
        star = qobject_cast<QQuickItem*>(child);
    QVERIFY(app.click(star));
    QTRY_COMPARE(count("favoritesList"), 1);

    qInfo() << "The star adds a station to the favorites and removes it";
}

void UiStationsTests::keepsListsAfterRestart()
{
    QVERIFY(showList("favorites"));
    QVERIFY(app.restart());

    // Favorites are shown again, both lists are restored
    QCOMPARE(app.text("stationListTitle"), QString("Favorites"));
    QCOMPARE(app.find("navDrawer")->property("current").toString(), QString("favorites"));
    QCOMPARE(shownStations().size(), 1);
    QVERIFY(showList("all"));
    QTRY_COMPARE(shownStations().size(), int(app.services().size()));

    // The logo comes from the disk cache
    QTRY_VERIFY(shownLogo(stationRow(app.services().first()["sid"].toInt())));

    qInfo() << "Station list, favorites, logos and the shown list survive a restart";
}

void UiStationsTests::clearsList()
{
    QVERIFY(menuEntry("clearListItem"));
    QTRY_COMPARE(shownStations().size(), 0);
    QVERIFY(app.isVisible("stationListEmpty"));

    qInfo() << "\"Clear station list\" empties the list";
}

void UiStationsTests::longList()
{
    const QString missing = app.useClipWithMostServices();
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));
    QVERIFY(app.restart());

    QVERIFY(scanStations());
    const QMap<quint32, QString> stations = shownStations();
    for (const QJsonObject& service : app.services())
        QCOMPARE(stations.value(service["sid"].toInt()), service["label"].toString().trimmed());

    // The list is sorted by name, the last entry is out of view
    QVERIFY(!app.visibleItems("stationRow").isEmpty());
    const QStringList names = QStringList(stations.values());
    QString lastName;
    quint32 lastSid = 0;
    for (auto it = stations.begin(); it != stations.end(); ++it) {
        if (QString::localeAwareCompare(it.value(), lastName) > 0 || lastSid == 0) {
            lastName = it.value();
            lastSid = it.key();
        }
    }
    QQuickItem* row = stationRow(lastSid);
    QVERIFY2(row, qPrintable("No row for " + lastName));
    QVERIFY(app.click(row));
    QTRY_COMPARE_WITH_TIMEOUT(app.radio()->property("service").toUInt(), lastSid, 10000);

    for (int width : {360, 1000}) {
        app.resize(width, 640);
        UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
        UI_VERIFY_EMPTY(app.controlsWithoutAccessibleName(), "Controls without accessible name");
        app.screenshot(QString("stations-long-list-%1").arg(width));
    }

    qInfo().noquote() << QString("%1 stations of %2 listed, played the last one: %3")
                             .arg(stations.size()).arg(app.clip()["id"].toString(), lastName);
}

WELLE_GUI_TEST_MAIN(UiStationsTests)

#include "tst_ui_stations.moc"
