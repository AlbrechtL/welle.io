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

#include "ui_app.h"

#include <QAccessible>
#include <QDir>
#include <QJsonArray>
#include <QMutex>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>
#include <QTranslator>
#include <QtQuickTest/quicktest.h>

#include "app_setup.h"
#include "expectations.h"
#include "gui_helper.h"
#include "radio_controller.h"

// Collects QML warnings and errors from all threads
static QMutex qmlMessagesMutex;
static QStringList qmlMessages;
static QtMessageHandler previousHandler = nullptr;

static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    const bool fromQml = (context.file && QString(context.file).startsWith("qrc:/")) || msg.startsWith("qrc:/");
    if (type != QtDebugMsg && type != QtInfoMsg && fromQml) {
        QMutexLocker lock(&qmlMessagesMutex);
        qmlMessages << msg;
    }
    previousHandler(type, context, msg);
}

UiApp::UiApp() = default;

// Installed when the user interface starts, i.e. while QTest::qExec() runs:
// qExec() installs its own handler and would replace one installed before
static void installMessageHandler()
{
    static bool installed = false;
    if (!installed) {
        previousHandler = qInstallMessageHandler(messageHandler);
        installed = true;
    }
}

UiApp::~UiApp()
{
    stop();
}

QString UiApp::useClip(const QString& clipId)
{
    const QString missing = missingTestData();
    if (!missing.isEmpty())
        return missing;

    for (const auto& value : loadManifest()["clips"].toArray()) {
        if (value.toObject()["id"].toString() == clipId)
            currentClip = value.toObject();
    }
    if (currentClip.isEmpty())
        return "Clip " + clipId + " is not in the manifest";

    const QString missingFile = missingClip(currentClip);
    if (!missingFile.isEmpty())
        return missingFile;

    currentClipFile = clipPath(currentClip);
    return QString();
}

QString UiApp::useClipWithMostServices()
{
    const QString missing = missingTestData();
    if (!missing.isEmpty())
        return missing;

    QString best;
    qsizetype bestCount = -1;
    for (const auto& value : loadManifest()["clips"].toArray()) {
        const QJsonObject clip = value.toObject();
        qsizetype count = 0;
        for (const auto& service : clip["expect"].toObject()["services"].toArray())
            count += service.toObject()["type"].toString() != "data";
        if (count > bestCount && missingClip(clip).isEmpty()) {
            best = clip["id"].toString();
            bestCount = count;
        }
    }
    if (best.isEmpty())
        return "No clip of the manifest is unpacked";
    return useClip(best);
}

QList<QJsonObject> UiApp::services() const
{
    QList<QJsonObject> result;
    for (const auto& value : currentClip["expect"].toObject()["services"].toArray()) {
        QJsonObject service = value.toObject();
        if (service["type"].toString() == "data")
            continue;
        service["sid"] = double(service["sid"].toString().toUInt(nullptr, 16));
        result << service;
    }
    return result;
}

void UiApp::writeSettings(const QVariantMap& values)
{
    QSettings settings;
    for (auto it = values.begin(); it != values.end(); ++it)
        settings.setValue(it.key(), it.value());
    settings.sync();
}

bool UiApp::start()
{
    // What the user would have chosen in the settings: RAW file input
    if (!currentClipFile.isEmpty()) {
        writeSettings({
            {"device", 5},
            {"enableAutoSdr", false},
            {"rawFile", currentClipFile},
            {"rawFileFormat", 0},  // auto
        });
    }

    // The window is created with the largest size of the tests and made
    // smaller afterwards: the OpenGL surface of the offscreen platform
    // keeps the size the window had when it was created.
    writeSettings({{"width", 2560}, {"height", 1600}});

    installMessageHandler();

    // Like a screen reader: controls get their accessible names
    QAccessible::setActive(true);

    if (!translator)
        translator = installTranslator();
    static bool environmentReady = false;
    if (!environmentReady) {
        setupQmlEnvironment();
        environmentReady = true;
    }

    radioController = std::make_unique<CRadioController>(commandLineOptions);
    radioController->setVolume(0);

    guiHelper = std::make_unique<CGUIHelper>(radioController.get());
    guiHelper->setTranslator(translator);

    engine = std::make_unique<QQmlApplicationEngine>();
    setupQmlEngine(*engine, guiHelper.get(), radioController.get());
    engine->load(QUrl("qrc:/QML/MainView.qml"));

    mainWindow = qobject_cast<QQuickWindow*>(root());
    if (!mainWindow)
        return false;
    if (!QTest::qWaitForWindowExposed(mainWindow))
        return false;
    // Keyboard shortcuts need an active window
    mainWindow->requestActivate();
    if (!QTest::qWaitForWindowActive(mainWindow))
        return false;
    resize(960, 640);
    return true;
}

bool UiApp::restart()
{
    stop();
    return start();
}

void UiApp::stop()
{
    // QML first, it uses the C++ objects. Settings are written when the
    // QML objects are destroyed.
    mainWindow = nullptr;
    engine.reset();
    if (guiHelper)
        guiHelper->disconnect();
    if (radioController)
        radioController->disconnect();
    guiHelper.reset();
    radioController.reset();
}

QObject* UiApp::root() const
{
    return engine ? engine->rootObjects().value(0) : nullptr;
}

void UiApp::forEachItem(const std::function<void(QQuickItem*)>& visit) const
{
    if (!mainWindow)
        return;

    std::function<void(QQuickItem*)> walk = [&](QQuickItem* item) {
        visit(item);
        for (QQuickItem* child : item->childItems())
            walk(child);
    };
    walk(mainWindow->contentItem());
}

QList<QObject*> UiApp::findAll(const QString& objectName) const
{
    QList<QObject*> result;
    QSet<QObject*> seen;
    auto add = [&](QObject* object) {
        if (object->objectName() == objectName && !seen.contains(object)) {
            seen.insert(object);
            result << object;
        }
    };

    // Non-visual objects (models, popups) are only in the QML object tree,
    // list delegates and popup contents only in the visual item tree
    if (QObject* rootObject = root()) {
        add(rootObject);
        for (QObject* child : rootObject->findChildren<QObject*>(objectName))
            add(child);
    }
    forEachItem([&](QQuickItem* item) {
        add(item);
        for (QObject* child : item->findChildren<QObject*>(objectName, Qt::FindDirectChildrenOnly))
            add(child);
    });
    return result;
}

QObject* UiApp::find(const QString& objectName) const
{
    return findAll(objectName).value(0);
}

QQuickItem* UiApp::item(const QString& objectName) const
{
    // Prefer a visible item if there are several
    QQuickItem* first = nullptr;
    for (QObject* object : findAll(objectName)) {
        if (auto* quickItem = qobject_cast<QQuickItem*>(object)) {
            if (quickItem->isVisible())
                return quickItem;
            if (!first)
                first = quickItem;
        }
    }
    return first;
}

QList<QQuickItem*> UiApp::visibleItems(const QString& objectName) const
{
    QList<QQuickItem*> result;
    for (QObject* object : findAll(objectName)) {
        auto* quickItem = qobject_cast<QQuickItem*>(object);
        if (quickItem && quickItem->isVisible() && quickItem->opacity() > 0)
            result << quickItem;
    }
    return result;
}

QVariant UiApp::property(const QString& objectName, const char* name) const
{
    QObject* object = item(objectName);
    if (!object)
        object = find(objectName);
    return object ? object->property(name) : QVariant();
}

QString UiApp::text(const QString& objectName) const
{
    return property(objectName, "text").toString().trimmed();
}

bool UiApp::isVisible(const QString& objectName) const
{
    QQuickItem* quickItem = item(objectName);
    if (quickItem)
        return quickItem->isVisible() && quickItem->opacity() > 0;
    // Popups: "visible" property
    QObject* object = find(objectName);
    return object && object->property("visible").toBool();
}

bool UiApp::clickAt(QPointF scenePos)
{
    if (!mainWindow)
        return false;
    if (scenePos.x() < 0 || scenePos.y() < 0 || scenePos.x() >= mainWindow->width() || scenePos.y() >= mainWindow->height())
        return false;
    QTest::mouseClick(mainWindow, Qt::LeftButton, Qt::NoModifier, scenePos.toPoint());
    QTest::qWait(50);
    return true;
}

// Topmost visible item at a point of the window that would get a mouse
// click: controls, mouse areas, open popups and the dimmed window behind
// modal popups (children of the overlay)
static QQuickItem* topmostItemAt(QQuickItem* item, QPointF scenePos)
{
    // Later children and higher z are on top
    QList<QQuickItem*> children = item->childItems();
    std::stable_sort(children.begin(), children.end(), [](QQuickItem* a, QQuickItem* b) { return a->z() < b->z(); });
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        QQuickItem* child = *it;
        if (!child->isVisible() || child->opacity() <= 0)
            continue;
        const bool inside = child->contains(child->mapFromScene(scenePos));
        if (child->clip() && !inside)
            continue;
        if (QQuickItem* hit = topmostItemAt(child, scenePos))
            return hit;
        // The overlay itself takes all clicks, only open popups and dimmed
        // windows on it count
        const bool takesClicks = item->inherits("QQuickOverlay")
                                 || (!child->inherits("QQuickOverlay")
                                     && ((child->acceptedMouseButtons() & Qt::LeftButton)
                                         || child->inherits("QQuickControl")));
        if (inside && takesClicks)
            return child;
    }
    return nullptr;
}

void UiApp::scrollIntoView(QQuickItem* item)
{
    // The nearest Flickable, list views scroll with positionViewAtIndex()
    for (QQuickItem* view = item->parentItem(); view; view = view->parentItem()) {
        if (!view->inherits("QQuickFlickable"))
            continue;
        if (view->inherits("QQuickItemView"))
            return;
        const qreal y = item->mapToItem(view, QPointF(0, 0)).y();
        if (y >= 0 && y + item->height() <= view->height())
            return;
        const qreal contentY = view->property("contentY").toReal();
        const qreal maxY = qMax(0.0, view->property("contentHeight").toReal() - view->height());
        const qreal target = contentY + y - qMax(0.0, (view->height() - item->height()) / 2);
        view->setProperty("contentY", qBound(0.0, target, maxY));
        settle(100);
        return;
    }
}

bool UiApp::click(QQuickItem* item, Qt::MouseButton button)
{
    if (!item || !mainWindow || !item->isVisible()) {
        qWarning().noquote() << "Cannot click" << describe(item) << "- not visible";
        return false;
    }
    scrollIntoView(item);
    const QPointF center = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    if (center.x() < 0 || center.y() < 0 || center.x() >= mainWindow->width() || center.y() >= mainWindow->height()) {
        qWarning().noquote() << "Cannot click" << describe(item) << "- center" << center << "outside the window";
        return false;
    }

    // Something else (a popup, the dimmed window behind a drawer, an item
    // scrolled over it) would get the click
    QQuickItem* hit = topmostItemAt(mainWindow->contentItem(), center);
    bool hitsItem = false;
    for (QQuickItem* i = hit; i; i = i->parentItem())
        hitsItem |= (i == item);
    if (hit && !hitsItem) {
        qWarning().noquote() << "Cannot click" << describe(item) << "- covered by" << describe(hit);
        return false;
    }

    QTest::mouseClick(mainWindow, button, Qt::NoModifier, center.toPoint());
    QTest::qWait(50);
    return true;
}

bool UiApp::click(const QString& objectName)
{
    return click(item(objectName));
}

bool UiApp::pressAndHold(QQuickItem* item)
{
    if (!item || !mainWindow || !item->isVisible())
        return false;
    const QPoint center = item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
    QTest::mousePress(mainWindow, Qt::LeftButton, Qt::NoModifier, center);
    QTest::qWait(1000);
    QTest::mouseRelease(mainWindow, Qt::LeftButton, Qt::NoModifier, center);
    QTest::qWait(50);
    return true;
}

void UiApp::key(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    QTest::keyClick(mainWindow, key, modifiers);
    QTest::qWait(50);
}

void UiApp::keySequence(const QKeySequence& sequence)
{
    QTest::keySequence(mainWindow, sequence);
    QTest::qWait(50);
}

bool UiApp::wheel(QQuickItem* item, int angleDelta)
{
    if (!item || !mainWindow || !item->isVisible())
        return false;
    const QPointF center = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    QWheelEvent event(center, mainWindow->mapToGlobal(center), QPoint(), QPoint(0, angleDelta),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(mainWindow, &event);
    QTest::qWait(50);
    return true;
}

bool UiApp::waitFor(const std::function<bool()>& condition, int timeout)
{
    return QTest::qWaitFor(condition, QDeadlineTimer(timeout));
}

bool UiApp::waitVisible(const QString& objectName, int timeout)
{
    return waitFor([&] { return isVisible(objectName); }, timeout);
}

bool UiApp::waitHidden(const QString& objectName, int timeout)
{
    return waitFor([&] { return !isVisible(objectName); }, timeout);
}

void UiApp::resize(int width, int height)
{
    mainWindow->resize(width, height);
    waitFor([&] { return mainWindow->width() == width && mainWindow->height() == height; }, 2000);
    QQuickTest::qWaitForPolish(mainWindow);
    settle();

    // A frame in the new size, otherwise a screenshot may show the old one
    QSignalSpy frames(mainWindow, &QQuickWindow::frameSwapped);
    mainWindow->update();
    frames.wait(1000);
}

void UiApp::settle(int ms)
{
    QTest::qWait(ms);
}

static bool isEffectivelyVisible(const QQuickItem* item)
{
    for (const QQuickItem* i = item; i; i = i->parentItem()) {
        if (!i->isVisible() || i->opacity() <= 0)
            return false;
    }
    return true;
}

// The item or one of its parents clips its content or scrolls it
static bool isClippedByParent(const QQuickItem* item)
{
    for (const QQuickItem* parent = item->parentItem(); parent; parent = parent->parentItem()) {
        if (parent->clip() || parent->inherits("QQuickFlickable"))
            return true;
    }
    return false;
}

QStringList UiApp::itemsOutsideWindow() const
{
    QStringList result;
    if (!mainWindow)
        return result;
    const qreal windowWidth = mainWindow->width();

    forEachItem([&](QQuickItem* item) {
        if (item == mainWindow->contentItem() || item->width() <= 0 || item->height() <= 0)
            return;
        if (!isEffectivelyVisible(item) || isClippedByParent(item))
            return;

        // Plain Items show nothing themselves (their children are checked),
        // shadows of the Material style may reach beyond the window
        const QString className = item->metaObject()->className();
        const bool isPlainItem = className == "QQuickItem" || className.startsWith("QQuickItem_QML_");
        const bool isShadow = className.contains("Shadow") || className.contains("ElevationEffect");
        if (isPlainItem || isShadow || item->inherits("QQuickShaderEffect") || item->inherits("QQuickShaderEffectSource"))
            return;

        QRectF rect = item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));

        // Text that doesn't fit and is neither elided nor wrapped
        if (item->inherits("QQuickText") && !item->clip()) {
            const qreal paintedWidth = item->property("paintedWidth").toReal();
            if (paintedWidth > item->width() + 1) {
                const int alignment = item->property("horizontalAlignment").toInt();
                qreal left = rect.left();
                if (alignment == Qt::AlignHCenter)
                    left = rect.center().x() - paintedWidth / 2;
                else if (alignment == Qt::AlignRight)
                    left = rect.right() - paintedWidth;
                rect.setLeft(left);
                rect.setWidth(paintedWidth);
            }
        }

        if (rect.left() < -1 || rect.right() > windowWidth + 1)
            result << QString("%1 at x %2..%3 (window width %4)")
                          .arg(describe(item))
                          .arg(rect.left(), 0, 'f', 0)
                          .arg(rect.right(), 0, 'f', 0)
                          .arg(windowWidth);
    });
    return result;
}

QStringList UiApp::controlsWithoutAccessibleName() const
{
    QStringList result;
    forEachItem([&](QQuickItem* item) {
        const bool isControl = item->inherits("QQuickAbstractButton")
                               || item->inherits("QQuickSlider")
                               || item->inherits("QQuickComboBox")
                               || item->inherits("QQuickTextInput");
        if (!isControl || !item->isEnabled() || !isEffectivelyVisible(item))
            return;

        QString name;
        if (QAccessibleInterface* accessible = QAccessible::queryAccessibleInterface(item))
            name = accessible->text(QAccessible::Name);
        if (name.trimmed().isEmpty())
            result << describe(item);
    });
    return result;
}

// All pixels have the color of the first one
static bool isBlank(const QImage& image)
{
    if (image.isNull())
        return true;
    const QRgb first = image.pixel(0, 0);
    for (int y = 0; y < image.height(); y += 4) {
        for (int x = 0; x < image.width(); x += 4) {
            if (image.pixel(x, y) != first)
                return false;
        }
    }
    return true;
}

void UiApp::screenshot(const QString& name) const
{
    const QString dir = qEnvironmentVariable("WELLE_TEST_SCREENSHOT_DIR");
    if (dir.isEmpty() || !mainWindow)
        return;
    QDir().mkpath(dir);

    // With OpenGL on the offscreen platform a grab is sometimes empty (one
    // color), then the next frame is grabbed
    QImage image;
    for (int attempt = 0; attempt < 3; attempt++) {
        QSignalSpy frames(mainWindow, &QQuickWindow::frameSwapped);
        mainWindow->update();
        frames.wait(1000);
        image = mainWindow->grabWindow();
        if (!isBlank(image))
            break;
        settle(200);
    }
    const QString file = QDir(dir).filePath(name + ".png");
    if (image.isNull() || !image.save(file))
        qWarning() << "Cannot save screenshot" << file;
}

QStringList UiApp::takeQmlMessages()
{
    QMutexLocker lock(&qmlMessagesMutex);
    QStringList result = qmlMessages;
    qmlMessages.clear();
    return result;
}

QString UiApp::describe(const QObject* object)
{
    if (!object)
        return "(null)";
    QString result = object->metaObject()->className();
    result.remove(QRegularExpression("_QML(TYPE)?_\\d+$"));
    if (!object->objectName().isEmpty())
        result += " \"" + object->objectName() + "\"";
    const QString text = object->property("text").toString().trimmed();
    if (!text.isEmpty())
        result += " text \"" + text.left(40) + "\"";
    return result;
}
