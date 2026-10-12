/*
 *    Smoke test of an installed welle-io (command line option --smoke-test):
 *    plays every service of a RAW file through the complete user interface,
 *    writes what it found into a JSON report and quits with an exit code.
 *    Used by the package workflows to check the packages, see
 *    src/tests/README.md.
 *
 *    This file was generated with the help of an AI assistant (Claude Code).
 *
 *    This file is part of the welle.io.
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

#ifndef SMOKE_TEST_H
#define SMOKE_TEST_H

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

class QQmlApplicationEngine;
class CRadioController;

class CSmokeTest : public QObject
{
    Q_OBJECT

public:
    // Sample formats of --raw-format, in the order of the settings page
    static const QStringList rawFileFormats;

    // Selects a RAW file as input device in the settings, as if it was
    // chosen on the settings page. Returns false for an unknown format.
    static bool selectRawFile(const QString& fileName, const QString& format);

    // Keeps settings and caches in a temporary directory, so that the smoke
    // test neither reads nor changes those of the user. Call before
    // anything reads the settings.
    static bool isolateSettings();

    // Create before MainView.qml is loaded, to collect its QML warnings
    CSmokeTest(const QString& reportFile, int timeoutSeconds,
               CRadioController* radioController, QQmlApplicationEngine* engine,
               QObject* parent = nullptr);
    ~CSmokeTest();

    // Call after MainView.qml is loaded
    void start();

private:
    enum class Step { WaitForDevice, Scan, Play, Done };

    struct Service {
        quint32 sId = 0;
        QString label;
        QString channel;
        QString mode;
        bool isDab = false;
        bool played = false;
        QString dls;
    };

    void compileAllQml();
    void tick();
    void playNext();
    void finish(const QString& error);
    QJsonObject report(const QString& error) const;
    QJsonArray guiStations() const;
    QString graphicsApi() const;
    QString text(const char* property) const;

    QString reportFile;
    int timeoutSeconds;
    CRadioController* radioController;
    QQmlApplicationEngine* engine;

    Step step = Step::WaitForDevice;
    QTimer pollTimer;
    QTimer timeoutTimer;
    QElapsedTimer sinceLastStation;
    QElapsedTimer sincePlay;
    QElapsedTimer total;

    QList<quint32> serviceOrder;
    QMap<quint32, Service> services;
    int playing = -1;
    int slides = 0;
    QStringList errorMessages;  // shown by the user interface
    QStringList qmlCompileErrors;
};

#endif // SMOKE_TEST_H
