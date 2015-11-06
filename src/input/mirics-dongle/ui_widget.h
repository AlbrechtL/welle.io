/********************************************************************************
** Form generated from reading UI file 'widget.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QLCDNumber>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QFrame *frame;
    QLCDNumber *rateDisplay;
    QLabel *label;
    QSpinBox *externalGain;
    QSpinBox *f_correction;
    QSpinBox *khzOffset;
    QSpinBox *hzOffset;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *statusLabel;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName(QString::fromUtf8("Form"));
        Form->resize(154, 223);
        frame = new QFrame(Form);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setGeometry(QRect(0, 0, 151, 221));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        rateDisplay = new QLCDNumber(frame);
        rateDisplay->setObjectName(QString::fromUtf8("rateDisplay"));
        rateDisplay->setGeometry(QRect(0, 120, 141, 21));
        rateDisplay->setNumDigits(7);
        rateDisplay->setSegmentStyle(QLCDNumber::Flat);
        label = new QLabel(frame);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(20, 150, 67, 21));
        externalGain = new QSpinBox(frame);
        externalGain->setObjectName(QString::fromUtf8("externalGain"));
        externalGain->setGeometry(QRect(0, 0, 91, 31));
        externalGain->setMaximum(103);
        externalGain->setValue(55);
        f_correction = new QSpinBox(frame);
        f_correction->setObjectName(QString::fromUtf8("f_correction"));
        f_correction->setGeometry(QRect(0, 30, 91, 31));
        f_correction->setMinimum(-100);
        f_correction->setMaximum(100);
        khzOffset = new QSpinBox(frame);
        khzOffset->setObjectName(QString::fromUtf8("khzOffset"));
        khzOffset->setGeometry(QRect(0, 60, 91, 31));
        khzOffset->setMaximum(1000000);
        hzOffset = new QSpinBox(frame);
        hzOffset->setObjectName(QString::fromUtf8("hzOffset"));
        hzOffset->setGeometry(QRect(0, 90, 91, 31));
        hzOffset->setMaximum(999);
        label_2 = new QLabel(frame);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(100, 90, 51, 21));
        label_3 = new QLabel(frame);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(100, 70, 41, 21));
        label_4 = new QLabel(frame);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(100, 40, 51, 21));
        label_5 = new QLabel(frame);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(100, 10, 41, 21));
        statusLabel = new QLabel(frame);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));
        statusLabel->setGeometry(QRect(16, 180, 121, 21));

        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QApplication::translate("Form", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Form", "mirics-sdr", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Form", "Hz", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("Form", "KHz", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("Form", "ppm", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("Form", "gain", 0, QApplication::UnicodeUTF8));
        statusLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WIDGET_H
