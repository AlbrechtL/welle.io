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

// cli.<function>: black box tests of welle-cli. The binary is started with
// its command line options, and the tests check its exit code, its output,
// the files it writes and its web server. Except for "options" they play
// the recording odr_dabmod of the test data (DAB and DAB+ service, DLS,
// slides). welle-cli plays files in real time, so most tests take 20-60 s.

#include <QtTest>

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QtEndian>

#include <cmath>

#include "expectations.h"
#include "welle_cli_process.h"

class CliTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void options_data();
    void options();
    void webServer();
    void webServerFlac();
    void carousel();
    void dumpAll();
    void testMode();
    void alsaPlayback();

private:
    QJsonObject manifest;
    QJsonObject odrClip;

    // Path of odr_dabmod, skips the test if it is missing
    QString odrPath();
    QJsonArray odrServices() const;
    void finish(const QString& title, const CheckList& checks, WelleCli& cli);
};

static const char* const odrClipId = "odr_dabmod";

void CliTests::initTestCase()
{
    const QString missing = missingWelleCli();
    if (!missing.isEmpty())
        QSKIP(qPrintable(missing));

    if (missingTestData().isEmpty()) {
        manifest = loadManifest();
        for (const auto& value : manifest["clips"].toArray())
            if (value.toObject()["id"].toString() == odrClipId)
                odrClip = value.toObject();
    }
}

QString CliTests::odrPath()
{
    QString missing = missingTestData();
    if (missing.isEmpty() && odrClip.isEmpty())
        missing = QString("No clip %1 in the manifest").arg(odrClipId);
    if (missing.isEmpty())
        missing = missingClip(odrClip);
    if (!missing.isEmpty())
        return QString();
    return clipPath(odrClip);
}

// QSKIP only works in the test function itself
#define REQUIRE_ODR_CLIP(path) \
    const QString path = odrPath(); \
    if (path.isEmpty()) \
        QSKIP("Needs the recording odr_dabmod of the test data, see src/tests/README.md")

QJsonArray CliTests::odrServices() const
{
    return odrClip["expect"].toObject()["services"].toArray();
}

void CliTests::finish(const QString& title, const CheckList& checks, WelleCli& cli)
{
    qInfo().noquote() << title + "\n" + checks.text();
    if (!checks.ok())
        QFAIL(qPrintable(checks.failures().join("\n   ") + "\n" + cli.errorTail()));
}

// Command line handling, no recording needed. "<empty file>" is replaced by
// an empty IQ file.
void CliTests::options_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<int>("exitCode");
    QTest::addColumn<QString>("message");

    QTest::newRow("help") << QStringList{"-h"} << 1 << "-c channel";
    QTest::newRow("version") << QStringList{"-v"} << 0 << "GPL-2.0-or-later";
    QTest::newRow("unknown option") << QStringList{"-x"} << 1 << "Unknown option";
    QTest::newRow("carousel and dump all") << QStringList{"-C", "1", "-D"} << 1
        << "Cannot select both -C and -D";
    QTest::newRow("missing IQ file") << QStringList{"-f", "/nonexistent/welle-cli-test.iq"} << 1
        << "Cannot open file";
    // Only the end of the message: the input thread writes to stderr at the same time
    QTest::newRow("invalid output codec") << QStringList{"-f", "<empty file>", "-w", "0", "-O", "ogg"} << 1
        << "not valid as an outputcodec";
}

void CliTests::options()
{
    QFETCH(QStringList, args);
    QFETCH(int, exitCode);
    QFETCH(QString, message);

    WelleCli cli;
    const QString emptyFile = QDir(cli.workDir()).filePath("empty.u8.iq");
    QFile f(emptyFile);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.close();
    args.replaceInStrings("<empty file>", emptyFile);

    QVERIFY2(cli.runToEnd(args, 10000), qPrintable("welle-cli did not exit\n" + cli.errorTail()));
    QVERIFY2(!cli.crashed(), qPrintable(cli.errorTail()));
    QCOMPARE(cli.exitCode(), exitCode);
    QVERIFY2(cli.standardError().contains(message),
             qPrintable(QString("\"%1\" not on stderr\n").arg(message) + cli.errorTail()));
    if (args.contains("-v"))
        QVERIFY(cli.standardError().startsWith("welle-cli "));
}

static bool hasMpegFrameSync(const QByteArray& data)
{
    for (int i = 0; i + 1 < data.size(); i++)
        if (uchar(data[i]) == 0xFF && (uchar(data[i + 1]) & 0xE0) == 0xE0)
            return true;
    return false;
}

static bool isImage(const QByteArray& data)
{
    return data.startsWith("\xFF\xD8") || data.startsWith("\x89PNG");
}

// "-w <port> -D": all services are decoded, mux.json and every GET resource
// of the web server
void CliTests::webServer()
{
    REQUIRE_ODR_CLIP(path);
    const QJsonObject expect = odrClip["expect"].toObject();
    const int minDls = expect["min_dls_services"].toInt();

    WelleCli cli;
    QVERIFY2(cli.startWebServer({"-f", path, "-D"}), qPrintable(cli.errorTail()));

    auto dlsCount = [](const QJsonObject& mux) {
        int n = 0;
        for (const auto& value : mux["services"].toArray())
            if (!value.toObject()["dls"].toObject()["label"].toString().isEmpty())
                n++;
        return n;
    };
    QJsonObject mux;
    cli.pollMux([&](const QJsonObject& m) {
        for (const auto& value : odrServices())
            if (muxService(m, value.toObject()["sid"].toString())["samplerate"].toInt() <= 0)
                return false;
        return dlsCount(m) >= minDls;
    }, 40000, &mux);

    CheckList checks;
    const QJsonObject ensemble = mux["ensemble"].toObject();
    checks.check(mux["demodulator"].toObject()["synced"].toBool(), "Sync");
    checks.check(ensemble["id"].toString() == expect["ensemble_id"].toString() &&
                 muxLabel(ensemble["label"]) == expect["ensemble_label"].toString(),
                 QString("Ensemble %1 \"%2\" (expected %3 \"%4\")")
                    .arg(ensemble["id"].toString(), muxLabel(ensemble["label"]),
                         expect["ensemble_id"].toString(), expect["ensemble_label"].toString()));

    for (const auto& value : odrServices()) {
        const QJsonObject es = value.toObject();
        const QJsonObject s = muxService(mux, es["sid"].toString());
        const QString mode = s["mode"].toString();
        checks.check(normalizedMode(mode) == normalizedMode(es["mode"].toString()) &&
                     s["samplerate"].toInt() == es["samplerate"].toInt() &&
                     s["channels"].toInt() == 2,
                     QString("%1 \"%2\": %3, %4 Hz, %5 channels, url_mp3 %6 (expected %7)")
                        .arg(es["sid"].toString(), muxLabel(s["label"]), mode)
                        .arg(s["samplerate"].toInt()).arg(s["channels"].toInt())
                        .arg(s["url_mp3"].toString(), es["mode"].toString()));
    }
    checks.check(dlsCount(mux) >= minDls, QString("%1 services with DLS (min %2)").arg(dlsCount(mux)).arg(minDls));

    // Static files, playlist, measurements
    struct Resource {
        const char* path;
        const char* contentType;
    };
    for (const Resource& r : {Resource{"/", "text/html"}, Resource{"/index.js", "text/javascript"},
                              Resource{"/favicon.ico", "image/x-icon"},
                              Resource{"/mux.m3u", "application/mpegurl"},
                              Resource{"/spectrum", "application/octet-stream"},
                              Resource{"/impulseresponse", "application/octet-stream"},
                              Resource{"/constellation", "application/octet-stream"},
                              Resource{"/nullspectrum", "application/octet-stream"}}) {
        const HttpReply reply = cli.httpGet(r.path);
        checks.check(reply.status == 200 && reply.contentType.startsWith(r.contentType) && !reply.body.isEmpty(),
                     QString("GET %1: %2 %3, %4 bytes %5").arg(r.path).arg(reply.status)
                        .arg(reply.contentType).arg(reply.body.size()).arg(reply.error));
    }

    const HttpReply playlist = cli.httpGet("/mux.m3u");
    for (const auto& value : odrServices()) {
        const QString url = QString("http://127.0.0.1:%1/mp3/%2").arg(cli.port()).arg(value.toObject()["sid"].toString());
        checks.check(playlist.body.contains(url.toUtf8()), "mux.m3u contains " + url);
    }

    // Endless streams: the first bytes are enough
    for (const auto& value : odrServices()) {
        const QString url = "/mp3/" + value.toObject()["sid"].toString();
        const HttpReply mp3 = cli.httpGet(url, 4096, 10000);
        checks.check(mp3.status == 200 && mp3.contentType == "audio/mpeg" &&
                     mp3.body.size() >= 4096 && hasMpegFrameSync(mp3.body),
                     QString("GET %1: %2 %3, %4 bytes with MPEG frame sync %5").arg(url).arg(mp3.status)
                        .arg(mp3.contentType).arg(mp3.body.size()).arg(mp3.error));
    }
    const HttpReply fic = cli.httpGet("/fic", 96, 10000);
    checks.check(fic.status == 200 && fic.body.size() >= 96,
                 QString("GET /fic: %1, %2 bytes %3").arg(fic.status).arg(fic.body.size()).arg(fic.error));

    // Errors
    const HttpReply unknown = cli.httpGet("/does-not-exist");
    checks.check(unknown.status == 404, QString("GET /does-not-exist: %1").arg(unknown.status));
    const HttpReply postOnly = cli.httpGet("/fftwindowplacement");
    checks.check(postOnly.status == 405, QString("GET /fftwindowplacement (POST only): %1").arg(postOnly.status));

    // The slide carousel of the recording takes a while in real time
    HttpReply slide;
    QString slideUrl;
    for (int i = 0; i < 120 && !isImage(slide.body); i++) {
        for (const auto& value : odrServices()) {
            slideUrl = "/slide/" + value.toObject()["sid"].toString();
            slide = cli.httpGet(slideUrl);
            if (slide.status == 200 && isImage(slide.body))
                break;
        }
        if (!isImage(slide.body))
            QTest::qWait(500);
    }
    checks.check(slide.status == 200 && isImage(slide.body),
                 QString("GET %1: %2 %3, %4 bytes").arg(slideUrl).arg(slide.status)
                    .arg(slide.contentType).arg(slide.body.size()));
    if (slide.status == 200) {
        const QJsonObject s = muxService(cli.mux(), slideUrl.mid(7));
        checks.check(s["mot"].toObject()["time"].toInteger() > 0, "mux.json: mot.time of " + slideUrl.mid(7));
    }

    const QString problem = cli.healthProblem();
    checks.check(problem.isEmpty(), problem.isEmpty() ? "welle-cli still running" : problem);
    finish("welle-cli -f odr_dabmod -D -w <port>", checks, cli);
}

// "-O flac": FLAC streams, only if welle-cli was built with FLAC
void CliTests::webServerFlac()
{
    REQUIRE_ODR_CLIP(path);

    WelleCli cli;
    if (!cli.startWebServer({"-f", path, "-D", "-O", "flac"})) {
        if (cli.standardError().contains("Flac support not compiled"))
            QSKIP("welle-cli was built without FLAC");
        QFAIL(qPrintable("The web server did not start\n" + cli.errorTail()));
    }

    const QString sid = odrServices().last().toObject()["sid"].toString();
    QVERIFY2(cli.pollMux([&](const QJsonObject& m) { return muxService(m, sid)["samplerate"].toInt() > 0; }, 30000),
             qPrintable(cli.errorTail()));

    CheckList checks;
    const HttpReply flac = cli.httpGet("/flac/" + sid, 4096, 10000);
    checks.check(flac.status == 200 && flac.body.startsWith("fLaC"),
                 QString("GET /flac/%1: %2 %3, %4 bytes, starts with fLaC").arg(sid).arg(flac.status)
                    .arg(flac.contentType).arg(flac.body.size()));
    const QString problem = cli.healthProblem();
    checks.check(problem.isEmpty(), problem.isEmpty() ? "welle-cli still running" : problem);
    finish("welle-cli -f odr_dabmod -D -O flac -w <port>", checks, cli);
}

// "-C 1": one decoder that switches to the next service every 10 seconds.
// mux.json keeps the audio format of services that were decoded before,
// so the test checks that one service starts first and the other follows.
void CliTests::carousel()
{
    REQUIRE_ODR_CLIP(path);

    WelleCli cli;
    QVERIFY2(cli.startWebServer({"-f", path, "-C", "1"}), qPrintable(cli.errorTail()));

    auto decoded = [&](const QJsonObject& m) {
        QStringList sids;
        for (const auto& value : odrServices()) {
            const QString sid = value.toObject()["sid"].toString();
            if (muxService(m, sid)["samplerate"].toInt() > 0)
                sids << sid;
        }
        return sids;
    };

    CheckList checks;
    QJsonObject mux;
    cli.pollMux([&](const QJsonObject& m) { return !decoded(m).isEmpty(); }, 20000, &mux);
    const QStringList first = decoded(mux);
    checks.check(first.size() == 1, "First decoded service: " + first.join(", "));

    cli.pollMux([&](const QJsonObject& m) { return decoded(m).size() == odrServices().size(); }, 35000, &mux);
    checks.check(decoded(mux).size() == odrServices().size(),
                 "All services decoded within 35 s: " + decoded(mux).join(", "));

    const QString problem = cli.healthProblem();
    checks.check(problem.isEmpty(), problem.isEmpty() ? "welle-cli still running" : problem);
    finish("welle-cli -f odr_dabmod -C 1 -w <port>", checks, cli);
}

// CRC of a FIB, EN 300 401 clause 5.2.1
static bool fibCrcOk(const char* fib)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < 30; i++) {
        crc ^= uint16_t(uchar(fib[i])) << 8;
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 0x8000) ? uint16_t((crc << 1) ^ 0x1021) : uint16_t(crc << 1);
    }
    return uint16_t(~crc) == qFromBigEndian<uint16_t>(fib + 30);
}

struct WavInfo {
    bool ok = false;
    int sampleRate = 0;
    int channels = 0;
    double seconds = 0;
    double rms = 0;
};

// The header sizes are only written when welle-cli closes the file, so the
// samples are taken from the data chunk up to the end of the file
static WavInfo readWav(const QString& path)
{
    WavInfo info;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return info;
    const QByteArray d = f.readAll();
    const int data = d.indexOf("data");
    if (d.size() < 44 || !d.startsWith("RIFF") || d.mid(8, 4) != "WAVE" || data < 0)
        return info;

    QDataStream header(d);
    header.setByteOrder(QDataStream::LittleEndian);
    header.skipRawData(22);
    quint16 channels = 0;
    quint32 sampleRate = 0;
    header >> channels >> sampleRate;
    info.channels = channels;
    info.sampleRate = sampleRate;
    const int samples = (d.size() - data - 8) / 2;
    double sum = 0;
    for (int i = 0; i < samples; i++) {
        const double x = qFromLittleEndian<int16_t>(d.constData() + data + 8 + 2 * i);
        sum += x * x;
    }
    info.rms = samples > 0 ? std::sqrt(sum / samples) : 0;
    if (info.channels > 0 && info.sampleRate > 0)
        info.seconds = double(samples) / info.channels / info.sampleRate;
    info.ok = true;
    return info;
}

// "-D" without web server: dump.fic, <label>.msc and <label>.wav of every
// service, DLS on stdout, quit with "."
void CliTests::dumpAll()
{
    REQUIRE_ODR_CLIP(path);

    WelleCli cli;
    cli.start({"-f", path, "-D"});
    QVERIFY2(cli.waitForOutput("Enter '.' to quit", 30000), qPrintable(cli.errorTail()));
    QTest::qWait(20000);

    CheckList checks;
    const QDir dir(cli.workDir());

    QFile ficFile(dir.filePath("dump.fic"));
    QByteArray fic;
    if (ficFile.open(QIODevice::ReadOnly))
        fic = ficFile.readAll();
    int badFibs = 0;
    for (int i = 0; i + 32 <= fic.size(); i += 32)
        if (!fibCrcOk(fic.constData() + i))
            badFibs++;
    checks.check(!fic.isEmpty() && fic.size() % 32 == 0 && badFibs == 0,
                 QString("dump.fic: %1 bytes = %2 FIBs, %3 with CRC errors")
                    .arg(fic.size()).arg(fic.size() / 32.0).arg(badFibs));

    const QString out = cli.standardOutput();
    for (const auto& value : odrServices()) {
        const QJsonObject es = value.toObject();
        const QString label = es["label"].toString();
        const QString sid = es["sid"].toString();

        const qint64 mscSize = QFileInfo(dir.filePath(label + ".msc")).size();
        checks.check(mscSize > 0, QString("%1.msc: %2 bytes").arg(label).arg(mscSize));

        const WavInfo wav = readWav(dir.filePath(label + ".wav"));
        checks.check(wav.ok && wav.sampleRate == es["samplerate"].toInt() && wav.channels == 2 &&
                     wav.seconds >= 10 && wav.rms > 100,
                     QString("%1.wav: %2 Hz, %3 channels, %4 s, RMS %5")
                        .arg(label).arg(wav.sampleRate).arg(wav.channels)
                        .arg(wav.seconds, 0, 'f', 1).arg(wav.rms, 0, 'f', 0));

        const QString rate = QString("[%1] rate %2 mode ").arg(sid).arg(es["samplerate"].toInt());
        checks.check(out.contains(rate), QString("stdout: \"%1...\"").arg(rate));
    }

    QSet<QString> dlsServices;
    static const QRegularExpression dlsLine("^\\[(0x[0-9a-f]+)\\] DLS: .+$", QRegularExpression::MultilineOption);
    for (auto it = dlsLine.globalMatch(out); it.hasNext();)
        dlsServices.insert(it.next().captured(1));
    const int minDls = odrClip["expect"].toObject()["min_dls_services"].toInt();
    checks.check(dlsServices.size() >= minDls,
                 QString("stdout: DLS of %1 services (min %2)").arg(dlsServices.size()).arg(minDls));

    QString problem = cli.healthProblem();
    checks.check(problem.isEmpty(), problem.isEmpty() ? "welle-cli still running" : problem);

    cli.writeStdin(".\n");
    const bool exited = cli.waitForExit(10000);
    checks.check(exited && !cli.crashed() && cli.exitCode() == 0,
                 QString("Quits with \".\": %1, exit code %2").arg(exited ? "exited" : "still running")
                    .arg(cli.exitCode()));
    problem = cli.healthProblem();
    checks.check(!problem.contains("memory error"), "No memory errors at exit");

    finish("welle-cli -f odr_dabmod -D", checks, cli);
}

// "-t 3": decodes the first service of the file at full speed and prints
// error statistics at the end of the file
void CliTests::testMode()
{
    REQUIRE_ODR_CLIP(path);

    WelleCli cli;
    const bool exited = cli.runToEnd({"-f", path, "-t", "3"}, 120000);

    CheckList checks;
    checks.check(exited && !cli.crashed() && cli.exitCode() == 0,
                 QString("Ends at the end of the file, exit code %1").arg(cli.exitCode()));

    const QString err = cli.standardError();
    const QJsonObject defaults = manifest["defaults"].toObject();

    // "Num samps processed: <n>"
    static const QRegularExpression samplesLine("Num samps processed: (\\d+)");
    const auto samples = samplesLine.match(err);
    const double seconds = samples.captured(1).toDouble() / 2048000;
    checks.check(samples.hasMatch() && seconds >= odrClip["duration_s"].toDouble() - 1,
                 QString("%1 s of the recording processed").arg(seconds, 0, 'f', 1));

    static const QRegularExpression syncLine("Num syncs/desyncs: (\\d+)/(\\d+)");
    const auto syncs = syncLine.match(err);
    checks.check(syncs.hasMatch() && syncs.captured(1).toInt() >= 1,
                 QString("Syncs/desyncs: %1/%2").arg(syncs.captured(1), syncs.captured(2)));

    // "<name>ErrorStats (<calls>) : <sum of errors>"
    auto stats = [&](const QString& name, double maxRate) {
        const QRegularExpression line(name + " \\((\\d+)\\) : (\\d+)");
        const auto m = line.match(err);
        const int calls = m.captured(1).toInt();
        const int errors = m.captured(2).toInt();
        checks.check(m.hasMatch() && (calls == 0 || double(errors) / calls <= maxRate),
                     QString("%1: %2 errors in %3 (max %4 %)").arg(name).arg(errors).arg(calls)
                        .arg(maxRate * 100));
        return calls;
    };
    const int frames = stats("frameErrorStats", defaults["max_frame_error_rate"].toDouble());
    checks.check(frames > 0, "Audio frames decoded");
    stats("rsErrorStats", defaults["max_rs_uncorrected_rate"].toDouble());

    checks.check(cli.standardOutput().contains("rate 48000"), "stdout: \"rate 48000\"");
    checks.check(!err.contains("ERROR: AddressSanitizer"), "No memory errors");

    finish("welle-cli -f odr_dabmod -t 3", checks, cli);
}

// "-p <programme>": plays with ALSA (the "null" device), more programmes
// are selected on stdin
void CliTests::alsaPlayback()
{
    REQUIRE_ODR_CLIP(path);
    const QJsonObject first = odrServices().last().toObject();
    const QJsonObject second = odrServices().first().toObject();

    WelleCli cli;
    cli.start({"-f", path, "-p", first["sid"].toString(), "-o", "null"});
    const bool prompt = cli.waitForOutput("Please enter programme name", 30000);
    if (cli.standardError().contains("Nothing to do, not ALSA support"))
        QSKIP("welle-cli was built without ALSA");
    QVERIFY2(prompt, qPrintable(cli.errorTail()));
    if (cli.waitForOutput("PCM name:", 10000) && cli.standardError().contains("ERROR: Can't open"))
        QSKIP("The ALSA null device can't be opened");

    CheckList checks;
    const QString err = cli.standardError();
    for (const auto& value : odrServices()) {
        const QJsonObject es = value.toObject();
        const QString line = QString("[%1] %2").arg(es["sid"].toString(), es["label"].toString());
        const bool listed = err.contains(QRegularExpression(QRegularExpression::escape(line) +
                                         "\\s+\\[component \\d+ ASCTy: " +
                                         QRegularExpression::escape(es["type"].toString()) + " \\]"));
        checks.check(listed, "Service list: " + line + " ASCTy: " + es["type"].toString());
    }
    checks.check(!err.contains("Could not tune"), "Plays " + first["sid"].toString());
    checks.check(err.contains("PCM rate: 48000"), "ALSA output with 48000 Hz");

    // cin reads single words, so the SId is used instead of the label
    cli.writeStdin(second["sid"].toString().toUtf8() + "\n");
    const bool trying = cli.waitForOutput("Trying to tune to " + second["sid"].toString(), 10000);

    // Answered in order, so the second programme was handled before this
    cli.writeStdin("nonexistent\n");
    checks.check(cli.waitForOutput("Could not tune to nonexistent", 10000), "Unknown programme reported");
    checks.check(trying && !cli.standardError().contains("Could not tune to " + second["sid"].toString()),
                 "Switches to " + second["sid"].toString());

    cli.writeStdin(".\n");
    const bool exited = cli.waitForExit(10000);
    checks.check(exited && !cli.crashed() && cli.exitCode() == 0,
                 QString("Quits with \".\": exit code %1").arg(cli.exitCode()));
    checks.check(!cli.standardError().contains("ERROR: AddressSanitizer"), "No memory errors");

    finish("welle-cli -f odr_dabmod -p " + first["sid"].toString() + " -o null", checks, cli);
}

QTEST_GUILESS_MAIN(CliTests)

#include "tst_cli.moc"
