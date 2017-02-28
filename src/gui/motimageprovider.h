#ifndef MOTIMAGEPROVIDER_H
#define MOTIMAGEPROVIDER_H

#include <QObject>
#include <QQuickImageProvider>

class MOTImageProvider : public QQuickImageProvider
{
public:
    MOTImageProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);

    void setPixmap(QPixmap Pixmap_);

private:
    QPixmap Pixmap_;
};

#endif // MOTIMAGEPROVIDER_H
