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
#ifndef MOTIMAGEPROVIDER_H
#define MOTIMAGEPROVIDER_H

#include <memory>
#include <list>
#include <QObject>
#include <QQuickImageProvider>

class motPicture;

class CMOTImageProvider : public QQuickImageProvider
{
public:
    CMOTImageProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);

    void setPixmap(QPixmap Pixmap, QString pictureName);
    void clear();
    void saveAll();

private:
    std::list<std::shared_ptr<motPicture>> pictureList;
};


class motPicture
{
public:
    motPicture(QPixmap data, QString name);
    void setData(QPixmap data);
    void save();

    QPixmap data;
    QString name;
};

#endif // MOTIMAGEPROVIDER_H
