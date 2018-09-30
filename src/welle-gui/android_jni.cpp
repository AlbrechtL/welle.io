/*
 *    Copyright (C) 2017
 *    Matthias Benesch (twoof7@gmail.com)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
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

#include <QDebug>
#include <QSettings>
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <QMetaObject>
#include <android/bitmap.h>

#include "gui_helper.h"
#include "input_factory.h"
#include "android_jni.h"
#include "audio_output.h"
#include "rtl_tcp.h"
#include "dab-constants.h"
#include "msc-handler.h"

#ifdef Q_OS_ANDROID
#include "jni.h"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_setLanguage(JNIEnv *env, jobject, jstring jLang)
{
    if (jLang == NULL)
        return;
    QString language(env->GetStringUTFChars(jLang, 0));
    qDebug() << "AndroidJNI:" <<  "setLanguage" << ":" << language;

    CGUI::AddTranslator(language);
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_openTcpConnection(JNIEnv *env, jobject, jstring host, jint port)
{
    if (host == NULL)
        return;
    QString qHost(env->GetStringUTFChars(host, 0));
    qDebug() << "AndroidJNI:" <<  "openTcpConnection" << ":" << qHost << ":" << port;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "openTcpConnection",
                              Qt::QueuedConnection,
                              Q_ARG(QString, qHost),
                              Q_ARG(int, port));
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_closeTcpConnection(JNIEnv *, jobject)
{
    qDebug() << "AndroidJNI:" <<  "closeTcpConnection";
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "closeTcpConnection",
                              Qt::QueuedConnection);
}

JNIEXPORT jboolean JNICALL Java_io_welle_welle_DabService_isFavoriteStation(JNIEnv *env, jobject, jstring jStation, jstring jChannel)
{
    if (jStation == NULL || jChannel == NULL)
        return JNI_FALSE;
    QString station(env->GetStringUTFChars(jStation, 0));
    QString channel(env->GetStringUTFChars(jChannel, 0));
    qDebug() << "AndroidJNI:" <<  "isFavoriteStation" << "station:" << station
             << "channel:" << channel;
    bool result = CAndroidJNI::getInstance().isFavoriteStation(station, channel);
    return  result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_setFavoriteStation(JNIEnv *env, jobject, jstring jStation, jstring jChannel, jboolean jValue)
{
    if (jStation == NULL || jChannel == NULL)
        return;
    QString station(env->GetStringUTFChars(jStation, 0));
    QString channel(env->GetStringUTFChars(jChannel, 0));
    bool value = (jValue == JNI_TRUE);
    qDebug() << "AndroidJNI:" <<  "setFavoriteStation" << "station:" << station
             << "channel:" << channel << "value:" << value;
    CAndroidJNI::getInstance().setFavoriteStation(station, channel, value);
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_play(JNIEnv *env, jobject, jstring jStation, jstring jChannel)
{
    if (jStation == NULL || jChannel == NULL)
        return;
    QString station(env->GetStringUTFChars(jStation, 0));
    QString channel(env->GetStringUTFChars(jChannel, 0));
    qDebug() << "AndroidJNI:" <<  "play" << "station:"
             << station << "channel:" << channel;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "play",
                              Qt::QueuedConnection,
                              Q_ARG(QString, station),
                              Q_ARG(QString, channel));
}

JNIEXPORT jstring JNICALL Java_io_welle_welle_DabService_lastStation(JNIEnv *env, jobject)
{
    qDebug() << "AndroidJNI:" <<  "getLastStation";
    QString station = CAndroidJNI::getLastStation();
    return env->NewStringUTF(station.toLatin1().constData());
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_duckPlayback(JNIEnv *, jobject, jboolean jDuck)
{
    bool duck = (jDuck == JNI_TRUE);
    qDebug() << "AndroidJNI:" <<  "duckPlayback is:" << duck;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "duckPlayback",
                              Qt::QueuedConnection, Q_ARG(bool, duck));
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_pausePlayback(JNIEnv *, jobject)
{
    qDebug() << "AndroidJNI:" <<  "pausePlayback";
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "pausePlayback",
                              Qt::QueuedConnection);
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_stopPlayback(JNIEnv *, jobject)
{
    qDebug() << "AndroidJNI:" <<  "stopPlayback";
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "stopPlayback",
                              Qt::QueuedConnection);
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_nextChannel(JNIEnv *, jobject)
{
    qDebug() << "AndroidJNI:" <<  "nextChannel";
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "nextChannel",
                              Qt::QueuedConnection);
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_setManualChannel(JNIEnv *env, jobject, jstring jChannel)
{
    if (jChannel == NULL)
        return;
    QString channel(env->GetStringUTFChars(jChannel, 0));
    qDebug() << "AndroidJNI:" <<  "setManualChannel" << "channel:" << channel;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "setManualChannel",
                              Qt::QueuedConnection,
                              Q_ARG(QString, channel));
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_startChannelScan(JNIEnv *, jobject)
{
    qDebug() << "AndroidJNI:" <<  "startChannelScan";
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "startChannelScan",
                              Qt::QueuedConnection);
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_stopChannelScan(JNIEnv *, jobject)
{
    qDebug() << "AndroidJNI:" <<  "stopChannelScan";
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "stopChannelScan",
                              Qt::QueuedConnection);
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_setErrorMessage(JNIEnv *env, jobject, jstring jText)
{
    if (jText == NULL)
        return;
    QString text(env->GetStringUTFChars(jText, 0));
    qDebug() << "AndroidJNI:" <<  "setErrorMessage" << "text:" << text;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "setErrorMessage",
                              Qt::QueuedConnection,
                              Q_ARG(QString, text));
}

#ifdef __cplusplus
}
#endif
#endif

CAndroidJNI& CAndroidJNI::getInstance()
{
    static CAndroidJNI instance;
    return instance;
}

CAndroidJNI::CAndroidJNI(QObject *parent)
    : QObject(parent)
    , mRadioController(NULL)
    , mFavoriteList("favorite")
{
    qDebug() << "AndroidJNI:" <<  "Created";
}

CAndroidJNI::~CAndroidJNI()
{
    qDebug() << "AndroidJNI:" <<  "Deleted";
}

void CAndroidJNI::setRadioController(CRadioController *radioController)
{
    if (!radioController)
        return;

    qDebug() << "AndroidJNI:" <<  "Set RadioController";
    this->mRadioController = radioController;
    connect(radioController, &CRadioController::motChanged,
            this, &CAndroidJNI::updateMOT);
    connect(radioController, &CRadioController::deviceReady,
            this, &CAndroidJNI::deviceReady);
    connect(radioController, &CRadioController::deviceClosed,
            this, &CAndroidJNI::deviceClosed);
    connect(radioController, &CRadioController::stationsCleared,
            this, &CAndroidJNI::clearStations);
    connect(radioController, &CRadioController::foundStation,
            this, &CAndroidJNI::foundStation);
    connect(radioController, &CRadioController::scanStopped,
            this, &CAndroidJNI::channelScanStopped);
    connect(radioController, &CRadioController::scanProgress,
            this, &CAndroidJNI::channelScanProgress);
    connect(radioController, &CRadioController::showErrorMessage,
            this, &CAndroidJNI::showErrorMessage);
    connect(radioController, &CRadioController::showInfoMessage,
            this, &CAndroidJNI::showInfoMessage);

    serviceReady();

    // Get stations
    clearStations();
    foreach (StationElement *s, mRadioController->stations()) {
        addStation(s->getStationName(), s->getChannelName());
    }

    // Read favorite stations from settings
    clearFavoriteStations();
    mFavoriteList.reset();
    mFavoriteList.loadStations();
    foreach (StationElement *s, mFavoriteList.getList()) {
        addFavoriteStation(s->getStationName(), s->getChannelName());
    }

    // Update GUI data
    //ToDo updateGuiData(mRadioController->guiData());

    // Update MOT image
    updateMOT(mRadioController->mot());
}

bool CAndroidJNI::openTcpConnection(QString host, int port)
{
    qDebug() << "AndroidJNI:" <<  "Open TCP connection:" << host << ":" << port;
    if(mRadioController) {
        CRTL_TCP_Client *device = new CRTL_TCP_Client(*mRadioController);
        device->setIP(host);
        device->setPort(port);
        mRadioController->openDevice(device);
        return true;
    }
    return false;
}

void CAndroidJNI::closeTcpConnection()
{
    qDebug() << "AndroidJNI:" <<  "Close TCP connection";
    if(mRadioController) {
        mRadioController->closeDevice();
    } else {
        deviceClosed();
    }
}

bool CAndroidJNI::isFavoriteStation(QString station, QString channel)
{
    qDebug() << "AndroidJNI:" <<  "Is favorite station:" << station
             << "channel:" << channel;
    return mFavoriteList.contains(station, channel);
}

void CAndroidJNI::setFavoriteStation(QString station, QString channel, bool value)
{
    qDebug() << "AndroidJNI:" <<  "Set favorite station:" << station
             << "channel:" << channel << "value:" << value;
    if (value) {
        mFavoriteList.append(station, channel);
        addFavoriteStation(station, channel);
    } else {
        mFavoriteList.remove(station, channel);
        removeFavoriteStation(station, channel);
    }
    mFavoriteList.saveStations();
}

void CAndroidJNI::startChannelScan(void)
{
    qDebug() << "AndroidJNI:" <<  "Start channel scan";
    if(mRadioController)
        mRadioController->startScan();
}

void CAndroidJNI::stopChannelScan(void)
{
    qDebug() << "AndroidJNI:" <<  "Stop channel scan";
    if(mRadioController)
        mRadioController->stopScan();
}

void CAndroidJNI::updateGuiData(QVariantMap GUIData)
{
    //qDebug() << "AndroidJNI:" <<  "Display Update";

    jint jStatus = GUIData["Status"].toInt();
    QAndroidJniObject jChannel = QAndroidJniObject::fromString(GUIData["Channel"].toString());
    QAndroidJniObject jStation = QAndroidJniObject::fromString(GUIData["Station"].toString());
    QAndroidJniObject jTitle = QAndroidJniObject::fromString(GUIData["Title"].toString());
    QAndroidJniObject jLabel = QAndroidJniObject::fromString(GUIData["Text"].toString());
    QAndroidJniObject jStationType = QAndroidJniObject::fromString(GUIData["StationType"].toString());

    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "updateGuiData",
                                              "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                                              jStatus,
                                              jChannel.object<jstring>(),
                                              jStation.object<jstring>(),
                                              jTitle.object<jstring>(),
                                              jLabel.object<jstring>(),
                                              jStationType.object<jstring>());
}

void CAndroidJNI::updateMOT(QImage img)
{
    if (img.isNull()) {
        // Update Android Bitmap
        QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                                  "updateMOT",
                                                  "(Landroid/graphics/Bitmap;)V",
                                                  NULL);
        return;
    }

    // Convert image
    QImage image = (img.format() == QImage::Format_RGBA8888) ? img
                                                             : img.convertToFormat(QImage::Format_RGBA8888);

    // Create Android Bitmap
    QAndroidJniObject config = QAndroidJniObject::getStaticObjectField("android/graphics/Bitmap$Config",
                                                                       "ARGB_8888",
                                                                       "Landroid/graphics/Bitmap$Config;");

    QAndroidJniObject bitmap = QAndroidJniObject::callStaticObjectMethod("android/graphics/Bitmap",
                                                                         "createBitmap",
                                                                         "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;",
                                                                         img.width(),
                                                                         img.height(),
                                                                         config.object());
    // Copy QImage to Android Bitmap
    QAndroidJniEnvironment env;
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap.object(), &info) != ANDROID_BITMAP_RESULT_SUCCESS)
        return;

    if (info.format!= ANDROID_BITMAP_FORMAT_RGBA_8888)
        return;

    void *pixels;
    if (AndroidBitmap_lockPixels(env, bitmap.object(), &pixels) != ANDROID_BITMAP_RESULT_SUCCESS)
        return;

    if (info.stride == uint32_t(image.bytesPerLine())) {
        memcpy(pixels, image.constBits(), info.stride * info.height);
    } else {
        uchar *bmpPtr = static_cast<uchar *>(pixels);
        const unsigned width = std::min(info.width, (uint)image.width());
        const unsigned height = std::min(info.height, (uint)image.height());
        for (unsigned y = 0; y < height; y++, bmpPtr += info.stride)
            memcpy(bmpPtr, image.constScanLine(y), width);
    }

    if (AndroidBitmap_unlockPixels(env, bitmap.object()) != ANDROID_BITMAP_RESULT_SUCCESS)
        return;

    // Update Android Bitmap
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "updateMOT",
                                              "(Landroid/graphics/Bitmap;)V",
                                              bitmap.object());
}

void CAndroidJNI::deviceReady(void)
{
    qDebug() << "AndroidJNI:" <<  "Device ready";
    if(!mRadioController)
        return;

    // Set AGC & Gain
    QSettings Settings;
    mRadioController->setHwAGC(Settings.value("enableHwAGCState", false).toBool());
    mRadioController->setGain(Settings.value("manualGainState", 0).toInt());
    mRadioController->setAGC(Settings.value("enableAGCState", true).toBool());

    // Notify service
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "deviceReady", "()V");
}

void CAndroidJNI::deviceClosed(void)
{
    qDebug() << "AndroidJNI:" <<  "Device closed";

    // Notify service
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "deviceClosed", "()V");
}

void CAndroidJNI::foundStation(QString station, QString channel)
{
    qDebug() << "AndroidJNI:" <<  "Found station:" << station
             << "channel:" << channel;
    addStation(station, channel);
}

void CAndroidJNI::play(QString station, QString channel)
{
    qDebug() << "AndroidJNI:" <<  "Play station:" << station
             << "channel:" << channel;
    if(mRadioController)
        mRadioController->play(channel, station);
}

QString CAndroidJNI::getLastStation(void)
{
    qDebug() << "AndroidJNI:" <<  "Get last station";
    QSettings settings;
    QStringList lastStation = settings.value("lastchannel").toStringList();
    return lastStation.size() == 2 ? lastStation.first() + lastStation.last()
                                   : "";
}

void CAndroidJNI::duckPlayback(bool duck)
{
    qDebug() << "AndroidJNI:" << (duck ? "Duck" : "Unduck") << "playback";
    if (mRadioController)
        mRadioController->setVolume(duck ? 0.5 : 1);
}

void CAndroidJNI::pausePlayback()
{
    qDebug() << "AndroidJNI:" <<  "Pause playback";
    if(mRadioController)
        mRadioController->stop();
}

void CAndroidJNI::stopPlayback()
{
    qDebug() << "AndroidJNI:" <<  "Stop playback";
    if(mRadioController)
        mRadioController->stop();
}

void CAndroidJNI::nextChannel()
{
    qDebug() << "AndroidJNI:" <<  "Next channel";
//TODO    if(mRadioController)
//        mRadioController->NextChannel(false);
}

void CAndroidJNI::setManualChannel(QString channel)
{
    qDebug() << "AndroidJNI:" <<  "Set channel:" << channel;
    if(mRadioController)
        mRadioController->setManualChannel(channel);
}

void CAndroidJNI::setErrorMessage(QString text)
{
    qDebug() << "AndroidJNI:" <<  "Set error msg:" << text;
    if(mRadioController)
        mRadioController->setErrorMessage(text);
}

void CAndroidJNI::serviceReady(void)
{
    qDebug() << "AndroidJNI:" <<  "Service ready";
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "serviceReady", "()V");
}

void CAndroidJNI::addStation(QString station, QString channel)
{
    qDebug() << "AndroidJNI:" <<  "Add station:" << station
             << "channel:" << channel;

    QAndroidJniObject jStation = QAndroidJniObject::fromString(station);
    QAndroidJniObject jChannel = QAndroidJniObject::fromString(channel);
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "addStation",
                                              "(Ljava/lang/String;Ljava/lang/String;)V",
                                              jStation.object<jstring>(),
                                              jChannel.object<jstring>());
}

void CAndroidJNI::addFavoriteStation(QString station, QString channel)
{
    qDebug() << "AndroidJNI:" <<  "Add favorite station:" << station
             << "channel:" << channel;

    QAndroidJniObject jStation = QAndroidJniObject::fromString(station);
    QAndroidJniObject jChannel = QAndroidJniObject::fromString(channel);
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabMediaService",
                                              "addFavoriteStation",
                                              "(Ljava/lang/String;Ljava/lang/String;)V",
                                              jStation.object<jstring>(),
                                              jChannel.object<jstring>());
}

void CAndroidJNI::removeFavoriteStation(QString station, QString channel)
{
    qDebug() << "AndroidJNI:" <<  "Remove favorite station:" << station
             << "channel:" << channel;

    QAndroidJniObject jStation = QAndroidJniObject::fromString(station);
    QAndroidJniObject jChannel = QAndroidJniObject::fromString(channel);
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabMediaService",
                                              "removeFavoriteStation",
                                              "(Ljava/lang/String;Ljava/lang/String;)V",
                                              jStation.object<jstring>(),
                                              jChannel.object<jstring>());
}

void CAndroidJNI::clearFavoriteStations()
{
    qDebug() << "AndroidJNI:" <<  "Clear favorite stations";
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabMediaService",
                                              "clearFavoriteStations", "()V");
}

void CAndroidJNI::clearStations(void)
{
    qDebug() << "AndroidJNI:" <<  "Clear stations";
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "clearStations", "()V");
}

void CAndroidJNI::channelScanStopped(void)
{
    qDebug() << "AndroidJNI:" <<  "Channel scan stopped";
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "channelScanStopped",
                                              "()V");
}

void CAndroidJNI::channelScanProgress(int Progress)
{
    qDebug() << "AndroidJNI:" <<  "Channel scan progress:" << Progress;
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "channelScanProgress",
                                              "(I)V", Progress);
    //if (mRadioController)
        //ToDo updateGuiData(mRadioController->guiData());
}

void CAndroidJNI::showErrorMessage(QString Text)
{
    qDebug() << "AndroidJNI:" <<  "Show error:" << Text;
    QAndroidJniObject jText = QAndroidJniObject::fromString(Text);
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "showErrorMessage",
                                              "(Ljava/lang/String;)V",
                                              jText.object<jstring>());
}

void CAndroidJNI::showInfoMessage(QString Text)
{
    qDebug() << "AndroidJNI:" <<  "Show info:" << Text;
    QAndroidJniObject jText = QAndroidJniObject::fromString(Text);
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "showInfoMessage",
                                              "(Ljava/lang/String;)V",
                                              jText.object<jstring>());
}
