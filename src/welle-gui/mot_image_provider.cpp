/******************************************************************************\
 * Copyright (c) 2016 Albrecht Lohofener <albrechtloh@gmx.de>
 *
 * Author(s):
 * Albrecht Lohofener
 *
 * Description:
 * The classes provides a list to store the station and channel information
 *
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
#include "mot_image_provider.h"

#include <iostream>
#include <stdio.h>
#include <QStandardPaths>

CMOTImageProvider::CMOTImageProvider(): QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap CMOTImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    (void) requestedSize;

    // Find the corresponding picture in list
    for (auto const& picture : pictureList) {
        if(picture->name == id) { // found picture
            *size = QSize(picture->data.width(), picture->data.height());
//            std::clog  << "SLS name: " << id.toStdString() << std::endl;
            return picture->data;
        }
    }

//    std::clog  << "SLS name: Cannot find " << id.toStdString() << std::endl;

    // TODO: There should be a better solution
    QImage emptyImage;
    emptyImage = QImage(320, 240, QImage::Format_Alpha8);
    emptyImage.fill(Qt::transparent);
    return QPixmap::fromImage(emptyImage);
}

void CMOTImageProvider::setPixmap(QPixmap pictureData, QString pictureName)
{
    // Check if picture is already in list
    for (auto const& picture : pictureList)
        if(picture->name == pictureName) {
            // Replace picture
            picture->setData(pictureData);
            return;
        }

    // New picture
    pictureList.push_front(std::make_shared<motPicture>(pictureData, pictureName));
}

void CMOTImageProvider::clear()
{
    pictureList.clear();
}

void CMOTImageProvider::saveAll()
{
    for (auto const& picture : pictureList)
        picture->save();
}

motPicture::motPicture(QPixmap data, QString name)
{
    this->data = data;
    this->name = name;
}

void motPicture::setData(QPixmap data)
{
    this->data = data;
}

void motPicture::save()
{
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    // Replace all "/" by "_"
    QString filename = name;
    filename.replace("/", "_");

    // Add home directory and "MOT" preffix
    filename = homePath + "/MOT" + filename;

    std::clog  << "SLS: Saving picture \"" << filename.toStdString() << "\"" << std::endl;
    data.save(filename);
}
