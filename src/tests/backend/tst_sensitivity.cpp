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

// backend.sensitivity: the synthetic recording odr_dabmod through a radio
// channel with noise and an echo (channel_model.h). The FIB errors and the
// erroneous audio frames of the MP2 and the DAB+ service must stay below
// limits. The echo gives notches in the spectrum: the soft bits of the
// carriers in the notches must not be trusted like the others (issue #908).

#include <QtTest>

#include "expectations.h"
#include "recording_runner.h"

class SensitivityTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void decodesThroughChannel_data();
    void decodesThroughChannel();

private:
    QString path;
};

void SensitivityTests::initTestCase()
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
}

void SensitivityTests::decodesThroughChannel_data()
{
    QTest::addColumn<double>("snrDb");
    QTest::addColumn<double>("echoGain");
    QTest::addColumn<int>("echoDelay");
    // Limits in percent
    QTest::addColumn<double>("maxFibErrors");
    QTest::addColumn<double>("maxFrameErrors");

    // Echo of 0.9 after 20 us: a notch every 50 kHz (50 carriers). With soft
    // bits without amplitude information: 1.1 % FIB errors, 24 % MP2 frame
    // errors; with: 0 %, 0.7 % and 0.5 % (DAB+).
    QTest::newRow("echo 0.9 20us, 10 dB") << 10.0 << 0.9 << 41 << 0.2 << 2.0;
    // Before 0.54 % FIB and 3.8 % MP2 frame errors, now 0.13 % and 1.2 %
    QTest::newRow("noise 4 dB") << 4.0 << 0.0 << 0 << 0.4 << 3.0;
}

void SensitivityTests::decodesThroughChannel()
{
    QFETCH(double, snrDb);
    QFETCH(double, echoGain);
    QFETCH(int, echoDelay);
    QFETCH(double, maxFibErrors);
    QFETCH(double, maxFrameErrors);

    RunOptions options;
    options.timeoutSeconds = 120;
    options.channel.snrDb = snrDb;
    options.channel.echoGain = echoGain;
    options.channel.echoDelay = echoDelay;
    const RunResult result = runRecording(path.toStdString(), "auto", options);

    const int fibs = result.fibOk + result.fibFailed;
    const double fibErrors = fibs > 0 ? 100.0 * result.fibFailed / fibs : 100;
    QString report = QString("FIB errors %1 % (max %2 %)")
        .arg(fibErrors, 0, 'f', 2).arg(maxFibErrors);
    bool ok = fibErrors <= maxFibErrors;

    int audioServices = 0;
    for (const auto& s : result.services) {
        if (s.type != "DAB" && s.type != "DAB+")
            continue;
        audioServices++;
        const double frameErrors = s.frameCount > 0 ? 100.0 * s.frameErrors / s.frameCount : 100;
        report += QString("\n   %1 (%2): frame errors %3 % (max %4 %), %5 s audio")
            .arg(s.label.c_str(), s.type.c_str())
            .arg(frameErrors, 0, 'f', 2).arg(maxFrameErrors)
            .arg(s.audioSeconds, 0, 'f', 1);
        ok = ok && frameErrors <= maxFrameErrors;
    }

    qInfo().noquote() << report;
    QCOMPARE(audioServices, 2);
    QVERIFY2(ok, qPrintable(report));
}

QTEST_GUILESS_MAIN(SensitivityTests)

#include "tst_sensitivity.moc"
