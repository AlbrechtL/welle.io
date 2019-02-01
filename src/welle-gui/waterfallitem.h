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

#ifndef WATERFALLITEM_H
#define WATERFALLITEM_H

#include <vector>
#include <QQuickPaintedItem>
#include <QImage>
#include <QtCharts>

class WaterfallItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(bool isStarted READ isStarted NOTIFY isStartedChanged)
    Q_PROPERTY(float sensitivity READ sensitivity WRITE setSensitivity NOTIFY sensitivityChanged)
    Q_PROPERTY(float minValue READ minValue WRITE setMinValue NOTIFY minMinValueChanged)

    QImage _image;
    QList<QRgb> _colors;
    bool _samplesUpdated;
    float _sensitivity;
    float _minValue;
    QLineSeries dataSeries;
    QString messageToPlot;

public:
    explicit WaterfallItem(QQuickItem *parent = 0);
    void paint(QPainter *painter);
    bool isStarted() const;
    float sensitivity() const;
    void setSensitivity(float value);
    float minValue() const;
    void setMinValue(float value);
    QLineSeries* getDataSeries();

    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void plotMessage(QString message);

private slots:
    void samplesCollected();
    void sizeChanged();

signals:
    void isStartedChanged();
    void sensitivityChanged();
    void minMinValueChanged();

};

#endif // WATERFALLITEM_H
