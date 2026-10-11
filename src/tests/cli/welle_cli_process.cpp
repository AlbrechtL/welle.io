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

#include "welle_cli_process.h"

#include <QDeadlineTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QTcpServer>
#include <QTest>
#include <QTimer>

QString welleCliPath()
{
    // welle-cli runs in a temporary directory
    const QString path = qEnvironmentVariable("WELLE_CLI");
    return path.isEmpty() ? path : QFileInfo(path).absoluteFilePath();
}

QString missingWelleCli()
{
    const QString path = welleCliPath();
    if (path.isEmpty())
        return "WELLE_CLI is not set, run the test with ctest";
    if (!QFileInfo(path).isExecutable())
        return "WELLE_CLI " + path + " is not an executable";
    return QString();
}

QString normalizedMode(QString mode)
{
    return mode.replace("Joint Stereo", "Stereo");
}

QJsonObject muxService(const QJsonObject& mux, const QString& sid)
{
    for (const auto& value : mux["services"].toArray()) {
        const QJsonObject service = value.toObject();
        if (service["sid"].toString().compare(sid, Qt::CaseInsensitive) == 0)
            return service;
    }
    return QJsonObject();
}

QString muxLabel(const QJsonValue& label)
{
    return label["label"].toString().trimmed();
}

void CheckList::check(bool ok, const QString& text)
{
    items.append({ok, text});
}

bool CheckList::ok() const
{
    for (const auto& item : items)
        if (!item.ok)
            return false;
    return true;
}

QString CheckList::text() const
{
    QStringList lines;
    for (const auto& item : items)
        lines << QString("  %1 %2").arg(item.ok ? "[ OK ]" : "[FAIL]", item.text);
    return lines.join("\n");
}

QStringList CheckList::failures() const
{
    QStringList list;
    for (const auto& item : items)
        if (!item.ok)
            list << item.text;
    return list;
}

static int freePort()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0))
        return -1;
    const int port = server.serverPort();
    server.close();
    return port;
}

WelleCli::WelleCli()
{
    process.setProgram(welleCliPath());
    process.setWorkingDirectory(dir.path());

    // Debug builds of welle-cli use AddressSanitizer. Its leak report at
    // exit would change the exit code; memory errors are still reported.
    auto env = QProcessEnvironment::systemEnvironment();
    QString asan = env.value("ASAN_OPTIONS");
    asan += QString(asan.isEmpty() ? "" : ":") + "detect_leaks=0";
    env.insert("ASAN_OPTIONS", asan);
    process.setProcessEnvironment(env);
}

WelleCli::~WelleCli()
{
    stop();
}

QString WelleCli::workDir() const
{
    return dir.path();
}

void WelleCli::start(const QStringList& args)
{
    out.clear();
    err.clear();
    process.setArguments(args);
    process.start();
    process.waitForStarted();
}

bool WelleCli::startWebServer(const QStringList& args, int timeoutMs)
{
    for (int attempt = 0; attempt < 3; attempt++) {
        webPort = freePort();
        start(args + QStringList{"-w", QString::number(webPort)});

        QDeadlineTimer deadline(timeoutMs);
        while (!deadline.hasExpired() && isRunning()) {
            if (httpGet("/mux.json", 0, 2000).ok)
                return true;
            QTest::qWait(200);
        }
        if (isRunning())
            return false;
        // Exited right away, e.g. because another process took the port
        stop();
    }
    return false;
}

bool WelleCli::runToEnd(const QStringList& args, int timeoutMs)
{
    start(args);
    if (waitForExit(timeoutMs))
        return true;
    stop();
    return false;
}

void WelleCli::writeStdin(const QByteArray& data)
{
    process.write(data);
    process.waitForBytesWritten(1000);
}

bool WelleCli::waitForOutput(const QString& text, int timeoutMs)
{
    QDeadlineTimer deadline(timeoutMs);
    while (true) {
        if (standardOutput().contains(text) || standardError().contains(text))
            return true;
        if (deadline.hasExpired() || !isRunning())
            return false;
        QTest::qWait(100);
    }
}

bool WelleCli::waitForExit(int timeoutMs)
{
    QDeadlineTimer deadline(timeoutMs);
    while (isRunning() && !deadline.hasExpired())
        process.waitForFinished(100);
    readOutput();
    return !isRunning();
}

void WelleCli::stop()
{
    if (isRunning()) {
        process.terminate();
        if (!process.waitForFinished(10000)) {
            process.kill();
            process.waitForFinished(5000);
        }
    }
    readOutput();
}

bool WelleCli::isRunning() const
{
    return process.state() != QProcess::NotRunning;
}

int WelleCli::exitCode() const
{
    return process.exitCode();
}

bool WelleCli::crashed() const
{
    return process.exitStatus() == QProcess::CrashExit;
}

void WelleCli::readOutput()
{
    out += process.readAllStandardOutput();
    err += process.readAllStandardError();
}

QString WelleCli::standardOutput()
{
    readOutput();
    return QString::fromUtf8(out);
}

QString WelleCli::standardError()
{
    readOutput();
    return QString::fromUtf8(err);
}

QString WelleCli::healthProblem()
{
    const QString errors = standardError();
    if (errors.contains("ERROR: AddressSanitizer") || errors.contains("runtime error:"))
        return "welle-cli reported a memory error";
    if (!isRunning())
        return QString("welle-cli exited (%1, exit code %2)")
            .arg(crashed() ? "crashed" : "normal").arg(exitCode());
    return QString();
}

QString WelleCli::errorTail(int lines)
{
    const QStringList all = standardError().split('\n');
    return "Last lines of the welle-cli stderr:\n    " +
        all.mid(qMax(0, all.size() - lines)).join("\n    ");
}

HttpReply WelleCli::httpGet(const QString& path, qint64 maxBytes, int timeoutMs)
{
    HttpReply result;

    QNetworkAccessManager manager;
    manager.setProxy(QNetworkProxy::NoProxy);
    QNetworkRequest request(QUrl(QString("http://127.0.0.1:%1%2").arg(webPort).arg(path)));
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool timedOut = false;
    QObject::connect(&timer, &QTimer::timeout, &loop, [&] {
        timedOut = true;
        reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::readyRead, &loop, [&] {
        result.body += reply->readAll();
        if (maxBytes > 0 && result.body.size() >= maxBytes)
            reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    if (reply->isRunning())
        loop.exec();

    result.body += reply->readAll();
    const QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    result.status = status.isValid() ? status.toInt() : 0;
    result.ok = result.status > 0;
    result.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    if (!result.ok)
        result.error = timedOut ? QString("timeout after %1 ms").arg(timeoutMs) : reply->errorString();
    else if (timedOut && maxBytes > 0)
        result.error = QString("only %1 of %2 bytes").arg(result.body.size()).arg(maxBytes);
    reply->deleteLater();
    return result;
}

QJsonObject WelleCli::mux()
{
    const HttpReply reply = httpGet("/mux.json");
    if (!reply.ok || reply.status != 200)
        return QJsonObject();
    return QJsonDocument::fromJson(reply.body).object();
}

bool WelleCli::pollMux(const std::function<bool(const QJsonObject&)>& done,
                       int timeoutMs, QJsonObject* lastMux)
{
    QDeadlineTimer deadline(timeoutMs);
    while (isRunning()) {
        const QJsonObject m = mux();
        if (!m.isEmpty()) {
            if (lastMux)
                *lastMux = m;
            if (done(m))
                return true;
        }
        if (deadline.hasExpired())
            return false;
        QTest::qWait(500);
    }
    return false;
}
