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

// cli.recording.<clip>: black box test of welle-cli with every recording of
// manifest.json. welle-cli plays the recording with its web server
// ("welle-cli -f <clip> -w <port>"), and the ensemble and service list of
// /mux.json are compared with the expectations of the manifest. Audio is
// not decoded here, see tst_cli.cpp for that.

#include <QtTest>

#include <QElapsedTimer>

#include "expectations.h"
#include "welle_cli_process.h"

class CliRecordingTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void checkRecording_data();
    void checkRecording();

private:
    QJsonObject manifest;

    static CheckList checkMux(const QJsonObject& expect, const QJsonObject& mux);
};

void CliRecordingTests::initTestCase()
{
    QString missing = missingWelleCli();
    if (missing.isEmpty())
        missing = missingTestData();
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));

    manifest = loadManifest();
}

void CliRecordingTests::checkRecording_data()
{
    QTest::addColumn<QJsonObject>("clip");

    for (const auto& value : manifest["clips"].toArray()) {
        const auto clip = value.toObject();
        QTest::newRow(qPrintable(clip["id"].toString())) << clip;
    }
}

// Type and bitrate of a service like the backend tests: the first audio
// component with a valid subchannel, otherwise "data"
static void serviceType(const QJsonObject& service, QString& type, int& bitrate)
{
    type = "data";
    bitrate = 0;
    for (const auto& value : service["components"].toArray()) {
        const QJsonObject component = value.toObject();
        const QJsonObject subchannel = component["subchannel"].toObject();
        if (component["transportmode"].toString() != "audio" || subchannel["bitrate"].toInt() <= 0)
            continue;
        type = component["ascty"].toString();
        bitrate = subchannel["bitrate"].toInt();
        return;
    }
}

CheckList CliRecordingTests::checkMux(const QJsonObject& expect, const QJsonObject& mux)
{
    CheckList checks;

    checks.check(mux["demodulator"].toObject()["synced"].toBool(), "Sync");

    const QJsonObject ensemble = mux["ensemble"].toObject();
    const QString id = ensemble["id"].toString();
    const QString label = muxLabel(ensemble["label"]);
    const bool ensembleOk = id.compare(expect["ensemble_id"].toString(), Qt::CaseInsensitive) == 0 &&
        label == expect["ensemble_label"].toString();
    checks.check(ensembleOk, QString("Ensemble %1 \"%2\"%3").arg(id, label,
            ensembleOk ? "" : QString(" -- expected %1 \"%2\"")
                .arg(expect["ensemble_id"].toString(), expect["ensemble_label"].toString())));

    for (const auto& value : expect["services"].toArray()) {
        const QJsonObject es = value.toObject();
        const QString sid = es["sid"].toString();
        const QJsonObject service = muxService(mux, sid);
        if (service.isEmpty()) {
            checks.check(false, QString("%1 \"%2\" -- not in mux.json").arg(sid, es["label"].toString()));
            continue;
        }

        QString type;
        int bitrate;
        serviceType(service, type, bitrate);
        const QString serviceLabel = muxLabel(service["label"]);

        QStringList wrong;
        if (serviceLabel != es["label"].toString())
            wrong << QString("label, expected \"%1\"").arg(es["label"].toString());
        if (type != es["type"].toString())
            wrong << QString("type, expected %1").arg(es["type"].toString());
        if (es.contains("bitrate") && bitrate != es["bitrate"].toInt())
            wrong << QString("bitrate, expected %1").arg(es["bitrate"].toInt());

        checks.check(wrong.isEmpty(), QString("%1 \"%2\": %3, %4 kbit/s%5")
                .arg(sid, serviceLabel, type).arg(bitrate)
                .arg(wrong.isEmpty() ? "" : " -- " + wrong.join(", ")));
    }
    return checks;
}

void CliRecordingTests::checkRecording()
{
    QFETCH(QJsonObject, clip);

    const QString missing = missingClip(clip);
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));

    const QJsonObject expect = clip["expect"].toObject();
    const double duration = clip["duration_s"].toDouble(10);

    QStringList args;
    if (!clip["channel"].toString().isEmpty())
        args << "-c" << clip["channel"].toString();
    args << "-f" << clipPath(clip);

    QElapsedTimer elapsed;
    elapsed.start();

    WelleCli cli;
    QVERIFY2(cli.startWebServer(args), qPrintable("The web server did not start\n" + cli.errorTail()));

    CheckList checks;
    QJsonObject mux;
    if (expect.contains("ensemble_id")) {
        // The recording loops, so everything should be there after the first
        // pass. Poll until all checks pass, then show the last state.
        cli.pollMux([&](const QJsonObject& m) { return checkMux(expect, m).ok(); },
                    int((duration + 20) * 1000), &mux);
        checks = checkMux(expect, mux);
    }
    else {
        // Robustness recordings only have to survive one pass
        const bool synced = cli.pollMux([](const QJsonObject& m) {
            return m["demodulator"].toObject()["synced"].toBool(); }, int((duration + 20) * 1000), &mux);
        checks.check(synced, "Sync");
        while (cli.isRunning() && elapsed.elapsed() < (duration + 2) * 1000)
            QTest::qWait(500);
        checks.check(!cli.mux().isEmpty(), QString("mux.json still answers after %1 s").arg(duration + 2));
    }

    const QString problem = cli.healthProblem();
    checks.check(problem.isEmpty(), problem.isEmpty() ? "welle-cli still running" : problem);

    qInfo().noquote() << QString("%1: welle-cli %2\n%3")
        .arg(clip["id"].toString(), args.join(' '), checks.text());

    if (!checks.ok())
        QFAIL(qPrintable(checks.failures().join("\n   ") + "\n" + cli.errorTail()));
}

QTEST_GUILESS_MAIN(CliRecordingTests)

#include "tst_cli_recordings.moc"
