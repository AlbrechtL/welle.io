/*
 *    This file is part of the welle.io.
 *    This file was generated with the help of an AI assistant (Claude Code).
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

// Starts the complete QML user interface like main.cpp does, with the
// settings pointing to the synthetic ODR recording, and checks that the
// stations show up without any QML warnings. The detailed UI tests are in
// gui/ui.

#include <QJSValue>

#include "expectations.h"
#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_app.h"

static const QString clipId = "odr_dabmod";

class QmlSmokeTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void loadsMainView();
    void opensRawFileFromSettings();
    void showsStations();
    void noQmlWarnings();

private:
    UiApp app;
};

void QmlSmokeTests::initTestCase()
{
    hideDebugOutput();

    const QString missing = app.useClip(clipId);
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));
}

void QmlSmokeTests::cleanupTestCase()
{
    app.stop();
}

void QmlSmokeTests::loadsMainView()
{
    QVERIFY(app.start());
    QCOMPARE(app.root()->property("title").toString(), QString("welle.io"));

    qInfo() << "MainView.qml loaded";
}

void QmlSmokeTests::opensRawFileFromSettings()
{
    QTRY_COMPARE_WITH_TIMEOUT(app.radio()->property("deviceId").value<CDeviceID>(), CDeviceID::RAWFILE, 5000);
    QVERIFY(app.radio()->property("deviceName").toString().contains(QFileInfo(app.clipFile()).fileName()));

    qInfo().noquote() << "The settings pages opened the input device:"
                      << app.radio()->property("deviceName").toString();
}

void QmlSmokeTests::showsStations()
{
    QObject* stationList = app.find("stationList");
    QVERIFY(stationList);

    // Same as the "Start station scan" menu entry
    QMetaObject::invokeMethod(app.radio(), "startScan");

    QTRY_COMPARE_WITH_TIMEOUT(stationList->property("count").toInt(), 2, 20000);

    QStringList names;
    for (int i = 0; i < 2; i++) {
        QJSValue station;
        QVERIFY(QMetaObject::invokeMethod(stationList, "get", Q_RETURN_ARG(QJSValue, station), Q_ARG(int, i)));
        names << station.property("stationName").toString().trimmed();
    }
    names.sort();
    QCOMPARE(names, QStringList({"DAB Test 1", "DAB+ Test 2"}));

    qInfo().noquote() << "Station list in QML:" << names.join(", ");
}

void QmlSmokeTests::noQmlWarnings()
{
    const QStringList messages = UiApp::takeQmlMessages();
    QVERIFY2(messages.isEmpty(), qPrintable(messages.join("\n")));

    qInfo() << "No QML warnings or errors";
}

WELLE_GUI_TEST_MAIN(QmlSmokeTests)

#include "tst_qml_smoke.moc"
