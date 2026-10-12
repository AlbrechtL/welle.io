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

// unit.rtl_agc: software AGC of the RTL-SDR and rtl_tcp inputs with a
// simulated tuner. The signal is Gaussian noise (like OFDM) quantised to
// 8 bit I/Q samples, the gain table is the one of the R820T. The AGC must
// find a gain without clipping and then keep it instead of toggling
// between two steps (issue #908).

#include <QtTest>

#include <algorithm>
#include <cmath>
#include <deque>
#include <random>
#include <vector>

#include "rtl_agc.h"

namespace {

const std::vector<float> r820tGains = { 0.0, 0.9, 1.4, 2.7, 3.7, 7.7,
    8.7, 12.5, 14.4, 15.7, 16.6, 19.7, 20.7, 22.9, 25.4, 28.0, 29.7, 32.8,
    33.8, 36.4, 37.2, 38.6, 40.2, 42.1, 43.4, 43.9, 44.5, 48.0, 49.6 };

// Level (dBFS) of a real R820T on 5C (rtl_tcp) at each gain of the table:
// the real steps differ from the table, e.g. 43.4 -> 43.9 dB is 2.6 dB
const std::vector<float> r820tLevels5C = { -48.13, -47.97, -47.81, -47.00,
    -46.49, -45.03, -43.69, -42.89, -41.43, -40.27, -39.31, -36.21, -35.25,
    -32.58, -31.01, -28.10, -26.45, -23.63, -22.00, -20.28, -19.37, -18.08,
    -16.89, -16.24, -14.75, -12.15, -11.72, -10.28, -9.57 };

// 2 ms at 2.048 MS/s, as delivered by rtl-sdr
const size_t bufferBytes = 8192;
const int buffersPerSecond = 2048000 * 2 / bufferBytes;
// The AGC threads of the inputs run every 50 ms
const int buffersPerTick = buffersPerSecond / 20;

double levelDb(const std::vector<uint8_t>& bytes)
{
    double sum = 0;
    double squares = 0;
    for (const uint8_t b : bytes) {
        const int v = int(b) - 128;
        sum += v;
        squares += v * v;
    }
    const double mean = sum / bytes.size();
    return 10 * std::log10((squares / bytes.size() - mean * mean) / (128.0 * 128.0));
}

double clippedRatio(const std::vector<uint8_t>& bytes)
{
    const auto clipped = std::count_if(bytes.begin(), bytes.end(),
            [](uint8_t b) { return b == 0 || b == 255; });
    return double(clipped) / bytes.size();
}

// Tuner, ADC and AGC thread of an input
class Simulation {
public:
    Simulation(int settleMs, int delayMs, int startIndex) :
        agc(settleMs),
        delayBuffers(delayMs * buffersPerSecond / 1000),
        appliedIndex(startIndex),
        commandedIndex(startIndex),
        noise(1 << 20)
    {
        std::mt19937 rng(908);
        std::normal_distribution<float> normal;
        for (auto& n : noise)
            n = normal(rng);
    }

    // Signal level at 0 dB tuner gain, in dB relative to full scale
    double inputDb = -40;
    // Real gain = table gain * gainError (tuners don't match their tables)
    double gainError = 1.0;
    // Real gains instead of the table, if not empty
    std::vector<float> realGains;
    // Slow fluctuation of the input level (period 5 s), like a real signal
    double fluctuationDb = 0;

    RtlAgc agc;
    int changes = 0;

    // Runs the given time, returns the samples of the last 500 ms
    std::vector<uint8_t> run(double seconds)
    {
        const int buffers = seconds * buffersPerSecond;
        const int keep = buffersPerSecond / 2;
        std::vector<uint8_t> last;
        last.reserve(keep * bufferBytes);
        std::vector<uint8_t> buf(bufferBytes);

        for (int i = 0; i < buffers; i++) {
            while (!pending.empty() && pending.front().first <= bufferCount) {
                appliedIndex = pending.front().second;
                pending.pop_front();
            }

            const double gainDb = realGains.empty() ?
                r820tGains[appliedIndex] * gainError : realGains[appliedIndex];
            const double t = double(bufferCount) / buffersPerSecond;
            const double fluctuation = fluctuationDb * std::sin(2 * M_PI * t / 5);
            const float amplitude = 128 * std::pow(10.0, (inputDb + fluctuation + gainDb) / 20);
            for (auto& b : buf) {
                const float v = 127.5f + amplitude * noise[noiseIndex++ & (noise.size() - 1)];
                b = std::clamp<long>(std::lround(v), 0, 255);
            }
            noiseIndex += 7919;  // no repeating pattern

            agc.process(buf.data(), buf.size());
            if (i >= buffers - keep)
                last.insert(last.end(), buf.begin(), buf.end());

            if (++bufferCount % buffersPerTick == 0) {
                const int next = agc.nextGainIndex(r820tGains, commandedIndex);
                if (next != commandedIndex) {
                    // What setGain() of the inputs does
                    commandedIndex = next;
                    pending.emplace_back(bufferCount + delayBuffers, next);
                    agc.gainChanged();
                    changes++;
                }
            }
        }
        return last;
    }

    int gainIndex() const { return commandedIndex; }

private:
    const int delayBuffers;
    int appliedIndex;
    int commandedIndex;
    long bufferCount = 0;
    std::deque<std::pair<long, int>> pending;
    std::vector<float> noise;
    size_t noiseIndex = 0;
};

} // namespace

class RtlAgcTests : public QObject
{
    Q_OBJECT

private slots:
    void findsGainAndKeepsIt_data();
    void findsGainAndKeepsIt();
    void followsLevelChanges();
    void toleratesInexactGainSteps();
    void learnsRealGainSteps_data();
    void learnsRealGainSteps();
    void reportsOverloadWithManualGain();
};

void RtlAgcTests::findsGainAndKeepsIt_data()
{
    QTest::addColumn<double>("inputDb");
    QTest::addColumn<int>("settleMs");
    QTest::addColumn<int>("delayMs");

    // rtl-sdr: the gain is set directly
    QTest::newRow("weak rtl-sdr") << -70.0 << 100 << 20;
    QTest::newRow("medium rtl-sdr") << -45.0 << 100 << 20;
    QTest::newRow("strong rtl-sdr") << -16.0 << 100 << 20;
    // rtl_tcp: the gain reaches the samples later
    QTest::newRow("medium rtl_tcp") << -45.0 << 500 << 300;
    QTest::newRow("strong rtl_tcp") << -16.0 << 500 << 300;
}

void RtlAgcTests::findsGainAndKeepsIt()
{
    QFETCH(double, inputDb);
    QFETCH(int, settleMs);
    QFETCH(int, delayMs);

    // The inputs start in the middle of the gain table
    Simulation sim(settleMs, delayMs, r820tGains.size() / 2);
    sim.inputDb = inputDb;

    // Converged within 4 s
    std::vector<uint8_t> samples = sim.run(4);
    const double level = levelDb(samples);
    QVERIFY2(level <= RtlAgc::highLevelDb + 0.5,
             qPrintable(QString("level %1 dBFS too high").arg(level)));
    QVERIFY2(level >= RtlAgc::highLevelDb - 6 || sim.gainIndex() == int(r820tGains.size()) - 1,
             qPrintable(QString("level %1 dBFS too low, gain %2 dB")
                        .arg(level).arg(r820tGains[sim.gainIndex()])));
    QVERIFY2(clippedRatio(samples) < RtlAgc::overloadRatio,
             qPrintable(QString("clipped %1").arg(clippedRatio(samples))));

    // Then the gain stays (the old AGC changed it several times a second)
    const int changesBefore = sim.changes;
    sim.run(20);
    QVERIFY2(sim.changes - changesBefore <= 1,
             qPrintable(QString("%1 gain changes in 20 s").arg(sim.changes - changesBefore)));
}

void RtlAgcTests::followsLevelChanges()
{
    Simulation sim(100, 20, r820tGains.size() / 2);
    sim.inputDb = -50;
    sim.run(4);

    // 20 dB stronger, e.g. another channel: backs off within 2 s
    sim.inputDb += 20;
    std::vector<uint8_t> samples = sim.run(2);
    QVERIFY2(levelDb(samples) <= RtlAgc::highLevelDb + 0.5,
             qPrintable(QString("level %1 dBFS").arg(levelDb(samples))));
    QVERIFY(clippedRatio(samples) < RtlAgc::overloadRatio);

    // 25 dB weaker: back in the band within 3 s, despite the hold-off
    sim.inputDb -= 25;
    samples = sim.run(3);
    QVERIFY2(levelDb(samples) >= RtlAgc::highLevelDb - 6,
             qPrintable(QString("level %1 dBFS").arg(levelDb(samples))));
}

void RtlAgcTests::toleratesInexactGainSteps()
{
    // The real steps are 20 % larger than in the table
    Simulation sim(100, 20, r820tGains.size() / 2);
    sim.gainError = 1.2;
    sim.inputDb = -45;
    sim.run(4);

    const int changesBefore = sim.changes;
    const std::vector<uint8_t> samples = sim.run(20);
    QVERIFY2(sim.changes - changesBefore <= 2,
             qPrintable(QString("%1 gain changes in 20 s").arg(sim.changes - changesBefore)));
    QVERIFY(clippedRatio(samples) < RtlAgc::overloadRatio);
}

void RtlAgcTests::learnsRealGainSteps_data()
{
    QTest::addColumn<double>("offsetDb");
    QTest::addColumn<int>("settleMs");
    QTest::addColumn<int>("delayMs");

    for (const double offset : { -3.0, -1.5, 0.0, 1.5 }) {
        QTest::newRow(qPrintable(QString("rtl-sdr %1 dB").arg(offset))) << offset << 100 << 20;
        QTest::newRow(qPrintable(QString("rtl_tcp %1 dB").arg(offset))) << offset << 500 << 300;
    }
}

void RtlAgcTests::learnsRealGainSteps()
{
    QFETCH(double, offsetDb);
    QFETCH(int, settleMs);
    QFETCH(int, delayMs);

    // The tuner measured on 5C, at the same and at slightly other levels
    Simulation sim(settleMs, delayMs, r820tGains.size() / 2);
    sim.realGains = r820tLevels5C;
    sim.inputDb = offsetDb;
    sim.fluctuationDb = 0.5;
    sim.run(6);

    const int changesBefore = sim.changes;
    const std::vector<uint8_t> samples = sim.run(30);
    QVERIFY2(sim.changes - changesBefore <= 1,
             qPrintable(QString("%1 gain changes in 30 s, gain %2 dB")
                        .arg(sim.changes - changesBefore).arg(r820tGains[sim.gainIndex()])));
    QVERIFY(clippedRatio(samples) < RtlAgc::overloadRatio);
    QVERIFY2(levelDb(samples) >= RtlAgc::highLevelDb - 6,
             qPrintable(QString("level %1 dBFS").arg(levelDb(samples))));
}

void RtlAgcTests::reportsOverloadWithManualGain()
{
    std::vector<uint8_t> clipped(RtlAgc::windowMs * 4096 + bufferBytes);
    std::vector<uint8_t> normal(clipped.size());
    std::mt19937 rng(1);
    std::normal_distribution<float> normalDist(127.5f, 25.0f);
    for (size_t i = 0; i < clipped.size(); i++) {
        normal[i] = std::clamp<long>(std::lround(normalDist(rng)), 0, 255);
        clipped[i] = (i % 100 == 0) ? 255 : normal[i];
    }

    RtlAgc agc(0);
    agc.process(normal.data(), normal.size());
    QVERIFY(!agc.overloaded());

    agc.process(clipped.data(), clipped.size());
    QVERIFY(agc.overloaded());

    // Each window is reported once
    QVERIFY(!agc.overloaded());
}

QTEST_GUILESS_MAIN(RtlAgcTests)
#include "tst_rtl_agc.moc"
