/******************************************************************************\
 * Description:
 * Station logos received by SPI: disk cache and QML image provider
 *
 * This file was generated with the help of an AI assistant (Claude Code).
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "station_logos.h"

CStationLogos::CStationLogos(QObject *parent)
    : QObject(parent)
{
    logoDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logos";

    // Only the names are read here, the images are loaded when needed
    const QStringList files = QDir(logoDirectory).entryList({"*.png"}, QDir::Files);
    for (const QString& file : files) {
        bool ok = false;
        const quint32 sId = file.section('.', 0, 0).toUInt(&ok, 16);
        if (ok)
            available.insert(sId);
    }
}

int CStationLogos::revision() const
{
    QMutexLocker lock(&mutex);
    return currentRevision;
}

bool CStationLogos::hasLogo(quint32 sId) const
{
    QMutexLocker lock(&mutex);
    return available.contains(sId);
}

QImage CStationLogos::image(quint32 sId)
{
    QMutexLocker lock(&mutex);
    if (!available.contains(sId))
        return QImage();

    auto it = images.find(sId);
    if (it == images.end()) {
        QImage img(fileName(sId));
        if (img.isNull()) {
            available.remove(sId);
            return QImage();
        }
        it = images.insert(sId, img);
    }
    return it.value();
}

void CStationLogos::addLogo(quint32 sId, const QByteArray& data)
{
    QImage img;
    if (!img.loadFromData(data)) {
        qDebug() << "StationLogos: Cannot decode logo of service" << qPrintable(QString::number(sId, 16).toUpper());
        return;
    }

    {
        QMutexLocker lock(&mutex);
        images.insert(sId, img);
        available.insert(sId);
        currentRevision++;
    }

    // PNG for all logos, also JPEG ones, to find them by name
    if (!QDir().mkpath(logoDirectory) || !img.save(fileName(sId), "PNG"))
        qDebug() << "StationLogos: Cannot save logo" << fileName(sId);

    emit revisionChanged();
}

QString CStationLogos::fileName(quint32 sId) const
{
    return logoDirectory + "/" + QString::number(sId, 16).toUpper() + ".png";
}


CStationLogoImageProvider::CStationLogoImageProvider(CStationLogos *logos)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , logos(logos)
{
}

QImage CStationLogoImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const quint32 sId = id.section('?', 0, 0).toUInt();
    QImage img = logos->image(sId);
    if (img.isNull()) {
        img = QImage(1, 1, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
    }

    if (size)
        *size = img.size();

    if (requestedSize.isValid() && !requestedSize.isEmpty())
        img = img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return img;
}
