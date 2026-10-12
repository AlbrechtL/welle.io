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

#ifndef EXPECTATIONS_H
#define EXPECTATIONS_H

#include <QJsonArray>
#include <QList>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include "recording_runner.h"

// Checkout of the private welle.io-test-data repository:
// $WELLE_TEST_DATA_DIR, otherwise the CMake cache variable of the same name.
// Empty if neither is set.
QString testDataDir();

// Why the test data can't be used, or an empty string if it can. Tests
// that need recordings skip with this message.
QString missingTestData();

// manifest.json of the test data repository
QJsonObject loadManifest();

// Path of an unpacked clip: <test data>/clips/<id>.<format>.iq
QString clipPath(const QJsonObject& clip);

// Same as missingTestData(), for one clip
QString missingClip(const QJsonObject& clip);

// welle.io writes a lot of log messages (stderr and qDebug), which hide the
// test results. This discards them, unless the environment variable
// WELLE_TEST_VERBOSE is set. Warnings are still shown.
void hideDebugOutput();

// Result of one audio service, for the summary of all recordings
struct AudioServiceCheck {
    QString audioFormat;    // e.g. "HE-AAC v2, 48 kHz Stereo", without bitrate
    QString protection;     // e.g. "EEP 3-A"
    bool ok = false;
};

// Result of comparing a decoding result with the "expect" block of a clip
struct CheckReport {
    struct Item {
        bool ok;
        QString text;
    };
    QList<Item> items;
    QList<AudioServiceCheck> audioServices;

    bool ok() const;
    QStringList failures() const;
    // One line per check, "[ OK ] ..." or "[FAIL] ..."
    QString text() const;
};

CheckReport checkExpectations(const QJsonObject& clip,
                              const QJsonObject& defaults,
                              const RunResult& result);

#endif // EXPECTATIONS_H
