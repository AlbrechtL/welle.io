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

// unit.halfband_decimator: decimation 4.096 -> 2.048 MS/s of the Airspy
// input. The DAB block (+-768 kHz) has to pass unchanged, the adjacent
// blocks (centre +-1.712 MHz) must not alias into it (issue #908).

#include <QtTest>

#include <cmath>
#include <complex>
#include <random>
#include <vector>

#include "halfband_decimator.h"

namespace {

const double inputRate = 4096000;

// Power (dB) of a decimated complex tone relative to the input tone
double gainDb(double frequency)
{
    const size_t n = 65536;
    std::vector<std::complex<float>> in(n);
    // The phase in double precision, a float phase adds noise at -60 dB
    for (size_t i = 0; i < n; i++)
        in[i] = std::complex<float>(std::polar(1.0, 2 * M_PI * std::fmod(frequency * i / inputRate, 1.0)));

    std::vector<std::complex<float>> out(n / 2);
    HalfbandDecimator decimator;
    const size_t produced = decimator.process(in.data(), n, out.data());

    // Skip the transient of the filter
    double power = 0;
    const size_t start = HalfbandDecimator::taps;
    for (size_t i = start; i < produced; i++)
        power += std::norm(out[i]);
    return 10 * std::log10(power / (produced - start));
}

} // namespace

class HalfbandDecimatorTests : public QObject
{
    Q_OBJECT

private slots:
    void passesDabBlock_data();
    void passesDabBlock();
    void suppressesAliases_data();
    void suppressesAliases();
    void doesNotDependOnBlockSizes();
};

void HalfbandDecimatorTests::passesDabBlock_data()
{
    QTest::addColumn<double>("frequency");
    for (const double f : { 0.0, 100e3, -300e3, 500e3, -700e3, 768e3, -768e3 })
        QTest::newRow(qPrintable(QString("%1 kHz").arg(f / 1000))) << f;
}

void HalfbandDecimatorTests::passesDabBlock()
{
    QFETCH(double, frequency);
    const double gain = gainDb(frequency);
    QVERIFY2(std::abs(gain) < 0.02, qPrintable(QString("gain %1 dB").arg(gain)));
}

void HalfbandDecimatorTests::suppressesAliases_data()
{
    QTest::addColumn<double>("frequency");
    // Folds onto the DAB block after the decimation
    for (const double f : { 1.28e6, -1.3e6, 1.712e6, -1.712e6, 2.0e6 })
        QTest::newRow(qPrintable(QString("%1 kHz").arg(f / 1000))) << f;
}

void HalfbandDecimatorTests::suppressesAliases()
{
    QFETCH(double, frequency);
    // The average of two samples (before) gave -5 dB at 1.28 MHz and
    // -12 dB at 1.712 MHz
    const double gain = gainDb(frequency);
    QVERIFY2(gain < -65, qPrintable(QString("gain %1 dB").arg(gain)));
}

void HalfbandDecimatorTests::doesNotDependOnBlockSizes()
{
    std::mt19937 rng(908);
    std::normal_distribution<float> normal;
    std::vector<std::complex<float>> in(20000);
    for (auto& z : in)
        z = { normal(rng), normal(rng) };

    std::vector<std::complex<float>> whole(in.size() / 2);
    HalfbandDecimator a;
    QCOMPARE(a.process(in.data(), in.size(), whole.data()), in.size() / 2);

    // Blocks of random sizes, also odd ones
    std::vector<std::complex<float>> split(in.size() / 2 + 1);
    HalfbandDecimator b;
    std::uniform_int_distribution<size_t> blockSize(1, 999);
    size_t done = 0;
    size_t produced = 0;
    while (done < in.size()) {
        const size_t n = std::min(blockSize(rng), in.size() - done);
        produced += b.process(in.data() + done, n, split.data() + produced);
        done += n;
    }
    QCOMPARE(produced, whole.size());
    for (size_t i = 0; i < whole.size(); i++)
        QCOMPARE(split[i], whole[i]);
}

QTEST_GUILESS_MAIN(HalfbandDecimatorTests)
#include "tst_halfband_decimator.moc"
