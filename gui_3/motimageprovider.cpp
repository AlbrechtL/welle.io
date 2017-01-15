#include "motimageprovider.h"

#include <stdio.h>

MOTImageProvider::MOTImageProvider(): QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap MOTImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    if (size)
        *size = QSize(this->Pixmap_.width(), this->Pixmap_.height());

    return this->Pixmap_; // not working for some reason
}

void MOTImageProvider::setPixmap(QPixmap Pixmap)
{
    this->Pixmap_ = Pixmap;
}
