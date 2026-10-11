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

// backend.spi.<id>: decodes the recorded SPI subchannels listed in the
// "spi" section of manifest.json of the test data repository and compares
// the logos with the expectations stored there.
//
// The recordings are the logical frames of a packet mode subchannel,
// recorded with src/tests/tools/spi_capture.cpp.

#include <QtTest>

#include <QFile>
#include <QJsonArray>

#include "expectations.h"
#include "spi_replay.h"

class SpiRecordingTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void decodeSpi_data();
    void decodeSpi();

private:
    QJsonObject manifest;
};

void SpiRecordingTests::initTestCase()
{
    hideDebugOutput();

    const QString missing = missingTestData();
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));

    manifest = loadManifest();
    if (manifest["spi"].toArray().isEmpty())
        QSKIP("The test data has no SPI recordings (\"spi\" in manifest.json)");
}

void SpiRecordingTests::decodeSpi_data()
{
    QTest::addColumn<QJsonObject>("recording");

    for (const auto& value : manifest["spi"].toArray()) {
        const auto recording = value.toObject();
        QTest::newRow(qPrintable(recording["id"].toString())) << recording;
    }
}

void SpiRecordingTests::decodeSpi()
{
    QFETCH(QJsonObject, recording);

    const QString path = testDataDir() + "/" + recording["file"].toString();
    if (!QFile::exists(path))
        QSKIP(qPrintable("Missing " + path));

    const SpiReplayResult r = replaySpi(path.toStdString(), recording["bitrate"].toInt(),
                                        recording["packet_address"].toInt());
    QVERIFY(r.opened);

    qInfo().noquote() << QString("%1: %2 s, %3 packets, %4 CRC errors, %5 FEC frames with %6 corrected bytes, "
                                 "%7 data groups, %8 objects of %9 in the directory, %10 logos")
        .arg(recording["id"].toString())
        .arg(r.frames * 24 / 1000)
        .arg(r.packets.packets)
        .arg(r.packets.packetCrcErrors)
        .arg(r.packets.fecFrames)
        .arg(r.packets.fecCorrectedBytes)
        .arg(r.packets.dataGroups)
        .arg(r.objects)
        .arg(r.directoryObjects)
        .arg(r.logos.size());

    const QJsonObject expect = recording["expect"].toObject();
    QCOMPARE(int(r.directoryObjects), expect["directory_objects"].toInt());
    QVERIFY2(int(r.objects) >= expect["min_objects"].toInt(), "Too few complete MOT objects");
    if (expect.contains("max_packet_crc_errors"))
        QVERIFY2(int(r.packets.packetCrcErrors) <= expect["max_packet_crc_errors"].toInt(), "Too many packet CRC errors");
    if (expect.contains("min_fec_corrected_bytes"))
        QVERIFY2(int(r.packets.fecCorrectedBytes) >= expect["min_fec_corrected_bytes"].toInt(), "The FEC corrected too few bytes");

    // The decoding is deterministic: exactly the expected logos
    QMap<uint32_t, QString> expected, actual;
    for (const auto& value : expect["logos"].toArray()) {
        const QJsonObject logo = value.toObject();
        expected[logo["sid"].toString().toUInt(nullptr, 16)] =
            QString("%1x%2").arg(logo["width"].toInt()).arg(logo["height"].toInt());
    }
    for (const auto& logo : r.logos)
        actual[logo.first] = QString("%1x%2").arg(logo.second.width).arg(logo.second.height);
    QCOMPARE(actual, expected);

    // The logos are images
    for (const auto& logo : r.logos) {
        QImage image;
        QVERIFY2(image.loadFromData(logo.second.data.data(), logo.second.data.size()),
                 qPrintable(QString("Logo of 0x%1 is not an image").arg(logo.first, 0, 16)));
        if (logo.second.width > 0)
            QCOMPARE(image.width(), logo.second.width);
    }
}

QTEST_GUILESS_MAIN(SpiRecordingTests)
#include "tst_spi_recordings.moc"
