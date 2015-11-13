/********************************************************************************
** Form generated from reading UI file 'airspy-widget.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AIRSPY_2D_WIDGET_H
#define UI_AIRSPY_2D_WIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QLCDNumber>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_airspyWidget
{
public:
    QFrame *contents;
    QLabel *label;
    QSlider *lnaSlider;
    QLabel *label_2;
    QSlider *mixerSlider;
    QLabel *label_3;
    QPushButton *lnaButton;
    QSlider *vgaSlider;
    QLabel *label_4;
    QPushButton *mixerButton;
    QLabel *displaySerial;
    QPushButton *biasButton;
    QLCDNumber *lnaDisplay;
    QLCDNumber *mixerDisplay;
    QLCDNumber *vgaDisplay;

    void setupUi(QWidget *airspyWidget)
    {
        if (airspyWidget->objectName().isEmpty())
            airspyWidget->setObjectName(QString::fromUtf8("airspyWidget"));
        airspyWidget->resize(423, 212);
        contents = new QFrame(airspyWidget);
        contents->setObjectName(QString::fromUtf8("contents"));
        contents->setGeometry(QRect(10, 0, 411, 201));
        contents->setFrameShape(QFrame::StyledPanel);
        contents->setFrameShadow(QFrame::Raised);
        label = new QLabel(contents);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(10, 170, 101, 21));
        lnaSlider = new QSlider(contents);
        lnaSlider->setObjectName(QString::fromUtf8("lnaSlider"));
        lnaSlider->setGeometry(QRect(60, 10, 281, 19));
        lnaSlider->setMaximum(15);
        lnaSlider->setValue(10);
        lnaSlider->setOrientation(Qt::Horizontal);
        label_2 = new QLabel(contents);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(10, 10, 65, 21));
        mixerSlider = new QSlider(contents);
        mixerSlider->setObjectName(QString::fromUtf8("mixerSlider"));
        mixerSlider->setGeometry(QRect(50, 40, 281, 20));
        mixerSlider->setMaximum(15);
        mixerSlider->setValue(10);
        mixerSlider->setOrientation(Qt::Horizontal);
        label_3 = new QLabel(contents);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(10, 40, 65, 21));
        lnaButton = new QPushButton(contents);
        lnaButton->setObjectName(QString::fromUtf8("lnaButton"));
        lnaButton->setGeometry(QRect(270, 130, 121, 31));
        vgaSlider = new QSlider(contents);
        vgaSlider->setObjectName(QString::fromUtf8("vgaSlider"));
        vgaSlider->setGeometry(QRect(60, 70, 281, 19));
        vgaSlider->setMaximum(15);
        vgaSlider->setValue(10);
        vgaSlider->setOrientation(Qt::Horizontal);
        label_4 = new QLabel(contents);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(10, 70, 65, 21));
        mixerButton = new QPushButton(contents);
        mixerButton->setObjectName(QString::fromUtf8("mixerButton"));
        mixerButton->setGeometry(QRect(270, 100, 121, 31));
        displaySerial = new QLabel(contents);
        displaySerial->setObjectName(QString::fromUtf8("displaySerial"));
        displaySerial->setGeometry(QRect(20, 150, 241, 21));
        biasButton = new QPushButton(contents);
        biasButton->setObjectName(QString::fromUtf8("biasButton"));
        biasButton->setGeometry(QRect(270, 160, 121, 31));
        lnaDisplay = new QLCDNumber(contents);
        lnaDisplay->setObjectName(QString::fromUtf8("lnaDisplay"));
        lnaDisplay->setGeometry(QRect(340, 10, 64, 23));
        lnaDisplay->setFrameShape(QFrame::NoFrame);
        lnaDisplay->setDigitCount(2);
        lnaDisplay->setSegmentStyle(QLCDNumber::Flat);
        mixerDisplay = new QLCDNumber(contents);
        mixerDisplay->setObjectName(QString::fromUtf8("mixerDisplay"));
        mixerDisplay->setGeometry(QRect(340, 40, 64, 23));
        mixerDisplay->setFrameShape(QFrame::NoFrame);
        mixerDisplay->setDigitCount(2);
        mixerDisplay->setSegmentStyle(QLCDNumber::Flat);
        vgaDisplay = new QLCDNumber(contents);
        vgaDisplay->setObjectName(QString::fromUtf8("vgaDisplay"));
        vgaDisplay->setGeometry(QRect(340, 70, 64, 23));
        vgaDisplay->setFrameShape(QFrame::NoFrame);
        vgaDisplay->setDigitCount(2);
        vgaDisplay->setSegmentStyle(QLCDNumber::Flat);

        retranslateUi(airspyWidget);

        QMetaObject::connectSlotsByName(airspyWidget);
    } // setupUi

    void retranslateUi(QWidget *airspyWidget)
    {
        airspyWidget->setWindowTitle(QApplication::translate("airspyWidget", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("airspyWidget", "airspy", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("airspyWidget", "lna", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("airspyWidget", "mixer", 0, QApplication::UnicodeUTF8));
        lnaButton->setText(QApplication::translate("airspyWidget", "lna-agc ", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("airspyWidget", "vga", 0, QApplication::UnicodeUTF8));
        mixerButton->setText(QApplication::translate("airspyWidget", "mixer-agc", 0, QApplication::UnicodeUTF8));
        displaySerial->setText(QString());
        biasButton->setText(QApplication::translate("airspyWidget", "bias", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class airspyWidget: public Ui_airspyWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AIRSPY_2D_WIDGET_H
