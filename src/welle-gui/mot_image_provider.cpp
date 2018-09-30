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

#include <stdio.h>

CMOTImageProvider::CMOTImageProvider(): QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap CMOTImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    (void) id;
    (void) requestedSize;
    if (size)
        *size = QSize(this->Pixmap_.width(), this->Pixmap_.height());

    return this->Pixmap_; // not working for some reason
}

void CMOTImageProvider::setPixmap(QPixmap Pixmap)
{
    this->Pixmap_ = Pixmap;
}
