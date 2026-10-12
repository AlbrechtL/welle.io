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

// gui.ui.messages: error and info messages in the snackbar. A RAW file
// that doesn't exist shows an error that stays until it is dismissed, info
// messages disappear by themselves, messages are queued and the snackbar
// fits into a phone sized window above the mini player.

#include "gui_helper.h"
#include "gui_test_main.h"
#include "radio_controller.h"
#include "ui_test_case.h"

static const QString clipId = "odr_dabmod";

class UiMessagesTests : public UiTestCase
{
    Q_OBJECT

private slots:
    void initTestCase();

    void showsErrorUntilDismissed();
    void hidesInfoAutomatically();
    void queuesMessages();
    void fitsCompactWindow();

private:
    QObject* snackbar() const { return app.find("snackbar"); }
    bool snackbarVisible() const { return snackbar()->property("visible").toBool() && snackbar()->property("opacity").toReal() > 0.99; }
    bool snackbarHidden() const { return !snackbar()->property("visible").toBool(); }
    void showMessage(const QString& text, const QString& kind)
    {
        QMetaObject::invokeMethod(app.root(), "showMessage", Q_ARG(QVariant, text), Q_ARG(QVariant, kind));
    }
};

void UiMessagesTests::initTestCase()
{
    UI_START_WITH_CLIP(app, clipId);
    QVERIFY(snackbarHidden());
}

void UiMessagesTests::showsErrorUntilDismissed()
{
    const QString missingFile = QDir::temp().filePath("welle-io-test-missing.iq");
    QVERIFY(!QFile::exists(missingFile));
    app.gui()->openRawFile(missingFile, "auto");

    QTRY_VERIFY(snackbarVisible());
    QCOMPARE(snackbar()->property("kind").toString(), QString("error"));
    QVERIFY(app.text("snackbar.text").contains("welle-io-test-missing.iq"));
    app.screenshot("messages-error");

    // Still there after the time an info message is shown
    QTest::qWait(6000);
    QVERIFY(snackbarVisible());

    QVERIFY(app.click("snackbar.close"));
    QTRY_VERIFY(snackbarHidden());

    // Back to the recording
    app.gui()->openRawFile(app.clipFile(), "auto");

    qInfo().noquote() << "Error shown until dismissed:" << app.text("snackbar.text");
}

void UiMessagesTests::hidesInfoAutomatically()
{
    showMessage("Info for the test", "info");
    QTRY_VERIFY(snackbarVisible());
    QCOMPARE(snackbar()->property("kind").toString(), QString("info"));
    app.screenshot("messages-info");
    QTRY_VERIFY_WITH_TIMEOUT(snackbarHidden(), 7000);

    qInfo() << "Info messages disappear after 5 s";
}

void UiMessagesTests::queuesMessages()
{
    showMessage("First error", "error");
    showMessage("Second error", "error");
    showMessage("Second error", "error"); // Not queued twice
    QTRY_VERIFY(snackbarVisible());
    QCOMPARE(app.text("snackbar.text"), QString("First error"));

    QVERIFY(app.click("snackbar.close"));
    QTRY_COMPARE(app.text("snackbar.text"), QString("Second error"));
    QTRY_VERIFY(snackbarVisible());

    QVERIFY(app.click("snackbar.close"));
    QTRY_VERIFY(snackbarHidden());
    QTest::qWait(500);
    QVERIFY(snackbarHidden());

    qInfo() << "Messages are shown one after the other, duplicates once";
}

void UiMessagesTests::fitsCompactWindow()
{
    app.resize(360, 640);
    QVERIFY(scanStations());
    QVERIFY(app.click(stationRow(app.services().first()["sid"].toInt())));
    QTRY_VERIFY(app.isVisible("miniPlayer"));

    showMessage("A long error message that needs more than one line in a narrow window", "error");
    QTRY_VERIFY(snackbarVisible());

    const QRectF bar(snackbar()->property("x").toReal(), snackbar()->property("y").toReal(),
                     snackbar()->property("width").toReal(), snackbar()->property("height").toReal());
    QQuickItem* miniPlayer = app.item("miniPlayer");
    const QRectF player = miniPlayer->mapRectToScene(QRectF(0, 0, miniPlayer->width(), miniPlayer->height()));
    QVERIFY2(bar.left() >= 0 && bar.right() <= 360, "The snackbar is wider than the window");
    QVERIFY2(bar.bottom() <= player.top(), "The snackbar covers the mini player");
    UI_VERIFY_EMPTY(app.itemsOutsideWindow(), "Items outside the window");
    app.screenshot("messages-compact");

    QVERIFY(app.click("snackbar.close"));
    QTRY_VERIFY(snackbarHidden());

    qInfo() << "The snackbar fits into a 360 px window above the mini player";
}

WELLE_GUI_TEST_MAIN(UiMessagesTests)

#include "tst_ui_messages.moc"
