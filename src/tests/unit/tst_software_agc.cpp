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

// unit.software_agc: software AGC of the RTL-SDR, rtl_tcp and Airspy inputs
// with a simulated tuner. The signal is Gaussian noise (like OFDM) quantised
// to 8 bit I/Q samples, the gain table is the one of the R820T. The AGC must
// find a gain without clipping and then keep it instead of toggling
// between two steps (issue #908). Float samples are measured like 8 bit ones.

#include <QtTest>

#include <algorithm>
#include <cmath>
#include <complex>
#include <deque>
#include <random>
#include <vector>

#include "software_agc.h"

namespace {

const float highLevelDb = SoftwareAgc::defaultHighLevelDb;

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

    SoftwareAgc agc;
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

// Airspy Mini at 3 MS/s, float samples: level (dBFS per component, raw) at
// each gain index, measured on 11D and 5D (below index 6 the noise floor)
const std::vector<float> airspyLinearity11D = { -64.5, -64.5, -64.5, -64.5,
    -64.5, -64.5, -63.9, -63.1, -62.0, -60.3, -57.1, -52.8, -49.9, -45.7,
    -41.2, -37.4, -34.5, -31.5, -27.0, -22.2, -17.6, -14.7 };
const std::vector<float> airspyLinearity5D = { -64.6, -64.6, -64.6, -64.6,
    -64.6, -64.6, -64.6, -64.5, -64.6, -64.1, -62.5, -59.5, -57.1, -53.0,
    -48.7, -45.3, -42.4, -39.1, -34.7, -30.0, -25.3, -21.5 };
const std::vector<float> airspyHybrid11D = { -62.7, -59.8, -56.7, -53.5,
    -50.2, -46.8, -43.3, -40.0, -36.3, -32.8, -29.4, -25.8, -22.4, -18.8,
    -15.7, -13.2 };
const float airspyHighLevelDb = -25;

// Airspy and the AGC thread of its input: buffers of 16384 samples, a
// decision every 50 ms, a gain change takes effect after 100 ms
class AirspySimulation {
public:
    AirspySimulation(const std::vector<float>& levels, float stepDb) :
        agc(200, rate, airspyHighLevelDb),
        levels(levels),
        appliedIndex(levels.size() / 2),
        commandedIndex(levels.size() / 2),
        noise(1 << 20)
    {
        std::mt19937 rng(17);
        std::normal_distribution<float> normal;
        for (auto& n : noise)
            n = normal(rng);
        for (size_t i = 0; i < levels.size(); i++)
            table.push_back(i * stepDb);
    }

    static constexpr int rate = 3000000;
    static constexpr int bufferSamples = 16384;
    double offsetDb = 0;    // change of the input level
    SoftwareAgc agc;
    int changes = 0;

    // Runs the given time, returns the level (dBFS per component) of the
    // last 500 ms
    double run(double seconds)
    {
        const long buffers = seconds * rate / bufferSamples;
        const long keep = rate / 2 / bufferSamples;
        double power = 0;
        std::vector<std::complex<float>> buf(bufferSamples);

        for (long i = 0; i < buffers; i++) {
            while (!pending.empty() && pending.front().first <= samples) {
                appliedIndex = pending.front().second;
                pending.pop_front();
            }
            const float sigma = std::pow(10.0, (levels[appliedIndex] + offsetDb) / 20);
            double p = 0;
            for (auto& z : buf) {
                const float re = sigma * noise[noiseIndex++ & (noise.size() - 1)];
                const float im = sigma * noise[noiseIndex++ & (noise.size() - 1)];
                z = { re, im };
                p += std::norm(z) / 2;
            }
            noiseIndex += 7919;
            agc.process(buf.data(), buf.size());
            if (i >= buffers - keep)
                power += p / bufferSamples;

            const long before = samples;
            samples += bufferSamples;
            if (samples / (rate / 20) != before / (rate / 20)) {
                const int next = agc.nextGainIndex(table, commandedIndex);
                if (next != commandedIndex) {
                    commandedIndex = next;
                    pending.emplace_back(samples + rate / 10, next);
                    agc.gainChanged();
                    changes++;
                }
            }
        }
        return 10 * std::log10(power / keep);
    }

    int gainIndex() const { return commandedIndex; }

private:
    std::vector<float> levels;
    std::vector<float> table;
    int appliedIndex;
    int commandedIndex;
    long samples = 0;
    std::deque<std::pair<long, int>> pending;
    std::vector<float> noise;
    size_t noiseIndex = 0;
};

} // namespace

class SoftwareAgcTests : public QObject
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
    void measuresFloatSamples();
    void airspyFindsGainAndKeepsIt_data();
    void airspyFindsGainAndKeepsIt();
};

void SoftwareAgcTests::findsGainAndKeepsIt_data()
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

void SoftwareAgcTests::findsGainAndKeepsIt()
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
    QVERIFY2(level <= highLevelDb + 0.5,
             qPrintable(QString("level %1 dBFS too high").arg(level)));
    QVERIFY2(level >= highLevelDb - 6 || sim.gainIndex() == int(r820tGains.size()) - 1,
             qPrintable(QString("level %1 dBFS too low, gain %2 dB")
                        .arg(level).arg(r820tGains[sim.gainIndex()])));
    QVERIFY2(clippedRatio(samples) < SoftwareAgc::overloadRatio,
             qPrintable(QString("clipped %1").arg(clippedRatio(samples))));

    // Then the gain stays (the old AGC changed it several times a second)
    const int changesBefore = sim.changes;
    sim.run(20);
    QVERIFY2(sim.changes - changesBefore <= 1,
             qPrintable(QString("%1 gain changes in 20 s").arg(sim.changes - changesBefore)));
}

void SoftwareAgcTests::followsLevelChanges()
{
    Simulation sim(100, 20, r820tGains.size() / 2);
    sim.inputDb = -50;
    sim.run(4);

    // 20 dB stronger, e.g. another channel: backs off within 2 s
    sim.inputDb += 20;
    std::vector<uint8_t> samples = sim.run(2);
    QVERIFY2(levelDb(samples) <= highLevelDb + 0.5,
             qPrintable(QString("level %1 dBFS").arg(levelDb(samples))));
    QVERIFY(clippedRatio(samples) < SoftwareAgc::overloadRatio);

    // 25 dB weaker: back in the band within 3 s, despite the hold-off
    sim.inputDb -= 25;
    samples = sim.run(3);
    QVERIFY2(levelDb(samples) >= highLevelDb - 6,
             qPrintable(QString("level %1 dBFS").arg(levelDb(samples))));
}

void SoftwareAgcTests::toleratesInexactGainSteps()
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
    QVERIFY(clippedRatio(samples) < SoftwareAgc::overloadRatio);
}

void SoftwareAgcTests::learnsRealGainSteps_data()
{
    QTest::addColumn<double>("offsetDb");
    QTest::addColumn<int>("settleMs");
    QTest::addColumn<int>("delayMs");

    for (const double offset : { -3.0, -1.5, 0.0, 1.5 }) {
        QTest::newRow(qPrintable(QString("rtl-sdr %1 dB").arg(offset))) << offset << 100 << 20;
        QTest::newRow(qPrintable(QString("rtl_tcp %1 dB").arg(offset))) << offset << 500 << 300;
    }
}

void SoftwareAgcTests::learnsRealGainSteps()
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
    QVERIFY(clippedRatio(samples) < SoftwareAgc::overloadRatio);
    QVERIFY2(levelDb(samples) >= highLevelDb - 6,
             qPrintable(QString("level %1 dBFS").arg(levelDb(samples))));
}

void SoftwareAgcTests::reportsOverloadWithManualGain()
{
    std::vector<uint8_t> clipped(SoftwareAgc::windowMs * 4096 + bufferBytes);
    std::vector<uint8_t> normal(clipped.size());
    std::mt19937 rng(1);
    std::normal_distribution<float> normalDist(127.5f, 25.0f);
    for (size_t i = 0; i < clipped.size(); i++) {
        normal[i] = std::clamp<long>(std::lround(normalDist(rng)), 0, 255);
        clipped[i] = (i % 100 == 0) ? 255 : normal[i];
    }

    SoftwareAgc agc(0);
    agc.process(normal.data(), normal.size());
    QVERIFY(!agc.overloaded());

    agc.process(clipped.data(), clipped.size());
    QVERIFY(agc.overloaded());

    // Each window is reported once
    QVERIFY(!agc.overloaded());
}

void SoftwareAgcTests::measuresFloatSamples()
{
    // The same Gaussian signal (-20 dBFS per component) as float samples and
    // as 8 bit samples gives the same level
    const size_t n = SoftwareAgc::windowMs * 2048 + 4096;
    const float sigma = std::pow(10.0f, -20.0f / 20);
    std::mt19937 rng(3);
    std::normal_distribution<float> normal(0, sigma);
    std::vector<std::complex<float>> samples(n);
    std::vector<uint8_t> bytes(2 * n);
    for (size_t i = 0; i < n; i++) {
        samples[i] = { normal(rng), normal(rng) };
        bytes[2 * i] = std::clamp<long>(std::lround(127.5f + 128 * samples[i].real()), 0, 255);
        bytes[2 * i + 1] = std::clamp<long>(std::lround(127.5f + 128 * samples[i].imag()), 0, 255);
    }

    const std::vector<float> gains = { 0, 10, 20 };
    SoftwareAgc floatAgc(0);
    floatAgc.process(samples.data(), samples.size());
    QCOMPARE(floatAgc.nextGainIndex(gains, 1), 1);
    SoftwareAgc byteAgc(0);
    byteAgc.process(bytes.data(), bytes.size());
    QCOMPARE(byteAgc.nextGainIndex(gains, 1), 1);
    QVERIFY2(std::abs(floatAgc.lastLevelDb() + 20) < 0.1,
             qPrintable(QString("float %1 dBFS").arg(floatAgc.lastLevelDb())));
    QVERIFY2(std::abs(byteAgc.lastLevelDb() - floatAgc.lastLevelDb()) < 0.2,
             qPrintable(QString("8 bit %1 dBFS").arg(byteAgc.lastLevelDb())));

    // Float values at full scale count as clipped
    for (size_t i = 0; i < n; i += 100)
        samples[i] = { 1.0f, 0.0f };
    floatAgc.process(samples.data(), samples.size());
    QVERIFY(floatAgc.overloaded());
}

void SoftwareAgcTests::airspyFindsGainAndKeepsIt_data()
{
    QTest::addColumn<std::vector<float>>("levels");
    QTest::addColumn<float>("stepDb");
    QTest::addColumn<double>("offsetDb");

    QTest::newRow("linearity 11D") << airspyLinearity11D << 3.5f << 0.0;
    QTest::newRow("linearity 5D") << airspyLinearity5D << 3.5f << 0.0;
    QTest::newRow("hybrid 11D") << airspyHybrid11D << 3.4f << 0.0;
    QTest::newRow("linearity 11D, 3 dB weaker") << airspyLinearity11D << 3.5f << -3.0;
}

void SoftwareAgcTests::airspyFindsGainAndKeepsIt()
{
    QFETCH(std::vector<float>, levels);
    QFETCH(float, stepDb);
    QFETCH(double, offsetDb);

    AirspySimulation sim(levels, stepDb);
    sim.offsetDb = offsetDb;

    // Within 6 s in the target range: at most one step and the hysteresis
    // below the high level (the SNR doesn't improve above about -40 dBFS)
    double level = sim.run(6);
    QVERIFY2(level <= airspyHighLevelDb + 0.5 && level >= airspyHighLevelDb - 7,
             qPrintable(QString("level %1 dBFS at gain %2").arg(level).arg(sim.gainIndex())));

    // Then the gain stays
    const int changesBefore = sim.changes;
    level = sim.run(30);
    QVERIFY2(sim.changes - changesBefore <= 1,
             qPrintable(QString("%1 gain changes in 30 s").arg(sim.changes - changesBefore)));
    QVERIFY2(level <= airspyHighLevelDb + 0.5 && level >= airspyHighLevelDb - 7,
             qPrintable(QString("level %1 dBFS at gain %2").arg(level).arg(sim.gainIndex())));
}

QTEST_GUILESS_MAIN(SoftwareAgcTests)
#include "tst_software_agc.moc"
