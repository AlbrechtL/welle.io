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

#ifndef GUI_TEST_MAIN_H
#define GUI_TEST_MAIN_H

#include <QApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QtTest>

// The tests render with OpenGL like welle-io on a desktop, so that shadows
// and other shader effects are drawn. Without OpenGL (or with
// QT_QUICK_BACKEND set) Qt Quick uses its software renderer.
inline void selectGraphicsApi()
{
    if (!qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND"))
        return;
    QOpenGLContext context;
    QOffscreenSurface surface;
    surface.create();
    if (context.create() && context.makeCurrent(&surface)) {
        context.doneCurrent();
        // The offscreen platform reports no OpenGL support, Qt Quick would
        // choose the software renderer without the explicit backend
        qputenv("QT_QUICK_BACKEND", "rhi");
        qputenv("QSG_RHI_BACKEND", "opengl");
    } else {
        qputenv("QT_QUICK_BACKEND", "software");
    }
}

// The offscreen platform (no display) with a large screen. Its default
// screen is 800 x 600, OpenGL doesn't draw outside of it.
inline void useOffscreenPlatform(const QString& configDir)
{
    const QByteArray platform = qgetenv("QT_QPA_PLATFORM");
    if (!platform.isEmpty() && platform != "offscreen")
        return;
    const QString config = configDir + "/offscreen.json";
    QFile file(config);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(R"({"screens": [{"name": "test", "x": 0, "y": 0, "width": 2560, "height": 1600,
                        "logicalDpi": 96, "logicalBaseDpi": 96, "dpr": 1}]})");
        file.close();
        qputenv("QT_QPA_PLATFORM", ("offscreen:configfile=" + config).toUtf8());
    } else {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
}

// main() for the GUI tests. The tests run without a display and with
// their own, empty settings and data directory, so they neither need nor
// change the settings of an installed welle.io. The user interface is
// English, unless WELLE_TEST_LANGUAGE sets a locale (e.g. de_DE.UTF-8).
#define WELLE_GUI_TEST_MAIN(TestClass) \
int main(int argc, char** argv) \
{ \
    QTemporaryDir platformDir; \
    useOffscreenPlatform(platformDir.path()); \
    qputenv("LC_ALL", qEnvironmentVariableIsEmpty("WELLE_TEST_LANGUAGE") \
                      ? QByteArray("C.UTF-8") : qgetenv("WELLE_TEST_LANGUAGE")); \
    QTemporaryDir configDir; \
    qputenv("XDG_CONFIG_HOME", configDir.path().toUtf8()); \
    QTemporaryDir dataDir; /* e.g. the station logos */ \
    qputenv("XDG_DATA_HOME", dataDir.path().toUtf8()); \
    QCoreApplication::setOrganizationName("welle.io"); \
    QCoreApplication::setOrganizationDomain("welle.io"); \
    QCoreApplication::setApplicationName("welle.io"); \
    QApplication app(argc, argv); \
    selectGraphicsApi(); \
    TestClass test; \
    return QTest::qExec(&test, argc, argv); \
}

#endif // GUI_TEST_MAIN_H
