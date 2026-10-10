/*
 *    Copyright (C) 2026
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    This file was generated with an AI assistant (Claude Code).
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
#include <QTemporaryDir>
#include <QtTest>

// main() for the GUI tests. The tests run without a display and with
// their own, empty settings, so they neither need nor change the settings
// of an installed welle.io.
#define WELLE_GUI_TEST_MAIN(TestClass) \
int main(int argc, char** argv) \
{ \
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) \
        qputenv("QT_QPA_PLATFORM", "offscreen"); \
    QTemporaryDir configDir; \
    qputenv("XDG_CONFIG_HOME", configDir.path().toUtf8()); \
    QCoreApplication::setOrganizationName("welle.io"); \
    QCoreApplication::setOrganizationDomain("welle.io"); \
    QCoreApplication::setApplicationName("welle.io"); \
    QApplication app(argc, argv); \
    TestClass test; \
    return QTest::qExec(&test, argc, argv); \
}

#endif // GUI_TEST_MAIN_H
