/*
 * Socket ownership transfer regression tests.
 * This file is part of welle.io and was generated with the help of an AI
 * assistant (OpenAi Codex 6 Astra).
 *
 * welle.io is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * welle.io is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with welle.io; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <memory>
#include <utility>
#include <vector>
#ifdef Q_OS_LINUX
#include <dirent.h>
#include <cerrno>
#endif
#include "various/Socket.h"

namespace {
#ifdef Q_OS_LINUX
int descriptorCount()
{
    DIR* directory = opendir("/proc/self/fd");
    if (!directory) return -1;
    int count = 0;
    errno = 0;
    while (const auto* entry = readdir(directory)) {
        if (entry->d_name[0] != '.') ++count;
    }
    const bool complete = errno == 0;
    closedir(directory);
    return complete ? count : -1;
}
#endif

struct Connection {
    QTcpServer server;
    Socket socket;
    std::unique_ptr<QTcpSocket> peer;

    bool open() {
        if (!server.listen(QHostAddress::LocalHost, 0) ||
            !socket.connect("127.0.0.1", server.serverPort(), 1)) {
            return false;
        }
        if (!server.hasPendingConnections() && !server.waitForNewConnection(1000)) {
            return false;
        }
        peer.reset(server.nextPendingConnection());
        server.close();
        return peer != nullptr;
    }
};

bool disconnected(QTcpSocket& peer)
{
    return peer.state() == QAbstractSocket::UnconnectedState ||
        peer.waitForDisconnected(1000);
}

bool sendsData(Socket& socket, QTcpSocket& peer)
{
    const QByteArray message("move test");
    if (socket.send(message.constData(), size_t(message.size()), MSG_NOSIGNAL) != message.size()) {
        return false;
    }
    QByteArray received;
    QElapsedTimer deadline;
    deadline.start();
    while (received.size() < message.size()) {
        const int remaining = 1000 - int(deadline.elapsed());
        if (remaining <= 0 || (peer.bytesAvailable() == 0 && !peer.waitForReadyRead(remaining))) {
            return false;
        }
        received += peer.readAll();
    }
    return received == message;
}
}

class TestSocket : public QObject {
    Q_OBJECT
private slots:
    void refusedConnectionDoesNotLeak() {
#ifdef Q_OS_LINUX
        QTcpServer reserved;
        QVERIFY(reserved.listen(QHostAddress::LocalHost, 0));
        const auto port = reserved.serverPort();
        reserved.close();
        const int before = descriptorCount();
        QVERIFY(before >= 0);
        for (int i = 0; i < 16; ++i) {
            Socket connection;
            QVERIFY(!connection.connect("127.0.0.1", port, 1));
            QVERIFY(!connection.valid());
        }
        QCOMPARE(descriptorCount(), before);
#else
        QSKIP("Descriptor counting requires Linux procfs");
#endif
    }

    void connectingTwicePreservesOriginal() {
        Connection connection;
        QVERIFY(connection.open());
        QTcpServer other;
        QVERIFY(other.listen(QHostAddress::LocalHost, 0));
        QVERIFY(!connection.socket.connect("127.0.0.1", other.serverPort(), 1));
        QVERIFY(sendsData(connection.socket, *connection.peer));
    }

    void highDescriptorConnects() {
#ifdef Q_OS_LINUX
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        struct Descriptors {
            std::vector<int> values;
            ~Descriptors() { for (int fd : values) ::close(fd); }
        } held;
        int fd;
        do {
            fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
            if (fd == -1) QSKIP("Descriptor limit too small for FD_SETSIZE test");
            held.values.push_back(fd);
        } while (fd < FD_SETSIZE + 4);
        Socket connection;
        QVERIFY(connection.connect("127.0.0.1", server.serverPort(), 1));
        if (!server.hasPendingConnections()) QVERIFY(server.waitForNewConnection(1000));
        std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
        QVERIFY(peer != nullptr);
        QVERIFY(sendsData(connection, *peer));
#else
        QSKIP("POSIX descriptor range test");
#endif
    }

    void moveAssignment_data() {
        QTest::addColumn<bool>("sourceValid");
        QTest::addColumn<bool>("targetValid");
        QTest::newRow("empty-to-empty") << false << false;
        QTest::newRow("connected-to-empty") << true << false;
        QTest::newRow("empty-to-connected") << false << true;
        QTest::newRow("connected-to-connected") << true << true;
    }

    void moveAssignment() {
        QFETCH(bool, sourceValid);
        QFETCH(bool, targetValid);
        Connection source, target;
        if (sourceValid) QVERIFY(source.open());
        if (targetValid) QVERIFY(target.open());

        Socket& result = (target.socket = std::move(source.socket));
        QCOMPARE(&result, &target.socket);
        QVERIFY(!source.socket.valid());
        QCOMPARE(target.socket.valid(), sourceValid);
        if (targetValid) QVERIFY(disconnected(*target.peer));
        if (sourceValid) {
            QVERIFY(sendsData(target.socket, *source.peer));
            target.socket.close();
            QVERIFY(disconnected(*source.peer));
        }
    }

    void selfMove_data() {
        QTest::addColumn<bool>("connected");
        QTest::newRow("empty") << false;
        QTest::newRow("connected") << true;
    }

    void moveAssignmentReleasesDescriptor_data() {
        QTest::addColumn<bool>("sourceValid");
        QTest::newRow("empty-to-connected") << false;
        QTest::newRow("connected-to-connected") << true;
    }

    void moveAssignmentReleasesDescriptor() {
#ifdef Q_OS_LINUX
        QFETCH(bool, sourceValid);
        if (descriptorCount() < 0) QSKIP("/proc/self/fd is unavailable");
        Connection source, target;
        if (sourceValid) QVERIFY(source.open());
        QVERIFY(target.open());
        const int before = descriptorCount();
        QVERIFY(before >= 0);
        target.socket = std::move(source.socket);
        // Measure before processing Qt events: the old peer still owns its
        // descriptor. shutdown() disconnects it but leaks the replaced fd.
        const int after = descriptorCount();
        QVERIFY(after >= 0);
        QCOMPARE(after, before - 1);
        QVERIFY(disconnected(*target.peer));
        if (sourceValid) QVERIFY(sendsData(target.socket, *source.peer));
#else
        QSKIP("Descriptor counting requires Linux procfs");
#endif
    }

    void selfMove() {
        QFETCH(bool, connected);
        Connection connection;
        if (connected) QVERIFY(connection.open());
        Socket& alias = connection.socket;
        Socket& result = (connection.socket = std::move(alias));
        QCOMPARE(&result, &connection.socket);
        QCOMPARE(connection.socket.valid(), connected);
        if (connected) QVERIFY(sendsData(connection.socket, *connection.peer));
    }

    void moveConstruction_data() { selfMove_data(); }

    void moveConstruction() {
        QFETCH(bool, connected);
        Connection connection;
        if (connected) QVERIFY(connection.open());
        {
            Socket moved(std::move(connection.socket));
            QVERIFY(!connection.socket.valid());
            QCOMPARE(moved.valid(), connected);
            if (connected) QVERIFY(sendsData(moved, *connection.peer));
        }
        if (connected) QVERIFY(disconnected(*connection.peer));
    }
};

QTEST_GUILESS_MAIN(TestSocket)
#include "tst_socket.moc"
