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

// Compares the differential and the coherent demodulation on the recordings
// of the test data (WELLE_TEST_DATA_DIR): every clip of the manifest, and
// SNR sweeps of some clips through the channel model (noise, an echo, an
// echo with Doppler shift). Writes one JSON file with the results of
// welle-test-probe for every run.
//
//   demod_compare <output.json> [--jobs N] [--clips-only | --sweeps-only]
//
// Every run is a process of its own (FFTW plans can't be created in
// parallel threads), N of them at the same time:
//
//   demod_compare --run <clip id> <differential|coherent> <snr dB>
//                 <echo gain> <echo delay> <echo Doppler Hz>

#include <algorithm>
#include <iostream>
#include <string>
#include <thread>

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QQueue>

#include "expectations.h"
#include "recording_runner.h"

namespace {

struct Job {
    QString clip;
    QString method;
    double snrDb = 100;
    double echoGain = 0;
    int echoDelay = 0;
    double dopplerHz = 0;
    QString group;      // "clips" or the name of a sweep
};

// Clips and channels of the SNR sweeps
const char* const sweepClips[] = {
    "odr_dabmod", "fr_paris_6b_2026", "fr_metz_8a_2026", "uk_11d_2020" };

struct SweepChannel {
    const char* name;
    double echoGain;
    int echoDelay;
    double dopplerHz;
};
// Echo 0.9 after 20 us (41 samples): a notch every 50 kHz
const SweepChannel sweepChannels[] = {
    { "noise", 0, 0, 0 },
    { "echo", 0.9, 41, 0 },
    { "echo, Doppler 5 Hz", 0.9, 41, 5 },
    { "echo, Doppler 20 Hz", 0.9, 41, 20 },
    { "echo, Doppler 50 Hz", 0.9, 41, 50 },
};

int runOne(const QStringList& args)
{
    hideDebugOutput();

    QJsonObject clip;
    for (const auto& value : loadManifest()["clips"].toArray()) {
        if (value.toObject()["id"].toString() == args[0])
            clip = value.toObject();
    }
    if (clip.isEmpty() || !missingClip(clip).isEmpty()) {
        std::cerr << "Clip " << args[0].toStdString() << " not available" << std::endl;
        return 2;
    }

    RunOptions options;
    options.timeoutSeconds = 300;
    options.receiverOptions.demodulationMethod = args[1] == "coherent" ?
        DemodulationMethod::Coherent : DemodulationMethod::Differential;
    options.channel.snrDb = args[2].toDouble();
    options.channel.echoGain = args[3].toDouble();
    options.channel.echoDelay = args[4].toInt();
    options.channel.echoDopplerHz = args[5].toDouble();

    const RunResult result = runRecording(clipPath(clip).toStdString(), "auto", options);
    std::cout << QJsonDocument(toJson(result)).toJson(QJsonDocument::Compact).toStdString()
              << std::endl;
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments().mid(1);

    if (args.size() == 7 && args[0] == "--run")
        return runOne(args.mid(1));

    if (args.isEmpty() || args[0].startsWith("--")) {
        std::cerr << "Usage: " << argv[0]
                  << " <output.json> [--jobs N] [--clips-only | --sweeps-only]" << std::endl;
        return 2;
    }
    const QString output = args.takeFirst();
    int jobs = std::max(1u, std::thread::hardware_concurrency() / 2);
    bool clips = true;
    bool sweeps = true;
    while (!args.isEmpty()) {
        const QString a = args.takeFirst();
        if (a == "--jobs" && !args.isEmpty())
            jobs = std::max(1, args.takeFirst().toInt());
        else if (a == "--clips-only")
            sweeps = false;
        else if (a == "--sweeps-only")
            clips = false;
        else {
            std::cerr << "Unknown option " << a.toStdString() << std::endl;
            return 2;
        }
    }

    const QString missing = missingTestData();
    if (!missing.isEmpty()) {
        std::cerr << missing.toStdString() << std::endl;
        return 2;
    }
    const QJsonObject manifest = loadManifest();

    QQueue<Job> queue;
    QJsonArray clipInfo;
    for (const auto& value : manifest["clips"].toArray()) {
        const auto clip = value.toObject();
        if (!missingClip(clip).isEmpty())
            continue;
        const QString id = clip["id"].toString();
        clipInfo.append(QJsonObject{
                {"id", id},
                {"description", clip["description"]},
                {"tags", clip["tags"]}});
        for (const char* method : { "differential", "coherent" }) {
            if (clips)
                queue.enqueue(Job{id, method, 100, 0, 0, 0, "clips"});
            if (!sweeps || std::find(std::begin(sweepClips), std::end(sweepClips), id) == std::end(sweepClips))
                continue;
            for (const auto& channel : sweepChannels) {
                for (int i = 0; i <= 20; i++) {
                    queue.enqueue(Job{id, method, i * 0.5, channel.echoGain,
                            channel.echoDelay, channel.dopplerHz, channel.name});
                }
            }
        }
    }

    const int total = queue.size();
    QJsonArray results;
    QList<QPair<QProcess*, Job>> running;
    while (!queue.isEmpty() || !running.isEmpty()) {
        while (!queue.isEmpty() && running.size() < jobs) {
            const Job job = queue.dequeue();
            auto* process = new QProcess;
            process->setProcessChannelMode(QProcess::SeparateChannels);
            process->start(app.applicationFilePath(), {
                    "--run", job.clip, job.method, QString::number(job.snrDb),
                    QString::number(job.echoGain), QString::number(job.echoDelay),
                    QString::number(job.dopplerHz)});
            running.append({process, job});
        }

        for (int i = 0; i < running.size(); i++) {
            auto* process = running[i].first;
            if (!process->waitForFinished(50))
                continue;
            const Job job = running[i].second;
            const auto doc = QJsonDocument::fromJson(process->readAllStandardOutput());
            results.append(QJsonObject{
                    {"group", job.group},
                    {"clip", job.clip},
                    {"method", job.method},
                    {"snr_db", job.snrDb},
                    {"echo_gain", job.echoGain},
                    {"echo_delay", job.echoDelay},
                    {"doppler_hz", job.dopplerHz},
                    {"result", doc.object()}});
            delete process;
            running.removeAt(i--);
            std::cerr << "\r" << results.size() << "/" << total << std::flush;
        }
    }
    std::cerr << std::endl;

    QFile file(output);
    if (!file.open(QIODevice::WriteOnly)) {
        std::cerr << "Cannot write " << output.toStdString() << std::endl;
        return 1;
    }
    file.write(QJsonDocument(QJsonObject{
            {"clips", clipInfo},
            {"runs", results}}).toJson(QJsonDocument::Indented));
    return 0;
}
