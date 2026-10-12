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

#include "expectations.h"

#include <cstdio>

#include <QDir>
#include <QLoggingCategory>
#include <QFile>
#include <QJsonDocument>

QString testDataDir()
{
    const QString env = qEnvironmentVariable("WELLE_TEST_DATA_DIR");
    if (!env.isEmpty())
        return env;
    return QStringLiteral(WELLE_TEST_DATA_DIR_DEFAULT);
}

void hideDebugOutput()
{
    if (qEnvironmentVariableIsSet("WELLE_TEST_VERBOSE"))
        return;

    QLoggingCategory::setFilterRules("*.debug=false\nqt.*.info=false");

    // The backend writes to stderr directly, QtTest writes to stdout
    fflush(stderr);
#ifdef Q_OS_WIN
    const char* nullDevice = "NUL";
#else
    const char* nullDevice = "/dev/null";
#endif
    if (!freopen(nullDevice, "w", stderr))
        qWarning("Cannot hide the backend log");
}

static QString manifestPath()
{
    return QDir(testDataDir()).filePath("manifest.json");
}

QString missingTestData()
{
    if (testDataDir().isEmpty())
        return "WELLE_TEST_DATA_DIR is not set, see src/tests/README.md";
    if (!QFile::exists(manifestPath()))
        return "No test data repository in " + testDataDir() + ", see src/tests/README.md";
    return QString();
}

QJsonObject loadManifest()
{
    QFile f(manifestPath());
    if (!f.open(QIODevice::ReadOnly))
        qFatal("Cannot open %s", qPrintable(manifestPath()));

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(f.readAll(), &error);
    if (doc.isNull())
        qFatal("Cannot parse %s: %s", qPrintable(manifestPath()), qPrintable(error.errorString()));
    return doc.object();
}

QString clipPath(const QJsonObject& clip)
{
    return QDir(testDataDir()).filePath(
            "clips/" + clip["id"].toString() + "." + clip["format"].toString() + ".iq");
}

QString missingClip(const QJsonObject& clip)
{
    if (QFile::exists(clipPath(clip)))
        return QString();
    return "Missing " + clipPath(clip) +
        ", run scripts/prepare_test_data.py --unpack in the test data repository";
}

// MP2 encoders switch between stereo and joint stereo from frame to frame
static QString normalizedMode(QString mode)
{
    return mode.replace("Joint Stereo", "Stereo");
}

static QString hex(uint32_t value, int width)
{
    return QString("0x%1").arg(value, width, 16, QChar('0'));
}

bool CheckReport::ok() const
{
    for (const auto& item : items) {
        if (!item.ok)
            return false;
    }
    return true;
}

QStringList CheckReport::failures() const
{
    QStringList list;
    for (const auto& item : items) {
        if (!item.ok)
            list << item.text;
    }
    return list;
}

QString CheckReport::text() const
{
    QStringList lines;
    for (const auto& item : items)
        lines << (item.ok ? "  [ OK ] " : "  [FAIL] ") + item.text;
    return lines.join("\n");
}

// "HE-AAC v2, 48 kHz Stereo @ 88 kbit/s" -> "HE-AAC v2, 48 kHz Stereo"
static QString audioFormat(const QString& mode)
{
    return normalizedMode(mode).section(" @ ", 0, 0);
}

static QString percent(int count, int total)
{
    return QString::number(total ? 100.0 * count / total : 0, 'f', 1) + " %";
}

CheckReport checkExpectations(const QJsonObject& clip,
                              const QJsonObject& defaults,
                              const RunResult& r)
{
    CheckReport report;
    auto check = [&report](bool ok, const QString& text) {
        report.items.append({ok, text});
        return ok;
    };

    const auto e = clip["expect"].toObject();

    for (const auto& error : r.errors)
        check(false, "Receiver error: " + QString::fromStdString(error));

    if (e["sync"].toBool())
        check(r.sync, "Sync");

    // Robustness cases only have to survive the recording
    if (!e.contains("ensemble_id")) {
        check(true, "Robustness case, no decoding expected");
        return report;
    }

    const QString ensembleId = hex(r.ensembleId, 4);
    const QString ensembleLabel = QString::fromStdString(r.ensembleLabel).trimmed();
    check(ensembleId == e["ensemble_id"].toString() && ensembleLabel == e["ensemble_label"].toString(),
            QString("Ensemble %1 \"%2\"").arg(ensembleId, ensembleLabel) +
            (ensembleId == e["ensemble_id"].toString() && ensembleLabel == e["ensemble_label"].toString() ? "" :
             QString(", expected %1 \"%2\"").arg(e["ensemble_id"].toString(), e["ensemble_label"].toString())));

    if (e.contains("max_first_fib_s")) {
        const double max = e["max_first_fib_s"].toDouble();
        check(r.firstFibOkSeconds >= 0 && r.firstFibOkSeconds <= max,
                QString("First correct FIB after %1 s (max %2 s)")
                .arg(r.firstFibOkSeconds, 0, 'f', 1).arg(max));
    }

    int frames = 0, frameErrors = 0, superframes = 0, rsUncorrected = 0;

    for (const auto& value : e["services"].toArray()) {
        const auto es = value.toObject();
        const QString sid = es["sid"].toString();
        const QString name = QString("%1 \"%2\"").arg(sid, es["label"].toString());

        const ServiceResult* s = nullptr;
        for (const auto& candidate : r.services) {
            if (hex(candidate.sid, 4) == sid)
                s = &candidate;
        }

        if (!s) {
            check(false, name + ": not found");
            if (es.contains("mode"))
                report.audioServices.append({audioFormat(es["mode"].toString()), "", false});
            continue;
        }

        // Everything of one service is reported in one line
        QStringList found, wrong;

        const QString label = QString::fromStdString(s->label).trimmed();
        if (label != es["label"].toString())
            wrong << QString("label \"%1\"").arg(label);

        const QString type = QString::fromStdString(s->type);
        found << type;
        if (type != es["type"].toString())
            wrong << QString("type %1, expected %2").arg(type, es["type"].toString());

        if (es.contains("bitrate")) {
            found << QString("%1 kbit/s %2").arg(s->bitrate).arg(QString::fromStdString(s->protection));
            if (s->bitrate != es["bitrate"].toInt())
                wrong << QString("bitrate %1, expected %2").arg(s->bitrate).arg(es["bitrate"].toInt());
        }

        if (es.contains("mode")) {
            const QString mode = QString::fromStdString(s->mode);
            found << audioFormat(mode)
                  << QString("%1 s audio").arg(s->audioSeconds, 0, 'f', 1);

            if (normalizedMode(mode) != normalizedMode(es["mode"].toString()))
                wrong << QString("audio \"%1\", expected \"%2\"").arg(mode, es["mode"].toString());
            if (s->sampleRate != es["samplerate"].toInt())
                wrong << QString("sample rate %1, expected %2").arg(s->sampleRate).arg(es["samplerate"].toInt());
            if (s->audioSeconds < es["min_audio_s"].toDouble())
                wrong << QString("expected at least %1 s audio").arg(es["min_audio_s"].toDouble());

            frames += s->frameCount;
            frameErrors += s->frameErrors;
            superframes += s->rsCount;
            rsUncorrected += s->rsUncorrected;

            report.audioServices.append({audioFormat(es["mode"].toString()),
                    QString::fromStdString(s->protection), wrong.isEmpty()});
        }
        else if (es.contains("audio") && !es["audio"].toBool()) {
            found << "no audio expected";
        }

        if (!s->dls.empty())
            found << "DLS";
        if (!s->motNames.empty())
            found << QString("%1 slides").arg(s->motNames.size());

        check(wrong.isEmpty(), name + ": " + found.join(", ") +
                (wrong.isEmpty() ? "" : " -- " + wrong.join(", ")));
    }

    const double maxFrameErrorRate = e.contains("max_frame_error_rate") ?
        e["max_frame_error_rate"].toDouble() : defaults["max_frame_error_rate"].toDouble();
    check(!frames || (double)frameErrors / frames <= maxFrameErrorRate,
            QString("Audio frame errors: %1 of %2 = %3 (max %4)")
            .arg(frameErrors).arg(frames).arg(percent(frameErrors, frames))
            .arg(percent(qRound(maxFrameErrorRate * 1000), 1000)));

    if (superframes) {
        const double maxRsRate = e.contains("max_rs_uncorrected_rate") ?
            e["max_rs_uncorrected_rate"].toDouble() : defaults["max_rs_uncorrected_rate"].toDouble();
        check((double)rsUncorrected / superframes <= maxRsRate,
                QString("Uncorrectable DAB+ superframes: %1 of %2 = %3 (max %4)")
                .arg(rsUncorrected).arg(superframes).arg(percent(rsUncorrected, superframes))
                .arg(percent(qRound(maxRsRate * 1000), 1000)));
    }

    int dlsServices = 0, slides = 0;
    for (const auto& s : r.services) {
        if (!s.dls.empty())
            dlsServices++;
        slides += (int)s.motNames.size();
    }

    if (e.contains("min_dls_services"))
        check(dlsServices >= e["min_dls_services"].toInt(),
                QString("%1 services with DLS (min %2)").arg(dlsServices).arg(e["min_dls_services"].toInt()));

    if (e.contains("min_mot"))
        check(slides >= e["min_mot"].toInt(),
                QString("%1 MOT slides (min %2)").arg(slides).arg(e["min_mot"].toInt()));

    return report;
}
