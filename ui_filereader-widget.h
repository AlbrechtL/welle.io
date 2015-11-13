/********************************************************************************
** Form generated from reading UI file 'filereader-widget.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FILEREADER_2D_WIDGET_H
#define UI_FILEREADER_2D_WIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_filereaderWidget
{
public:
    QFrame *frame;
    QLabel *label;
    QLabel *nameofFile;

    void setupUi(QWidget *filereaderWidget)
    {
        if (filereaderWidget->objectName().isEmpty())
            filereaderWidget->setObjectName(QString::fromUtf8("filereaderWidget"));
        filereaderWidget->resize(411, 124);
        frame = new QFrame(filereaderWidget);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setGeometry(QRect(10, 20, 401, 91));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        label = new QLabel(frame);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(30, 20, 91, 21));
        nameofFile = new QLabel(frame);
        nameofFile->setObjectName(QString::fromUtf8("nameofFile"));
        nameofFile->setGeometry(QRect(10, 50, 361, 21));

        retranslateUi(filereaderWidget);

        QMetaObject::connectSlotsByName(filereaderWidget);
    } // setupUi

    void retranslateUi(QWidget *filereaderWidget)
    {
        filereaderWidget->setWindowTitle(QApplication::translate("filereaderWidget", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("filereaderWidget", "fileReader", 0, QApplication::UnicodeUTF8));
        nameofFile->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class filereaderWidget: public Ui_filereaderWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FILEREADER_2D_WIDGET_H
