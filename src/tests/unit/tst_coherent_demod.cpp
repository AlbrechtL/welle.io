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

// unit.coherent_demod: the coherent demodulation (CoherentDemodulator in
// ofdm-decoder.h) with
// synthetic spectra of DAB frames: the PRS and data symbols with known
// bits, through a channel with an echo, noise, a different FFT window
// position and phase in every frame, and a frequency offset and a sample
// clock error that move the phase and the timing from symbol to symbol.

#include <QtTest>

#include <cmath>
#include <complex>
#include <random>
#include <vector>

#include "ofdm-decoder.h"
#include "phasetable.h"

namespace {

struct Channel {
    double echoGain = 0;
    int echoDelay = 41;         // samples
    double snrDb = 100;         // per carrier
    double timingJump = 0;      // FFT window offset, added in every frame
    double timingDrift = 0;     // samples per symbol (sample clock)
    double phaseDrift = 0;      // rad per symbol (frequency offset)
    bool newPhase = false;      // random phase in every frame
    bool scrambled = false;     // random phase of every carrier (another channel)
};

using Spectrum = std::vector<DSPCOMPLEX>;

// Number of symbols of a frame that are tested (the demodulation of symbol
// sym needs the symbols before it)
const int symbols = 30;

// Generates frames and collects the soft values of the coherent and the
// differential demodulation
class FrameSource {
public:
    FrameSource(const Channel& c, unsigned seed) :
        p(1), c(c), rng(seed), table(1),
        sigma(std::sqrt(std::pow(10.0, -c.snrDb / 10) / 2))
    {
        std::uniform_real_distribution<float> angle(-M_PI, M_PI);
        for (int k = -p.K / 2; k <= p.K / 2; k++) {
            if (k != 0) {
                carriers.push_back(k);
                scramble.push_back(c.scrambled ? std::polar(1.0f, angle(rng)) : 1);
            }
        }
    }

    // One frame through the demodulator. Collects the soft values if count
    // is set.
    void frame(CoherentDemodulator& demod, bool count)
    {
        timing += c.timingJump;
        if (c.newPhase)
            phase = std::uniform_real_distribution<float>(-M_PI, M_PI)(rng);

        // Absolute phase index of every carrier: PRS phase + sym * pi/4 +
        // index * pi/2
        std::vector<int> state(carriers.size(), 0);
        Spectrum previous = spectrum(0, state);
        demod.processPRS(previous.data());

        std::uniform_int_distribution<int> data(0, 3);
        for (int sym = 1; sym < symbols; sym++) {
            std::vector<int> n(carriers.size());
            for (size_t i = 0; i < carriers.size(); i++) {
                n[i] = data(rng);
                state[i] = (state[i] + n[i]) & 3;
            }
            const Spectrum s = spectrum(sym, state);

            // The frequency offset turns the end of the symbol against its
            // guard interval by phaseDrift * T_u / T_s
            const DSPCOMPLEX guard = std::polar(1000.0f, float(c.phaseDrift * p.T_u / p.T_s));
            const bool ready = demod.ready();
            demod.demodulate(s.data(), sym, guard);

            if (count) {
                for (size_t i = 0; i < carriers.size(); i++) {
                    const int bin = (carriers[i] + p.T_u) % p.T_u;
                    // Phase difference pi/4 + n * pi/2: the real part is
                    // negative for n = 1, 2 (bit 1), the imaginary part for
                    // n = 2, 3
                    const bool bitRe = n[i] == 1 || n[i] == 2;
                    const bool bitIm = n[i] == 2 || n[i] == 3;
                    // Soft values with the sign of the bit (> 0: right)
                    const float signRe = bitRe ? -1 : 1;
                    const float signIm = bitIm ? -1 : 1;
                    coherent.push_back(ready ? signRe * demod.llrReal(bin) : 0);
                    coherent.push_back(ready ? signIm * demod.llrImag(bin) : 0);
                    const DSPCOMPLEX r = s[bin] * std::conj(previous[bin]);
                    differential.push_back(signRe * r.real());
                    differential.push_back(signIm * r.imag());
                }
            }
            previous = s;
        }
    }

    // Bit errors of the hard decisions
    static double ber(const std::vector<float>& soft)
    {
        long errors = 0;
        for (const float v : soft)
            errors += v <= 0;
        return soft.empty() ? 1 : double(errors) / soft.size();
    }

    // Information per bit that the soft values give the Viterbi decoder
    // (1: certain, 0: nothing), with the best scaling of the soft values:
    // 1 - E[log2(1 + exp(-scale * v))]
    static double information(const std::vector<float>& soft)
    {
        double rms = 0;
        for (const float v : soft)
            rms += v * v;
        rms = std::sqrt(rms / soft.size());
        double best = 0;
        for (double scale = 0.05; scale < 20; scale *= 1.1) {
            double sum = 0;
            for (const float v : soft)
                sum += std::log2(1 + std::exp(-scale * v / rms));
            best = std::max(best, 1 - sum / soft.size());
        }
        return best;
    }

    std::vector<float> coherent;
    std::vector<float> differential;

private:
    Spectrum spectrum(int sym, const std::vector<int>& state)
    {
        Spectrum s(p.T_u, 0);
        std::normal_distribution<float> normal(0, sigma);
        const double t = timing + sym * c.timingDrift;
        const double ph = phase + sym * c.phaseDrift;
        for (size_t i = 0; i < carriers.size(); i++) {
            const int k = carriers[i];
            const double w = 2 * M_PI * k / p.T_u;
            const DSPCOMPLEX h = DSPCOMPLEX(1, 0) +
                std::polar(float(c.echoGain), float(-w * c.echoDelay));
            const double phi = table.get_Phi(k) + sym * M_PI / 4 + state[i] * M_PI / 2;
            s[(k + p.T_u) % p.T_u] = h * scramble[i] * std::polar(1.0f, float(phi + ph - w * t)) +
                DSPCOMPLEX(normal(rng), normal(rng));
        }
        return s;
    }

    const DABParams p;
    const Channel c;
    std::mt19937 rng;
    PhaseTable table;
    const float sigma;
    std::vector<int> carriers;
    std::vector<DSPCOMPLEX> scramble;
    double timing = 0;
    double phase = 0;
};

} // namespace

class CoherentDemodTests : public QObject
{
    Q_OBJECT

private slots:
    void decodesBits_data();
    void decodesBits();
    void moreSensitiveThanDifferential();
    void restartsWithNewChannel();
};

void CoherentDemodTests::decodesBits_data()
{
    QTest::addColumn<double>("echoGain");
    QTest::addColumn<double>("timingJump");
    QTest::addColumn<double>("timingDrift");
    QTest::addColumn<double>("phaseDrift");
    QTest::addColumn<bool>("newPhase");

    QTest::newRow("clean") << 0.0 << 0.0 << 0.0 << 0.0 << false;
    QTest::newRow("echo") << 0.5 << 0.0 << 0.0 << 0.0 << false;
    // The time synchronisation moves the FFT window by whole and partial
    // samples, the oscillator gives every frame another phase
    QTest::newRow("window and phase change every frame") << 0.5 << 1.3 << 0.0 << 0.0 << true;
    // 20 Hz frequency offset, sample clock error of 66 ppm (si_10d)
    QTest::newRow("frequency offset and clock error") << 0.5 << 0.0 << 0.17 << 0.16 << true;
}

void CoherentDemodTests::decodesBits()
{
    QFETCH(double, echoGain);
    QFETCH(double, timingJump);
    QFETCH(double, timingDrift);
    QFETCH(double, phaseDrift);
    QFETCH(bool, newPhase);

    Channel c;
    c.echoGain = echoGain;
    c.snrDb = 25;
    c.timingJump = timingJump;
    c.timingDrift = timingDrift;
    c.phaseDrift = phaseDrift;
    c.newPhase = newPhase;

    const DABParams p(1);
    CoherentDemodulator demod(p);
    FrameSource source(c, 908);
    // The clock error has to be learned first
    for (int i = 0; i < 5; i++)
        source.frame(demod, false);
    QVERIFY(demod.ready());
    for (int i = 0; i < 5; i++)
        source.frame(demod, true);

    QCOMPARE(FrameSource::ber(source.coherent), 0.0);
}

void CoherentDemodTests::moreSensitiveThanDifferential()
{
    // 2 dB SNR, an echo and a phase change in every frame
    Channel c;
    c.echoGain = 0.5;
    c.snrDb = 2;
    c.newPhase = true;

    const DABParams p(1);
    CoherentDemodulator demod(p);
    FrameSource source(c, 1);
    for (int i = 0; i < 5; i++)
        source.frame(demod, false);
    for (int i = 0; i < 10; i++)
        source.frame(demod, true);

    // The hard decisions are about equally bad (the coherent demodulation
    // decides the phases of two symbols), but the soft values carry more
    // information for the Viterbi decoder
    const double coherentInformation = FrameSource::information(source.coherent);
    const double differentialInformation = FrameSource::information(source.differential);
    qInfo("Bit errors: coherent %.3f, differential %.3f; information per bit: coherent %.3f, differential %.3f",
          FrameSource::ber(source.coherent), FrameSource::ber(source.differential),
          coherentInformation, differentialInformation);
    // 0.48 and 0.41 bit
    QVERIFY(coherentInformation > 1.1 * differentialInformation);
}

void CoherentDemodTests::restartsWithNewChannel()
{
    const DABParams p(1);
    CoherentDemodulator demod(p);
    QVERIFY(!demod.ready());

    Channel a;
    a.snrDb = 20;
    FrameSource first(a, 2);
    first.frame(demod, false);
    QVERIFY(!demod.ready());
    first.frame(demod, false);
    QVERIFY(demod.ready());

    // Another ensemble with the same power: its channel has nothing in
    // common with the average
    Channel b = a;
    b.scrambled = true;
    FrameSource second(b, 3);
    second.frame(demod, false);
    QVERIFY(!demod.ready());
    second.frame(demod, false);
    QVERIFY(demod.ready());

    demod.reset();
    QVERIFY(!demod.ready());
}

QTEST_GUILESS_MAIN(CoherentDemodTests)
#include "tst_coherent_demod.moc"
