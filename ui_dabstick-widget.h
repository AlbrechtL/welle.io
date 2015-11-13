/********************************************************************************
** Form generated from reading UI file 'dabstick-widget.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DABSTICK_2D_WIDGET_H
#define UI_DABSTICK_2D_WIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_dabstickWidget
{
public:
    QFrame *contents;
    QLabel *label;
    QSpinBox *externalGain;
    QSpinBox *f_correction;
    QSpinBox *KhzOffset;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;

    void setupUi(QWidget *dabstickWidget)
    {
        if (dabstickWidget->objectName().isEmpty())
            dabstickWidget->setObjectName(QString::fromUtf8("dabstickWidget"));
        dabstickWidget->resize(159, 226);
        contents = new QFrame(dabstickWidget);
        contents->setObjectName(QString::fromUtf8("contents"));
        contents->setGeometry(QRect(0, 0, 151, 191));
        contents->setFrameShape(QFrame::StyledPanel);
        contents->setFrameShadow(QFrame::Raised);
        label = new QLabel(contents);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(10, 150, 101, 21));
        externalGain = new QSpinBox(contents);
        externalGain->setObjectName(QString::fromUtf8("externalGain"));
        externalGain->setGeometry(QRect(0, 0, 91, 21));
        externalGain->setMaximum(103);
        externalGain->setValue(10);
        f_correction = new QSpinBox(contents);
        f_correction->setObjectName(QString::fromUtf8("f_correction"));
        f_correction->setGeometry(QRect(0, 20, 91, 21));
        f_correction->setMinimum(-100);
        f_correction->setMaximum(100);
        KhzOffset = new QSpinBox(contents);
        KhzOffset->setObjectName(QString::fromUtf8("KhzOffset"));
        KhzOffset->setGeometry(QRect(0, 40, 91, 21));
        KhzOffset->setMinimum(-100);
        KhzOffset->setMaximum(100);
        label_3 = new QLabel(contents);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(100, 40, 41, 21));
        label_4 = new QLabel(contents);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(100, 20, 51, 21));
        label_5 = new QLabel(contents);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(100, 0, 41, 21));

        retranslateUi(dabstickWidget);

        QMetaObject::connectSlotsByName(dabstickWidget);
    } // setupUi

    void retranslateUi(QWidget *dabstickWidget)
    {
        dabstickWidget->setWindowTitle(QApplication::translate("dabstickWidget", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("dabstickWidget", "dabstick", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("dabstickWidget", "KHz", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("dabstickWidget", "ppm", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("dabstickWidget", "gain", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class dabstickWidget: public Ui_dabstickWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DABSTICK_2D_WIDGET_H
