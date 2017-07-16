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
#include <QAndroidJniObject>
#include <QMetaObject>

#include "CInputFactory.h"
#include "CAndroidJNI.h"
#include "CAudio.h"
#include "CRTL_SDR.h"
#include "DabConstants.h"
#include "msc-handler.h"

#ifdef Q_OS_ANDROID
#include "jni.h"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_openUsbDevice(JNIEnv *env, jobject, jint fd, jstring path)
{
    QString qPath(env->GetStringUTFChars(path, 0));
    qDebug() << "AndroidJNI:" <<  "openUsbDevice" << " fd:" << fd << " path: " << qPath;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "openUsbDevice",
                              Qt::QueuedConnection,
                              Q_ARG(int, fd),
                              Q_ARG(QString, qPath));
}

JNIEXPORT jboolean JNICALL Java_io_welle_welle_DabService_isFavoriteStation(JNIEnv *env, jobject, jstring stationId)
{
    QString qStationId(env->GetStringUTFChars(stationId, 0));
    qDebug() << "AndroidJNI:" <<  "isFavoriteStation" << " id: " << qStationId;
    return CAndroidJNI::getInstance().isFavoriteStation(qStationId) ? JNI_TRUE
                                                                    : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_addFavoriteStation(JNIEnv *env, jobject, jstring stationId, jstring stationName)
{
    QString qStationId(env->GetStringUTFChars(stationId, 0));
    QString qStationName(env->GetStringUTFChars(stationName, 0));
    qDebug() << "AndroidJNI:" <<  "addFavoriteStation" << " id: " << qStationId
             << " station: " << qStationName;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "addFavoriteStation",
                              Qt::QueuedConnection,
                              Q_ARG(QString, qStationId),
                              Q_ARG(QString, qStationName));
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_removeFavoriteStation(JNIEnv *env, jobject, jstring stationId)
{
    QString qStationId(env->GetStringUTFChars(stationId, 0));
    qDebug() << "AndroidJNI:" <<  "addFavoriteStation" << " id: " << qStationId;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "removeFavoriteStation",
                              Qt::QueuedConnection,
                              Q_ARG(QString, qStationId));
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_playStationById(JNIEnv *env, jobject, jstring stationId)
{
    QString qStationId(env->GetStringUTFChars(stationId, 0));
    qDebug() << "AndroidJNI:" <<  "playStationById" << " id: " << qStationId;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "playStationById",
                              Qt::QueuedConnection, Q_ARG(QString, qStationId));
}

JNIEXPORT jstring JNICALL Java_io_welle_welle_DabService_lastStation(JNIEnv *env, jobject)
{
    qDebug() << "AndroidJNI:" <<  "getLastStation";
    QString station = CAndroidJNI::getLastStation();
    //QAndroidJniObject jStation = QAndroidJniObject::fromString(station);
    //return jStation.object<jstring>();

    return env->NewStringUTF(station.toLatin1().constData());
}

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_duckPlayback(JNIEnv *, jobject, jboolean duck)
{
    qDebug() << "AndroidJNI:" <<  "duckPlayback is:" << duck;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "duckPlayback",
                              Qt::QueuedConnection, Q_ARG(bool, (duck == JNI_TRUE)));
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

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_setManualChannel(JNIEnv *env, jobject, jstring channelName)
{
    QString qChannelName(env->GetStringUTFChars(channelName, 0));
    qDebug() << "AndroidJNI:" <<  "setManualChannel" << " channel: " << qChannelName;
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "setManualChannel",
                              Qt::QueuedConnection,
                              Q_ARG(QString, qChannelName));
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

JNIEXPORT void JNICALL Java_io_welle_welle_DabService_saveStations(JNIEnv *, jobject)
{
    qDebug() << "AndroidJNI:" <<  "saveStations";
    QMetaObject::invokeMethod(&CAndroidJNI::getInstance(), "saveStations",
                              Qt::QueuedConnection);
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
    , mStationList()
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
    qDebug() << "AndroidJNI:" <<  "Set RadioController";
    this->mRadioController = radioController;

    connect(radioController, &CRadioController::GUIDataChanged,
            this, &CAndroidJNI::updateGuiData);
    connect(radioController, &CRadioController::DeviceReady,
            this, &CAndroidJNI::deviceReady);
    connect(radioController, &CRadioController::FoundStation,
            this, &CAndroidJNI::foundStation);
    connect(radioController, &CRadioController::ScanStopped,
            this, &CAndroidJNI::channelScanStopped);
    connect(radioController, &CRadioController::ScanProgress,
            this, &CAndroidJNI::channelScanProgress);
    connect(radioController, &CRadioController::showErrorMessage,
            this, &CAndroidJNI::showErrorMessage);
    connect(radioController, &CRadioController::showInfoMessage,
            this, &CAndroidJNI::showInfoMessage);

    serviceReady();
}

bool CAndroidJNI::openUsbDevice(int fd, QString path)
{
    qDebug() << "AndroidJNI:" <<  "Open USB device:" << path << "fd:" << fd;
    if(mRadioController) {
        CRTL_SDR *device = new CRTL_SDR(*mRadioController, fd, path);
        mRadioController->setDevice(device);
        QTimer::singleShot(0, mRadioController, SLOT(onEventLoopStarted()));
        return true;
    }
    return false;
}

StationElement *CAndroidJNI::findStation(QString stationId)
{
    QString sId = stationId.left(4);
    QString channelName = stationId.mid(4);
    return mStationList.find(sId, channelName);
}

bool CAndroidJNI::isFavoriteStation(QString stationId)
{
    qDebug() << "AndroidJNI:" <<  "Is favorite station:" + stationId;
    QString sId = stationId.left(4);
    QString channelName = stationId.mid(4);
    return mFavoriteList.contains(sId, channelName);
}

void CAndroidJNI::addFavoriteStation(QString stationId, QString stationName)
{
    qDebug() << "AndroidJNI:" <<  "Add favorite station:" + stationId;
    QString sId = stationId.left(4);
    QString channelName = stationId.mid(4);
    mFavoriteList.append(sId, stationName, channelName);
    saveFavoriteStations();
}

void CAndroidJNI::removeFavoriteStation(QString stationId)
{
    qDebug() << "AndroidJNI:" <<  "Remove favorite station:" + stationId;
    QString sId = stationId.left(4);
    QString channelName = stationId.mid(4);
    mFavoriteList.remove(sId, channelName);
    saveFavoriteStations();
}

void CAndroidJNI::saveFavoriteStations()
{
    qDebug() << "AndroidJNI:" <<  "Save favorite stations";
    mFavoriteList.saveStations();
    //TODO update favorite stations
}

void CAndroidJNI::saveStations()
{
    qDebug() << "AndroidJNI:" <<  "Save stations";
    mStationList.saveStations();
}

void CAndroidJNI::startChannelScan(void)
{
    qDebug() << "AndroidJNI:" <<  "Start channel scan";
    if(mRadioController)
        mRadioController->StartScan();

//    clearStationList();
}

void CAndroidJNI::stopChannelScan(void)
{
    qDebug() << "AndroidJNI:" <<  "Stop channel scan";
    if(mRadioController)
        mRadioController->StopScan();
}

void CAndroidJNI::updateGuiData(QVariantMap GUIData)
{
    //qDebug() << "AndroidJNI:" <<  "Display Update";

    jint jStatus = GUIData["Status"].toInt();
    QAndroidJniObject jChannel = QAndroidJniObject::fromString(GUIData["Channel"].toString());
    QAndroidJniObject jStation = QAndroidJniObject::fromString(GUIData["Station"].toString());
    QAndroidJniObject jLabel = QAndroidJniObject::fromString(GUIData["Label"].toString());
    QAndroidJniObject jStationType = QAndroidJniObject::fromString(GUIData["StationType"].toString());

    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "updateGuiData",
                                              "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                                              jStatus,
                                              jChannel.object<jstring>(),
                                              jStation.object<jstring>(),
                                              jLabel.object<jstring>(),
                                              jStationType.object<jstring>());
}

void CAndroidJNI::deviceReady(void)
{
    qDebug() << "AndroidJNI:" <<  "Device ready";
    if(!mRadioController)
        return;

    // Read stations from settings
    mStationList.reset();
    mStationList.loadStations();
    foreach (QObject *obj, mStationList.getList()) {
        StationElement *s = (StationElement*)obj;
        addStation(s->getStationId(), s->getStationName(), s->getChannelName());
    }

    // Read favorite stations from settings
    mFavoriteList.reset();
    mFavoriteList.loadStations();
    mFavoriteList.sort();
    //TODO update favorite stations

    // Set AGC & Gain
    QSettings Settings;
    mRadioController->setHwAGC(Settings.value("enableHwAGCState", false).toBool());
    mRadioController->setGain(Settings.value("manualGainState", 0).toInt());
    mRadioController->setAGC(Settings.value("enableAGCState", true).toBool());

    // Notify service
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "deviceReady", "()V");
}

void CAndroidJNI::foundStation(QString stationId, QString stationName,
                               QString channelName)
{
    qDebug() << "AndroidJNI:" <<  "Found station:" << stationName
             << "(" << qPrintable(stationId) << ")" << "channel:" << channelName;
    //	Add new station into list
    if (!mStationList.contains(stationId, channelName)) {
        addStation(stationId, stationName, channelName);

        // Save the channels
        saveStations();
    }
}

void CAndroidJNI::playStationById(QString stationId)
{
    qDebug() << "AndroidJNI:" <<  "Play station id:" << stationId;
    if(mRadioController) {
        StationElement *station = findStation(stationId);
        if (station) {
            mRadioController->Play(station->getChannelName(), station->getStationName());

            QSettings Settings;
            Settings.setValue("laststation", stationId);
        } else
            qDebug() << "AndroidJNI:" <<  "Station id:" << stationId << "not found!";
    }
}

QString CAndroidJNI::getLastStation(void)
{
    qDebug() << "AndroidJNI:" <<  "Get last station";
    QSettings settings;
    QString lastStation = settings.value("laststation").toString();
    return (lastStation.isEmpty()) ? "" : lastStation;
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
        mRadioController->Pause();
}

void CAndroidJNI::stopPlayback()
{
    qDebug() << "AndroidJNI:" <<  "Stop playback";
    if(mRadioController)
        mRadioController->Stop();
}

void CAndroidJNI::nextChannel()
{
    qDebug() << "AndroidJNI:" <<  "Next channel";
//TODO    if(mRadioController)
//        mRadioController->NextChannel(false);
}

void CAndroidJNI::setManualChannel(QString ChannelName)
{
    qDebug() << "AndroidJNI:" <<  "Set channel: " << ChannelName;
    if(mRadioController)
        mRadioController->SetChannel(ChannelName, false);
}

void CAndroidJNI::serviceReady(void)
{
    qDebug() << "AndroidJNI:" <<  "Service ready";
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "serviceReady", "()V");
}

void CAndroidJNI::addStation(QString stationId, QString stationName, QString channelName)
{
    qDebug() << "AndroidJNI:" <<  "Add station:" << stationName
             << "(" << qPrintable(stationId) << ")" << "channel:" << channelName;
    mStationList.append(stationId, stationName, channelName);
    mStationList.sort();

    QAndroidJniObject jSId = QAndroidJniObject::fromString(stationId + channelName);
    QAndroidJniObject jStation = QAndroidJniObject::fromString(stationName);
    QAndroidJniObject jChannel = QAndroidJniObject::fromString(channelName);
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "addStation",
                                              "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                                              jSId.object<jstring>(),
                                              jStation.object<jstring>(),
                                              jChannel.object<jstring>());
}

void CAndroidJNI::clearStations(void)
{
    qDebug() << "AndroidJNI:" <<  "Clear stations";
    mStationList.reset();
    saveStations();
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
    qDebug() << "AndroidJNI:" <<  "Channel scan progress: " << Progress;
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "channelScanProgress",
                                              "(I)V", Progress);
    if (mRadioController)
        updateGuiData(mRadioController->GUIData());
}

void CAndroidJNI::showErrorMessage(QString Text)
{
    qDebug() << "AndroidJNI:" <<  "Show error: " << Text;
    QAndroidJniObject jText = QAndroidJniObject::fromString(Text);
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "showErrorMessage",
                                              "(Ljava/lang/String;)V",
                                              jText.object<jstring>());
}

void CAndroidJNI::showInfoMessage(QString Text)
{
    qDebug() << "AndroidJNI:" <<  "Show info: " << Text;
    QAndroidJniObject jText = QAndroidJniObject::fromString(Text);
    QAndroidJniObject::callStaticMethod<void>("io/welle/welle/DabService",
                                              "showInfoMessage",
                                              "(Ljava/lang/String;)V",
                                              jText.object<jstring>());
}
