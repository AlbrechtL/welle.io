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

#include <QtTest>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "null_radio_controller.h"
#include "raw_file.h"

// CRAWFile reads in chunks of 32768 bytes and drops a trailing partial
// chunk at the end of the file, so every test file has exactly this size.
static constexpr size_t FILE_SIZE = 32768;

class MessageRecorder : public NullRadioController {
public:
    std::vector<std::string> errors;

    void onMessage(message_level_t level, const std::string& text, const std::string& = std::string()) override
    {
        if (level == message_level_t::Error)
            errors.push_back(text);
    }
};

class RawFileTests : public QObject
{
    Q_OBJECT

private slots:
    void convertsSamples_data();
    void convertsSamples();
    void autoFormatFallsBackToU8();
    void unknownFormatReportsError();
    void missingFileReportsError();

private:
    QTemporaryDir tmpDir;

    QString writeFile(const QString& name, const std::vector<uint8_t>& data);
    std::vector<DSPCOMPLEX> readAll(const QString& path, const std::string& format, size_t numSamples);
};

// Deterministic, non-symmetric test pattern so that swapped bytes or swapped
// I/Q components produce different values.
static int16_t patternS16(size_t i) { return (int16_t)(uint16_t)(i * 2731u + 12345u); }
static uint8_t patternU8(size_t i) { return (uint8_t)(i * 37u + 11u); }

QString RawFileTests::writeFile(const QString& name, const std::vector<uint8_t>& data)
{
    const QString path = tmpDir.filePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        qFatal("Cannot write %s", qPrintable(path));
    f.write(reinterpret_cast<const char*>(data.data()), (qint64)data.size());
    return path;
}

std::vector<DSPCOMPLEX> RawFileTests::readAll(const QString& path, const std::string& format, size_t numSamples)
{
    NullRadioController rc;
    CRAWFile rawFile(rc, false, false);
    rawFile.setFileName(path.toStdString(), format);
    rawFile.restart();

    std::vector<DSPCOMPLEX> samples(numSamples);
    const int32_t n = rawFile.getSamples(samples.data(), (int32_t)numSamples);
    samples.resize(n);
    return samples;
}

void RawFileTests::convertsSamples_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("format");

    QTest::newRow("u8 explicit") << "a.bin" << "u8";
    QTest::newRow("u8 auto") << "a.u8.iq" << "auto";
    QTest::newRow("s8 explicit") << "b.bin" << "s8";
    QTest::newRow("s8 auto") << "b.s8.iq" << "auto";
    QTest::newRow("s16le explicit") << "c.bin" << "s16le";
    QTest::newRow("s16le auto") << "c.s16le.iq" << "auto";
    QTest::newRow("s16be explicit") << "d.bin" << "s16be";
    QTest::newRow("s16be auto") << "d.s16be.iq" << "auto";
    QTest::newRow("cf32 explicit") << "e.bin" << "cf32";
    QTest::newRow("cf32 auto") << "e.cf32.iq" << "auto";
}

void RawFileTests::convertsSamples()
{
    QFETCH(QString, fileName);
    QFETCH(QString, format);

    // The effective format is the explicit one, or the one from the suffix
    QString fmt = format;
    if (fmt == "auto")
        fmt = fileName.section('.', -2, -2);

    std::vector<uint8_t> data(FILE_SIZE);
    std::vector<DSPCOMPLEX> expected;

    if (fmt == "u8" || fmt == "s8") {
        for (size_t i = 0; i < FILE_SIZE; i++)
            data[i] = patternU8(i);
        for (size_t i = 0; i < FILE_SIZE / 2; i++) {
            if (fmt == "u8")
                expected.emplace_back((data[2*i] - 128) / 128.0f, (data[2*i+1] - 128) / 128.0f);
            else
                expected.emplace_back((int8_t)data[2*i] / 128.0f, (int8_t)data[2*i+1] / 128.0f);
        }
    }
    else if (fmt == "s16le" || fmt == "s16be") {
        const bool le = (fmt == "s16le");
        for (size_t i = 0; i < FILE_SIZE / 2; i++) {
            const uint16_t v = (uint16_t)patternS16(i);
            data[2*i + (le ? 0 : 1)] = v & 0xFF;
            data[2*i + (le ? 1 : 0)] = v >> 8;
        }
        for (size_t i = 0; i < FILE_SIZE / 4; i++)
            expected.emplace_back((float)patternS16(2*i), (float)patternS16(2*i+1));
    }
    else if (fmt == "cf32") {
        for (size_t i = 0; i < FILE_SIZE / 8; i++)
            expected.emplace_back(patternS16(2*i) / 32768.0f, patternS16(2*i+1) / 32768.0f);
        std::memcpy(data.data(), expected.data(), FILE_SIZE);
    }
    else {
        QFAIL("Unknown format in test data");
    }

    const QString path = writeFile(fileName, data);
    const auto samples = readAll(path, format.toStdString(), expected.size());

    QCOMPARE(samples.size(), expected.size());
    for (size_t i = 0; i < expected.size(); i++) {
        if (samples[i] != expected[i]) {
            QFAIL(qPrintable(QString("Sample %1: got (%2, %3), expected (%4, %5)")
                        .arg(i)
                        .arg(samples[i].real()).arg(samples[i].imag())
                        .arg(expected[i].real()).arg(expected[i].imag())));
        }
    }
}

void RawFileTests::autoFormatFallsBackToU8()
{
    std::vector<uint8_t> data(FILE_SIZE);
    for (size_t i = 0; i < FILE_SIZE; i++)
        data[i] = patternU8(i);

    const QString path = writeFile("recording.raw", data);
    const auto samples = readAll(path, "auto", FILE_SIZE / 2);

    QCOMPARE(samples.size(), FILE_SIZE / 2);
    QCOMPARE(samples[1], DSPCOMPLEX((data[2] - 128) / 128.0f, (data[3] - 128) / 128.0f));
}

void RawFileTests::unknownFormatReportsError()
{
    const QString path = writeFile("f.bin", std::vector<uint8_t>(FILE_SIZE));

    MessageRecorder rc;
    CRAWFile rawFile(rc, false, false);
    rawFile.setFileName(path.toStdString(), "s24");

    QCOMPARE(rc.errors.size(), size_t(1));
    QCOMPARE(QString::fromStdString(rc.errors[0]), QString("Unknown RAW file format"));
}

void RawFileTests::missingFileReportsError()
{
    MessageRecorder rc;
    CRAWFile rawFile(rc, false, false);
    rawFile.setFileName(tmpDir.filePath("does-not-exist.iq").toStdString(), "auto");

    QVERIFY(!rawFile.is_ok());
    QCOMPARE(rc.errors.size(), size_t(1));
}

QTEST_APPLESS_MAIN(RawFileTests)

#include "tst_rawfile.moc"
