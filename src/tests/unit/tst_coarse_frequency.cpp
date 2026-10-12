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

// unit.coarse_frequency: coarse frequency offset from the spectrum of the
// phase reference symbol (PRS). The offset has to be found with a timing
// offset, an echo and noise; noise and a null symbol with TII carriers
// (which have the phases of the PRS) must give no estimate (issue #908).

#include <QtTest>

#include <cmath>
#include <complex>
#include <random>
#include <vector>

#include "coarse-frequency.h"
#include "phasetable.h"

namespace {

using Spectrum = std::vector<DSPCOMPLEX>;

struct Channel {
    int shift = 0;          // frequency offset in carriers
    double timing = 0;      // FFT window offset in samples
    double echoGain = 0;    // second path
    int echoDelay = 0;      // in samples
    double snrDb = 100;     // per carrier
    std::vector<int> onlyCarriers;  // if not empty: only these carriers (TII)
};

// Received PRS spectrum in FFT order (carrier k at index k mod T_u)
Spectrum receivedPrs(const DABParams& p, const Channel& c, std::mt19937& rng)
{
    PhaseTable table(p.dabMode);
    Spectrum s(p.T_u, 0);
    std::vector<int> carriers = c.onlyCarriers;
    if (carriers.empty()) {
        for (int k = -p.K / 2; k <= p.K / 2; k++)
            if (k != 0)
                carriers.push_back(k);
    }

    for (const int k : carriers) {
        const double w = 2 * M_PI * k / p.T_u;
        const DSPCOMPLEX h = DSPCOMPLEX(1, 0) +
            std::polar(float(c.echoGain), float(-w * c.echoDelay));
        const DSPCOMPLEX prs = std::polar(1.0f, float(table.get_Phi(k)));
        s[(k + c.shift + p.T_u) % p.T_u] = prs * h * std::polar(1.0f, float(-w * c.timing));
    }

    const float sigma = std::sqrt(std::pow(10.0, -c.snrDb / 10) / 2);
    std::normal_distribution<float> normal(0, sigma);
    for (auto& z : s)
        z += DSPCOMPLEX(normal(rng), normal(rng));
    return s;
}

} // namespace

class CoarseFrequencyTests : public QObject
{
    Q_OBJECT

private slots:
    void findsOffset_data();
    void findsOffset();
    void noEstimateForNoise();
    void noEstimateForNullSymbolWithTii();
};

void CoarseFrequencyTests::findsOffset_data()
{
    QTest::addColumn<int>("shift");
    QTest::addColumn<double>("timing");
    QTest::addColumn<double>("echoGain");
    QTest::addColumn<double>("snrDb");

    for (const int shift : { -36, -20, -1, 0, 1, 7, 35 }) {
        QTest::newRow(qPrintable(QString("%1 carriers").arg(shift)))
            << shift << 0.0 << 0.0 << 30.0;
        // The FFT window 199 samples before the PRS (as the time
        // synchronisation places it), an echo of 0.9 after 20 us, 3 dB SNR
        QTest::newRow(qPrintable(QString("%1 carriers, timing, echo, noise").arg(shift)))
            << shift << 199.0 << 0.9 << 3.0;
    }
}

void CoarseFrequencyTests::findsOffset()
{
    QFETCH(int, shift);
    QFETCH(double, timing);
    QFETCH(double, echoGain);
    QFETCH(double, snrDb);

    const DABParams p(1);
    CoarseFrequencyEstimator estimator(p);
    std::mt19937 rng(908);

    Channel c;
    c.shift = shift;
    c.timing = timing;
    c.echoGain = echoGain;
    c.echoDelay = 41;
    c.snrDb = snrDb;
    for (int i = 0; i < 10; i++) {
        const Spectrum s = receivedPrs(p, c, rng);
        QCOMPARE(estimator.estimate(s.data()), int16_t(shift));
    }
}

void CoarseFrequencyTests::noEstimateForNoise()
{
    const DABParams p(1);
    CoarseFrequencyEstimator estimator(p);
    std::mt19937 rng(1);
    std::normal_distribution<float> normal;

    for (int i = 0; i < 100; i++) {
        Spectrum s(p.T_u);
        for (auto& z : s)
            z = DSPCOMPLEX(normal(rng), normal(rng));
        QCOMPARE(estimator.estimate(s.data()), CoarseFrequencyEstimator::noEstimate);
    }
}

void CoarseFrequencyTests::noEstimateForNullSymbolWithTii()
{
    // TII of transmission mode I: pairs of carriers with the phases of the
    // PRS, 4 groups of 8 carriers. The time synchronisation can lock onto
    // the null symbol because of them, but it is not the PRS.
    const DABParams p(1);
    CoarseFrequencyEstimator estimator(p);
    std::mt19937 rng(2);

    Channel c;
    c.snrDb = 20;
    const int comb = 5;
    for (const int b : { 0, 2, 5, 7 }) {
        const int k = 1 + 2 * comb + 48 * b;
        for (const int carrier : { k - 769, k - 768, k - 385, k - 384, k, k + 1, k + 384, k + 385 })
            c.onlyCarriers.push_back(carrier);
    }

    for (int i = 0; i < 20; i++) {
        const Spectrum s = receivedPrs(p, c, rng);
        QCOMPARE(estimator.estimate(s.data()), CoarseFrequencyEstimator::noEstimate);
    }
}

QTEST_GUILESS_MAIN(CoarseFrequencyTests)
#include "tst_coarse_frequency.moc"
