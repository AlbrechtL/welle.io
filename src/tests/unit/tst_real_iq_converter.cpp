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

// unit.real_iq_converter: conversion of the real samples of the Airspy ADC
// (Linux kernel SDR input) to complex samples. A real tone at fs/4 + d has
// to become a complex tone at +d (or -d with inverted spectrum) of the same
// amplitude, its mirror at -d has to be suppressed, and so do the tones that
// the second stage (from 10 MHz) folds into the DAB block.

#include <QtTest>

#include <cmath>
#include <complex>
#include <random>
#include <vector>

#include "v4l2_sdr.h"

namespace {

std::vector<float> realTone(double frequency, int rate, size_t n)
{
    // The phase in double precision, a float phase adds noise at -60 dB
    std::vector<float> s(n);
    for (size_t i = 0; i < n; i++)
        s[i] = float(std::cos(2 * M_PI * std::fmod(frequency * i / rate, 1.0)));
    return s;
}

// Amplitude (dB relative to 1) of the complex output at frequency f, after
// the transient, over 10 ms: an integer number of periods of 2 * f for f in
// multiples of 50 Hz, so a tone at -f doesn't leak into the result
double amplitudeDb(const RealIqConverter& converter, const std::vector<std::complex<float>>& out, double f)
{
    const int outputRate = converter.outputRate();
    const size_t skip = converter.transientSamples();
    const size_t n = outputRate / 100;
    std::complex<double> sum = 0;
    for (size_t k = skip; k < skip + n; k++)
        sum += std::complex<double>(out[k]) * std::polar(1.0, -2 * M_PI * std::fmod(f * k / outputRate, 1.0));
    return 20 * std::log10(std::abs(sum) / n + 1e-20);
}

// Converts a real tone at the given frequency
std::vector<std::complex<float>> convert(RealIqConverter& converter, int inputRate, double frequency)
{
    const size_t outputs = converter.transientSamples() + converter.outputRate() / 100;
    const auto in = realTone(frequency, inputRate, outputs * inputRate / converter.outputRate());
    std::vector<std::complex<float>> out;
    converter.process(in.data(), in.size(), out);
    return out;
}

struct Result {
    double wantedDb;  // amplitude at the expected frequency
    double mirrorDb;  // amplitude at the mirrored frequency
};

// Converts a real tone at fs/4 + offset and measures the complex output at
// +offset and -offset
Result convertTone(int inputRate, double offset, bool invert)
{
    RealIqConverter converter(inputRate, invert);
    const auto out = convert(converter, inputRate, inputRate / 4.0 + offset);
    const double wanted = invert ? -offset : offset;
    return { amplitudeDb(converter, out, wanted), amplitudeDb(converter, out, -wanted) };
}

void addRatesAndOffsets(bool withZero)
{
    QTest::addColumn<int>("inputRate");
    QTest::addColumn<double>("offset");
    // Airspy Mini 12 MHz and 6 MHz, R2 20 MHz
    for (const int rate : { 6000000, 12000000, 20000000 })
        for (const double f : { 0.0, 100e3, -300e3, 500e3, 768e3, -768e3 })
            if (withZero || f != 0)
                QTest::newRow(qPrintable(QString("%1 MHz, %2 kHz").arg(rate / 1e6).arg(f / 1000))) << rate << f;
}

} // namespace

class RealIqConverterTests : public QObject
{
    Q_OBJECT

private slots:
    void passesDabBlock_data() { addRatesAndOffsets(true); }
    void passesDabBlock();
    void suppressesMirror_data() { addRatesAndOffsets(false); }
    void suppressesMirror();
    void invertsSpectrum_data() { addRatesAndOffsets(false); }
    void invertsSpectrum();
    void suppressesSecondStageAliases_data();
    void suppressesSecondStageAliases();
    void producesOutputRate_data();
    void producesOutputRate();
    void doesNotDependOnBlockSizes();
    void rejectsTooLowRate();
};

void RealIqConverterTests::passesDabBlock()
{
    QFETCH(int, inputRate);
    QFETCH(double, offset);
    const Result r = convertTone(inputRate, offset, false);
    QVERIFY2(std::abs(r.wantedDb) < 0.05, qPrintable(QString("gain %1 dB").arg(r.wantedDb)));
}

void RealIqConverterTests::suppressesMirror()
{
    QFETCH(int, inputRate);
    QFETCH(double, offset);
    const Result r = convertTone(inputRate, offset, false);
    QVERIFY2(r.mirrorDb < -75, qPrintable(QString("mirror %1 dB").arg(r.mirrorDb)));
}

void RealIqConverterTests::invertsSpectrum()
{
    QFETCH(int, inputRate);
    QFETCH(double, offset);
    const Result r = convertTone(inputRate, offset, true);
    QVERIFY2(std::abs(r.wantedDb) < 0.05, qPrintable(QString("gain %1 dB").arg(r.wantedDb)));
    QVERIFY2(r.mirrorDb < -75, qPrintable(QString("mirror %1 dB").arg(r.mirrorDb)));
}

void RealIqConverterTests::suppressesSecondStageAliases_data()
{
    QTest::addColumn<int>("inputRate");
    QTest::addColumn<double>("offset");
    // After the first stage (fs/2) these are fs/4 - 500 kHz, which the
    // second stage (fs/4) folds onto -500 kHz
    QTest::newRow("12 MHz") << 12000000 << 2500e3;
    QTest::newRow("20 MHz") << 20000000 << 4500e3;
}

void RealIqConverterTests::suppressesSecondStageAliases()
{
    QFETCH(int, inputRate);
    QFETCH(double, offset);
    RealIqConverter converter(inputRate, false);
    const auto out = convert(converter, inputRate, inputRate / 4.0 + offset);
    const double alias = amplitudeDb(converter, out, -500e3);
    QVERIFY2(alias < -75, qPrintable(QString("alias %1 dB").arg(alias)));
}

void RealIqConverterTests::producesOutputRate_data()
{
    QTest::addColumn<int>("inputRate");
    QTest::addColumn<int>("outputRate");
    QTest::newRow("6 MHz: one stage") << 6000000 << 3000000;
    QTest::newRow("12 MHz: two stages") << 12000000 << 3000000;
    QTest::newRow("20 MHz: two stages") << 20000000 << 5000000;
}

void RealIqConverterTests::producesOutputRate()
{
    QFETCH(int, inputRate);
    QFETCH(int, outputRate);
    RealIqConverter converter(inputRate, false);
    QCOMPARE(converter.outputRate(), outputRate);

    // 1 s in odd pieces, the rest is kept for the next call
    const std::vector<float> in(inputRate / 1000 + 1);
    std::vector<std::complex<float>> out;
    size_t total = 0;
    for (int i = 0; i < 1000; i++)
        total += converter.process(in.data(), in.size(), out);
    const size_t expected = size_t(inputRate / 1000 + 1) * 1000 / (inputRate / outputRate);
    QCOMPARE(total, expected);
    QCOMPARE(out.size(), total);
}

void RealIqConverterTests::doesNotDependOnBlockSizes()
{
    std::mt19937 rng(908);
    std::normal_distribution<float> normal(0, 0.2f);
    std::vector<float> in(50001);
    for (auto& x : in)
        x = normal(rng);

    RealIqConverter a(12000000, true);
    std::vector<std::complex<float>> whole;
    a.process(in.data(), in.size(), whole);

    RealIqConverter b(12000000, true);
    std::vector<std::complex<float>> split;
    std::uniform_int_distribution<size_t> blockSize(0, 999);
    size_t done = 0;
    while (done < in.size()) {
        const size_t n = std::min(blockSize(rng), in.size() - done);
        b.process(in.data() + done, n, split);
        done += n;
    }

    QCOMPARE(split.size(), whole.size());
    for (size_t i = 0; i < whole.size(); i++)
        QCOMPARE(split[i], whole[i]);
}

void RealIqConverterTests::rejectsTooLowRate()
{
    // +-768 kHz around fs/4 doesn't fit into 3 MHz
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, RealIqConverter(3000000, false));
}

QTEST_GUILESS_MAIN(RealIqConverterTests)
#include "tst_real_iq_converter.moc"
