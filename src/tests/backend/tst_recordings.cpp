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

// Decodes every recording listed in manifest.json of the test data
// repository with all services and compares the result with the
// expectations stored there.

#include <QtTest>

#include "expectations.h"
#include "recording_runner.h"

class RecordingTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void decodeRecording_data();
    void decodeRecording();
    void cleanupTestCase();

private:
    QJsonObject manifest;

    // For the summary: passed and failed count per key
    struct Count {
        int passed = 0;
        int failed = 0;
    };
    QMap<QString, Count> audioFormats;
    QMap<QString, Count> protections;
    QMap<QString, Count> sampleFormats;

    static QString summaryTable(const QString& title, const QMap<QString, Count>& counts);
};

void RecordingTests::initTestCase()
{
    hideDebugOutput();

    const QString missing = missingTestData();
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));

    manifest = loadManifest();
}

void RecordingTests::decodeRecording_data()
{
    QTest::addColumn<QJsonObject>("clip");

    for (const auto& value : manifest["clips"].toArray()) {
        const auto clip = value.toObject();
        QTest::newRow(qPrintable(clip["id"].toString())) << clip;
    }
}

void RecordingTests::decodeRecording()
{
    QFETCH(QJsonObject, clip);

    const QString missing = missingClip(clip);
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));
    const QString path = clipPath(clip);

    RunOptions options;
    options.timeoutSeconds = 120;
    const RunResult result = runRecording(path.toStdString(), "auto", options);

    const CheckReport report = checkExpectations(clip, manifest["defaults"].toObject(), result);

    qInfo().noquote() << QString("%1 (%2, %3 s): %4\n%5")
        .arg(clip["id"].toString(), result.format.c_str())
        .arg(result.signalSeconds, 0, 'f', 1)
        .arg(clip["description"].toString(), report.text());

    for (const auto& service : report.audioServices) {
        auto& format = audioFormats[service.audioFormat];
        (service.ok ? format.passed : format.failed)++;
        if (!service.protection.isEmpty()) {
            auto& protection = protections[service.protection];
            (service.ok ? protection.passed : protection.failed)++;
        }
    }
    auto& sampleFormat = sampleFormats[QString::fromStdString(result.format)];
    (report.ok() ? sampleFormat.passed : sampleFormat.failed)++;

    if (!report.ok()) {
        qInfo().noquote() << "All details of the decoding: welle-test-probe" << path;
        QFAIL(qPrintable(report.failures().join("\n   ")));
    }
}

QString RecordingTests::summaryTable(const QString& title, const QMap<QString, Count>& counts)
{
    QStringList lines;
    lines << title << "  passed failed";
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        lines << QString("  %1 %2   %3").arg(it->passed, 6).arg(it->failed, 6).arg(it.key());
    return lines.join("\n");
}

void RecordingTests::cleanupTestCase()
{
    // Only useful if more than one recording was tested
    int recordings = 0;
    for (const auto& count : sampleFormats)
        recordings += count.passed + count.failed;
    if (recordings < 2)
        return;

    qInfo().noquote() << "Summary\n" +
        summaryTable("Audio services by audio format:", audioFormats) + "\n" +
        summaryTable("Audio services by protection:", protections) + "\n" +
        summaryTable("Recordings by sample format:", sampleFormats);
}

QTEST_GUILESS_MAIN(RecordingTests)

#include "tst_recordings.moc"
