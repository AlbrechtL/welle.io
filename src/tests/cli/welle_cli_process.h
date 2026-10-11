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

// Runs the welle-cli binary as a black box for the cli.* tests: starts it
// with command line options in a temporary working directory, collects its
// output, talks to its web server and stops it again.

#ifndef WELLE_CLI_PROCESS_H
#define WELLE_CLI_PROCESS_H

#include <functional>

#include <QByteArray>
#include <QJsonObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

// Path of the welle-cli binary from $WELLE_CLI (set by CTest), or an empty
// string
QString welleCliPath();

// Why welle-cli can't be run, or an empty string if it can
QString missingWelleCli();

// MP2 encoders switch between stereo and joint stereo from frame to frame
QString normalizedMode(QString mode);

// The service with this SId ("0x8dab") from mux.json, or an empty object
QJsonObject muxService(const QJsonObject& mux, const QString& sid);

// Trimmed text of a label object of mux.json ({"label": ..., ...})
QString muxLabel(const QJsonValue& label);

struct HttpReply {
    bool ok = false;        // a HTTP response was received
    int status = 0;
    QString contentType;
    QByteArray body;
    QString error;
};

// One check of a test, printed as "[ OK ] text" or "[FAIL] text"
class CheckList {
public:
    void check(bool ok, const QString& text);
    bool ok() const;
    QString text() const;
    QStringList failures() const;

private:
    struct Item {
        bool ok;
        QString text;
    };
    QList<Item> items;
};

class WelleCli {
public:
    WelleCli();
    ~WelleCli();

    // Working directory of welle-cli, files it writes end up here
    QString workDir() const;

    void start(const QStringList& args);

    // Starts welle-cli with "-w <free port>" appended and waits until the web
    // server answers. Tries another port if welle-cli exits right away.
    bool startWebServer(const QStringList& args, int timeoutMs = 20000);

    // Runs welle-cli until it exits. Returns false on timeout (it is killed).
    bool runToEnd(const QStringList& args, int timeoutMs);

    void writeStdin(const QByteArray& data);

    // Waits until stdout or stderr contains text, or welle-cli exits
    bool waitForOutput(const QString& text, int timeoutMs);

    // Waits for welle-cli to exit by itself
    bool waitForExit(int timeoutMs);

    // SIGTERM, after 10 s SIGKILL
    void stop();

    bool isRunning() const;
    int exitCode() const;
    bool crashed() const;
    QString standardOutput();
    QString standardError();

    // Empty if welle-cli runs and wrote no sanitizer report, otherwise why not
    QString healthProblem();

    // Last lines of stderr, for failure messages
    QString errorTail(int lines = 30);

    int port() const { return webPort; }

    // GET path from the web server. With maxBytes > 0 the request is aborted
    // after that many bytes, for the endless audio and FIC streams.
    HttpReply httpGet(const QString& path, qint64 maxBytes = 0, int timeoutMs = 5000);

    // /mux.json, or an empty object
    QJsonObject mux();

    // Reads /mux.json every 500 ms until done(mux) is true. Stops early if
    // welle-cli exits. lastMux is the last mux.json received.
    bool pollMux(const std::function<bool(const QJsonObject&)>& done,
                 int timeoutMs, QJsonObject* lastMux = nullptr);

private:
    void readOutput();

    QTemporaryDir dir;
    QProcess process;
    QByteArray out;
    QByteArray err;
    int webPort = -1;
};

#endif // WELLE_CLI_PROCESS_H
