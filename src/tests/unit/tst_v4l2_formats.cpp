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

// unit.v4l2_formats: conversion of the sample formats of the Linux kernel
// SDR interface (V4L2_SDR_FMT_*) to float samples with full scale 1.

#include <QtTest>

#include <complex>
#include <vector>

#include "v4l2_sdr.h"

using namespace V4L2SdrFormats;

class V4L2FormatsTests : public QObject
{
    Q_OBJECT

private slots:
    void convertsComplex_data();
    void convertsComplex();
    void convertsReal();
    void ignoresIncompleteSamples();
};

void V4L2FormatsTests::convertsComplex_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<QList<float>>("expected");  // I, Q, I, Q, ...

    // rtl2832_sdr, the same as librtlsdr: 128 is 0
    QTest::newRow("CU8") << int(SdrFormat::CU8)
        << QByteArray("\x80\x00\xff\xc0", 4)
        << QList<float>{ 0.0f, -1.0f, 127 / 128.0f, 0.5f };
    QTest::newRow("CS8") << int(SdrFormat::CS8)
        << QByteArray("\x00\x80\x7f\x40", 4)
        << QList<float>{ 0.0f, -1.0f, 127 / 128.0f, 0.5f };
    QTest::newRow("CU16LE") << int(SdrFormat::CU16LE)
        << QByteArray("\x00\x80\x00\x00\xff\xff\x00\xc0", 8)
        << QList<float>{ 0.0f, -1.0f, 32767 / 32768.0f, 0.5f };
    // 14 bit: -8192 with zero padding (0x2000) and sign extended (0xe000)
    QTest::newRow("CS14LE") << int(SdrFormat::CS14LE)
        << QByteArray("\x00\x20\x00\xe0\xff\x1f\x00\x10", 8)
        << QList<float>{ -1.0f, -1.0f, 8191 / 8192.0f, 0.5f };
}

void V4L2FormatsTests::convertsComplex()
{
    QFETCH(int, format);
    QFETCH(QByteArray, bytes);
    QFETCH(QList<float>, expected);
    const auto f = SdrFormat(format);

    QVERIFY(isComplex(f));
    std::vector<std::complex<float>> out(expected.size() / 2);
    const size_t n = convertComplex(f, reinterpret_cast<const uint8_t*>(bytes.constData()), bytes.size(), out.data());

    QCOMPARE(n, size_t(expected.size() / 2));
    for (size_t i = 0; i < n; i++) {
        QCOMPARE(out[i].real(), expected[2 * i]);
        QCOMPARE(out[i].imag(), expected[2 * i + 1]);
    }
}

void V4L2FormatsTests::convertsReal()
{
    // airspy: 12 bit unsigned, 2048 is 0, the high 4 bits are ignored
    const SdrFormat f = SdrFormat::RU12LE;
    QVERIFY(!isComplex(f));
    const QByteArray bytes("\x00\x08\x00\x00\xff\x0f\x00\x0c\x00\xf8", 10);
    const QList<float> expected{ 0.0f, -1.0f, 2047 / 2048.0f, 0.5f, 0.0f };

    std::vector<float> out(expected.size());
    QCOMPARE(convertReal(f, reinterpret_cast<const uint8_t*>(bytes.constData()), bytes.size(), out.data()),
             size_t(expected.size()));
    for (int i = 0; i < expected.size(); i++)
        QCOMPARE(out[i], expected[i]);
}

void V4L2FormatsTests::ignoresIncompleteSamples()
{
    std::complex<float> out[2];
    const uint8_t bytes[7] = {};
    QCOMPARE(convertComplex(SdrFormat::CU8, bytes, 3, out), size_t(1));
    QCOMPARE(convertComplex(SdrFormat::CU16LE, bytes, 7, out), size_t(1));

    float real[4];
    QCOMPARE(convertReal(SdrFormat::RU12LE, bytes, 7, real), size_t(3));
    QCOMPARE(convertReal(SdrFormat::CU8, bytes, 7, real), size_t(0));
}

QTEST_GUILESS_MAIN(V4L2FormatsTests)
#include "tst_v4l2_formats.moc"
