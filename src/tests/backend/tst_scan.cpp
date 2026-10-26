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

// backend.scan: the decision of the channel scan (RadioReceiver::restart(true),
// onSignalPresence()). After a retune, rtl_tcp still delivers up to about
// 150 ms of the previous channel. If that channel is much stronger, the
// level reference of the null symbol search is far too high and every
// attempt fails within a few ms. A weak channel after a strong one (10A
// after 9D) was missed by the scan. Here the recording odr_dabmod follows
// 150 ms of noise 18 dB above its level. The scan must find the signal, and
// it must still decide quickly on noise only.

#include <QtTest>

#include <atomic>
#include <chrono>
#include <cmath>
#include <random>
#include <thread>

#include "expectations.h"
#include "null_radio_controller.h"
#include "radio-receiver.h"
#include "raw_file.h"

namespace {

class PresenceController : public NullRadioController {
public:
    std::atomic<int> presence{-1};
    std::atomic<int64_t> decidedAt{0};
    std::function<int64_t()> position;

    void onSignalPresence(bool isSignal) override
    {
        decidedAt = position();
        presence = isSignal ? 1 : 0;
    }
};

// The first leadIn samples are Gaussian noise with the given RMS amplitude,
// then the samples of the wrapped input follow (if any)
class LeadInInput : public InputInterface {
public:
    LeadInInput(InputInterface* input, int64_t leadIn, float rms) :
        input(input), leadIn(leadIn), sigma(rms / std::sqrt(2.0f)) {}

    void setFrequency(int) override {}
    int getFrequency() const override { return 0; }
    bool is_ok() override { return true; }
    bool restart() override { return input ? input->restart() : true; }
    void stop() override { if (input) input->stop(); }
    void reset() override {}
    int32_t getSamples(DSPCOMPLEX* buffer, int32_t size) override
    {
        int32_t n = 0;
        while (n < size && position < leadIn) {
            buffer[n++] = DSPCOMPLEX(sigma * normal(rng), sigma * normal(rng));
            position++;
        }
        if (n < size && input) {
            const int32_t read = input->getSamples(buffer + n, size - n);
            n += read;
            position += read;
        }
        return n;
    }
    std::vector<DSPCOMPLEX> getSpectrumSamples(int) override { return {}; }
    int32_t getSamplesToRead() override
    {
        const int64_t noise = std::max<int64_t>(0, std::min<int64_t>(leadIn - position, 1 << 20));
        return int32_t(noise) + (input ? input->getSamplesToRead() : 0);
    }
    float setGain(int) override { return 0; }
    float getGain() const override { return 0; }
    int getGainCount() override { return 0; }
    void setAgc(bool) override {}
    std::string getDescription() override { return "lead-in"; }

    std::atomic<int64_t> position{0};

private:
    InputInterface* input;
    const int64_t leadIn;
    const float sigma;
    std::mt19937 rng{102};
    std::normal_distribution<float> normal;
};

} // namespace

class ScanTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void findsSignal_data();
    void findsSignal();
    void decidesQuicklyOnNoise();

private:
    QString path;
    float clipRms = 0;
};

void ScanTests::initTestCase()
{
    hideDebugOutput();

    const QString missing = missingTestData();
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));

    for (const auto& value : loadManifest()["clips"].toArray()) {
        const auto clip = value.toObject();
        if (clip["id"].toString() == "odr_dabmod") {
            const QString missingOdr = missingClip(clip);
            if (!missingOdr.isEmpty())
                QSKIP(qPrintable(missingOdr));
            path = clipPath(clip);
        }
    }
    if (path.isEmpty())
        QSKIP("odr_dabmod is not in the manifest");

    // Level of the recording (first second)
    NullRadioController rc;
    CRAWFile rawFile(rc, false, false);
    rawFile.setFileName(path.toStdString(), "auto");
    QVERIFY(rawFile.is_ok());
    rawFile.restart();
    // getSamples() waits until the ring buffer holds the requested samples
    std::vector<DSPCOMPLEX> buffer(INPUT_RATE);
    int32_t read = 0;
    while (read < INPUT_RATE)
        read += rawFile.getSamples(buffer.data() + read, std::min(8192, INPUT_RATE - read));
    double power = 0;
    for (int32_t i = 0; i < read; i++)
        power += std::norm(buffer[i]);
    clipRms = std::sqrt(power / read);
    rawFile.stop();
}

void ScanTests::findsSignal_data()
{
    QTest::addColumn<int>("leadInMs");
    QTest::addColumn<double>("leadInDb");

    QTest::newRow("recording") << 0 << 0.0;
    // Before: "no signal" after 5 failed searches for the end of the null
    // symbol, within 20 ms of the recording
    QTest::newRow("150 ms 18 dB stronger before") << 150 << 18.0;
}

void ScanTests::findsSignal()
{
    QFETCH(int, leadInMs);
    QFETCH(double, leadInDb);

    PresenceController rc;
    CRAWFile rawFile(rc, false, false);
    rawFile.setFileName(path.toStdString(), "auto");
    QVERIFY(rawFile.is_ok());

    LeadInInput input(&rawFile, int64_t(leadInMs) * INPUT_RATE / 1000,
                      clipRms * std::pow(10.0f, float(leadInDb) / 20));
    rc.position = [&]() { return input.position.load(); };

    RadioReceiver receiver(rc, input, RadioReceiverOptions());
    receiver.restart(true);

    for (int i = 0; i < 2000 && rc.presence < 0; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    receiver.stop();

    QVERIFY2(rc.presence >= 0, "No scan decision");
    const double decidedMs = 1000.0 * rc.decidedAt / INPUT_RATE;
    QVERIFY2(rc.presence == 1,
             qPrintable(QString("No signal found, decided after %1 ms").arg(decidedMs)));
    qInfo() << "Signal found after" << decidedMs << "ms";
}

void ScanTests::decidesQuicklyOnNoise()
{
    PresenceController rc;
    LeadInInput input(nullptr, int64_t(1) << 40, clipRms);
    rc.position = [&]() { return input.position.load(); };

    RadioReceiver receiver(rc, input, RadioReceiverOptions());
    receiver.restart(true);

    for (int i = 0; i < 2000 && rc.presence < 0; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    receiver.stop();

    QVERIFY2(rc.presence >= 0, "No scan decision");
    QCOMPARE(int(rc.presence), 0);
    // An empty channel must not slow the scan down: 5 frames (0.48 s) and
    // the search that is running then
    const double decidedMs = 1000.0 * rc.decidedAt / INPUT_RATE;
    QVERIFY2(decidedMs < 700, qPrintable(QString("Decided after %1 ms").arg(decidedMs)));
    qInfo() << "No signal after" << decidedMs << "ms";
}

QTEST_GUILESS_MAIN(ScanTests)
#include "tst_scan.moc"
