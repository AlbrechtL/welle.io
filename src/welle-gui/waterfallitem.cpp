/*
 *    Copyright (C) 2019
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on based on the frequency-analyzer application. (https://github.com/Venemo/frequency-analyzer)
 *    Copyright (c) 2014 Timur Kristóf
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *    SOFTWARE.
 *
 *    It is licensed to you under the terms of the MIT license.
 *    http://opensource.org/licenses/MIT
 */

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <cmath>
#include <cfloat>
#include <climits>
#include "waterfallitem.h"

WaterfallItem::WaterfallItem(QQuickItem *parent)
    : QQuickPaintedItem(parent) {

    // Initialize connections
    QObject::connect(&dataSeries, &QLineSeries::pointsReplaced, this, &WaterfallItem::samplesCollected);
    QObject::connect(this, &QQuickItem::widthChanged, this, &WaterfallItem::sizeChanged);
    QObject::connect(this, &QQuickItem::heightChanged, this, &WaterfallItem::sizeChanged);

    // Initialize properties
    this->setVisible(true);
    this->setFlag(QQuickItem::ItemHasContents);
    _samplesUpdated = false;
    _image = QImage((int)this->width(), (int)this->height(), QImage::Format_ARGB32);
    _image.fill(QColor(255, 255, 255));
    _sensitivity = 0.05;

    // Generate displayable colors
    QImage img(500, 1, QImage::Format_ARGB32);
    _colors.reserve(img.height());
    QPainter painter;
    painter.begin(&img);
    QLinearGradient gradient;
    gradient.setStart(0, 0);
    gradient.setFinalStop(img.width(), 0);
    gradient.setColorAt(0, QColor(255, 255, 255));
    gradient.setColorAt(0.2, QColor(0, 0, 255));
    gradient.setColorAt(0.4, QColor(255, 240, 0));
    gradient.setColorAt(0.6, QColor(255, 0, 0));
    gradient.setColorAt(1, QColor(0, 0, 0));
    gradient.setSpread(QGradient::PadSpread);
    painter.fillRect(QRect(0, 0, img.width(), 1), QBrush(gradient));
    painter.end();
    for (int i = 0; i < img.width(); i++) {
        QRgb rgb = img.pixel(i, 0);
        _colors.append(rgb);
    }

    // Repaint this item
    update();
}

void WaterfallItem::sizeChanged() {
    QImage img = QImage((int)this->width(), (int)this->height(), QImage::Format_ARGB32);
    img.fill(QColor(255, 255, 255));

    if (!_image.isNull()) {
        QPainter painter;
        painter.begin(&img);
        painter.drawImage(QRect(0, 0, width(), height()), _image, QRect(0, 0, _image.width(), _image.height()));
        painter.end();
    }

    _image = img;
    update();
}

void WaterfallItem::paint(QPainter *painter) {
    painter->drawImage(QRect(0, 0, width(), height()), _image, QRect(0, 0, _image.width(), _image.height()));
}

bool WaterfallItem::start() {
//    bool result = _sampler.start();
    bool result = true;
    emit this->isStartedChanged();
    return result;
}

void WaterfallItem::stop() {
//    _sampler.stop();
    emit this->isStartedChanged();
}

bool WaterfallItem::isStarted() const {
//    return _sampler.isStarted();
    return true;
}

void WaterfallItem::clear() {
    _image = QImage((int)this->width(), (int)this->height(), QImage::Format_ARGB32);
    _image.fill(QColor(255, 255, 255));
}

void WaterfallItem::plotMessage(QString message)
{
    messageToPlot = message;
}

float WaterfallItem::sensitivity() const {
    return _sensitivity;
}

void WaterfallItem::setSensitivity(float value) {
    _sensitivity = value;
    emit this->sensitivityChanged();
}

float WaterfallItem::minValue() const
{
    return _minValue;
}

void WaterfallItem::setMinValue(float value)
{
    _minValue = value;
    emit this->minMinValueChanged();
}

QLineSeries *WaterfallItem::getDataSeries()
{
    return &dataSeries;
}

void WaterfallItem::samplesCollected() {
    int _sampleNumber = dataSeries.count();

    // Create new image
    QImage img((int)this->width(), (int)this->height(), QImage::Format_ARGB32);
    QPainter painter;
    painter.begin(&img);

    // Draw 1st pixel row: new values
    for (int x = 0; x < img.width(); x++) {
        unsigned i1 = x * _sampleNumber / img.width();
        float amplitude = dataSeries.at(i1).y() - _minValue;
        int value = (int)(amplitude * (float)_sensitivity * (float)_colors.length());
        if (value < 0)
            value = 0;
        if (value >= _colors.length())
            value = _colors.length() - 1;

        painter.setPen(_colors[value]);
        painter.drawRect(x, 0, 1, 5);
    }

    // Draw old values
    if (!_image.isNull()) {
        painter.drawImage(QRect(0, 6, width(), height()), _image, QRect(0, 0, _image.width(), _image.height()));
    }

    // Draw message into the plot
    if(!messageToPlot.isEmpty()) {
        // Draw everthing in black
        painter.setPen(QColor("black"));

        // Draw horizontal line
        painter.drawLine(0,14, img.width(), 14);

        // Put text above the line
        painter.setFont(QFont("Arial", 12));
        painter.drawText(2,12, messageToPlot);

        // Reset message
        messageToPlot.clear();
    }

    painter.end();

    // Redraw the item
    _image = img;
    update();
}
