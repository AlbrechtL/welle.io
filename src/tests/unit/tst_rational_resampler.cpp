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

// unit.rational_resampler: resampling of the Airspy input from its native
// sample rates (Mini 3 MS/s, R2 2.5 MS/s, and 4.096 MS/s) to 2.048 MS/s.
// The DAB block (+-768 kHz) has to pass unchanged, signals from 1.28 MHz
// (e.g. the adjacent block at 1.712 MHz) must not alias into it.

#include <QtTest>

#include <cmath>
#include <complex>
#include <random>
#include <vector>

#include "rational_resampler.h"

namespace {

const int outputRate = 2048000;

std::vector<std::complex<float>> tone(double frequency, int rate, size_t n)
{
    // The phase in double precision, a float phase adds noise at -60 dB
    std::vector<std::complex<float>> s(n);
    for (size_t i = 0; i < n; i++)
        s[i] = std::complex<float>(std::polar(1.0, 2 * M_PI * std::fmod(frequency * i / rate, 1.0)));
    return s;
}

// Power (dB) of a resampled complex tone relative to the input tone
double gainDb(int inputRate, double frequency)
{
    const auto in = tone(frequency, inputRate, inputRate / 10);
    RationalResampler resampler(inputRate, outputRate);
    std::vector<std::complex<float>> out;
    resampler.process(in.data(), in.size(), out);

    // Skip the transient of the filter
    const size_t start = 2 * resampler.tapsPerPhase();
    double power = 0;
    for (size_t i = start; i < out.size(); i++)
        power += std::norm(out[i]);
    return 10 * std::log10(power / (out.size() - start));
}

} // namespace

class RationalResamplerTests : public QObject
{
    Q_OBJECT

private slots:
    void passesDabBlock_data();
    void passesDabBlock();
    void suppressesAliases_data();
    void suppressesAliases();
    void producesOutputRate_data();
    void producesOutputRate();
    void doesNotDependOnBlockSizes_data();
    void doesNotDependOnBlockSizes();
};

static void addRates()
{
    QTest::addColumn<int>("inputRate");
    QTest::newRow("Airspy Mini 3 MS/s") << 3000000;
    QTest::newRow("Airspy R2 2.5 MS/s") << 2500000;
    QTest::newRow("4.096 MS/s") << 4096000;
}

void RationalResamplerTests::passesDabBlock_data()
{
    QTest::addColumn<int>("inputRate");
    QTest::addColumn<double>("frequency");
    for (const int rate : { 3000000, 2500000, 4096000 })
        for (const double f : { 0.0, 100e3, -300e3, 500e3, -700e3, 768e3, -768e3 })
            QTest::newRow(qPrintable(QString("%1 MS/s, %2 kHz").arg(rate / 1e6).arg(f / 1000))) << rate << f;
}

void RationalResamplerTests::passesDabBlock()
{
    QFETCH(int, inputRate);
    QFETCH(double, frequency);
    const double gain = gainDb(inputRate, frequency);
    QVERIFY2(std::abs(gain) < 0.05, qPrintable(QString("gain %1 dB").arg(gain)));
}

void RationalResamplerTests::suppressesAliases_data()
{
    QTest::addColumn<int>("inputRate");
    QTest::addColumn<double>("frequency");
    // Frequencies that fold into the DAB block at 2.048 MS/s, as far as the
    // input rate contains them (2.5 MS/s: up to 1.25 MHz, none)
    for (const int rate : { 3000000, 4096000 })
        for (const double f : { 1.28e6, -1.4e6, 1.49e6, 1.712e6, -1.712e6, 2.0e6 })
            if (std::abs(f) < rate / 2)
                QTest::newRow(qPrintable(QString("%1 MS/s, %2 kHz").arg(rate / 1e6).arg(f / 1000))) << rate << f;
}

void RationalResamplerTests::suppressesAliases()
{
    QFETCH(int, inputRate);
    QFETCH(double, frequency);
    const double gain = gainDb(inputRate, frequency);
    QVERIFY2(gain < -65, qPrintable(QString("gain %1 dB").arg(gain)));
}

void RationalResamplerTests::producesOutputRate_data()
{
    addRates();
}

void RationalResamplerTests::producesOutputRate()
{
    QFETCH(int, inputRate);
    RationalResampler resampler(inputRate, outputRate);
    const std::vector<std::complex<float>> in(inputRate);  // 1 s
    std::vector<std::complex<float>> out;
    QCOMPARE(resampler.process(in.data(), in.size(), out), size_t(outputRate));
}

void RationalResamplerTests::doesNotDependOnBlockSizes_data()
{
    addRates();
}

void RationalResamplerTests::doesNotDependOnBlockSizes()
{
    QFETCH(int, inputRate);
    std::mt19937 rng(908);
    std::normal_distribution<float> normal;
    std::vector<std::complex<float>> in(50000);
    for (auto& z : in)
        z = { normal(rng), normal(rng) };

    RationalResampler a(inputRate, outputRate);
    std::vector<std::complex<float>> whole;
    a.process(in.data(), in.size(), whole);

    RationalResampler b(inputRate, outputRate);
    std::vector<std::complex<float>> split;
    std::uniform_int_distribution<size_t> blockSize(1, 999);
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

QTEST_GUILESS_MAIN(RationalResamplerTests)
#include "tst_rational_resampler.moc"
