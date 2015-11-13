/********************************************************************************
** Form generated from reading UI file 'sdrgui.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SDRGUI_H
#define UI_SDRGUI_H

#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QLCDNumber>
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_dabframe
{
public:
    QPushButton *startButton;
    QLabel *timeDisplay;
    QPushButton *QuitButton;
    QLCDNumber *coarseCorrectorDisplay;
    QLCDNumber *fineCorrectorDisplay;
    QComboBox *deviceSelector;
    QListView *ensembleDisplay;
    QLabel *ensembleName;
    QLCDNumber *ensembleId;
    QPushButton *dumpButton;
    QLCDNumber *errorDisplay;
    QLCDNumber *ficRatioDisplay;
    QLCDNumber *snrDisplay;
    QLabel *label_8;
    QLabel *label_9;
    QLabel *label_10;
    QComboBox *streamOutSelector;
    QPushButton *correctorReset;
    QLabel *versionName;
    QPushButton *audioDump;
    QComboBox *bandSelector;
    QComboBox *channelSelector;
    QComboBox *modeSelector;
    QLabel *syncedLabel;
    QLabel *dynamicLabel;

    void setupUi(QDialog *dabframe)
    {
        if (dabframe->objectName().isEmpty())
            dabframe->setObjectName(QString::fromUtf8("dabframe"));
        dabframe->resize(447, 301);
        QPalette palette;
        QBrush brush(QColor(255, 255, 255, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Base, brush);
        palette.setBrush(QPalette::Active, QPalette::Window, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Window, brush);
        palette.setBrush(QPalette::Disabled, QPalette::Base, brush);
        palette.setBrush(QPalette::Disabled, QPalette::Window, brush);
        dabframe->setPalette(palette);
        QFont font;
        font.setBold(false);
        font.setWeight(50);
        dabframe->setFont(font);
        startButton = new QPushButton(dabframe);
        startButton->setObjectName(QString::fromUtf8("startButton"));
        startButton->setGeometry(QRect(370, 80, 71, 61));
        QPalette palette1;
        palette1.setBrush(QPalette::Active, QPalette::Base, brush);
        QBrush brush1(QColor(255, 0, 0, 255));
        brush1.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Window, brush1);
        palette1.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette1.setBrush(QPalette::Inactive, QPalette::Window, brush1);
        palette1.setBrush(QPalette::Disabled, QPalette::Base, brush1);
        palette1.setBrush(QPalette::Disabled, QPalette::Window, brush1);
        startButton->setPalette(palette1);
        QFont font1;
        font1.setBold(true);
        font1.setWeight(75);
        startButton->setFont(font1);
        startButton->setAutoFillBackground(true);
        timeDisplay = new QLabel(dabframe);
        timeDisplay->setObjectName(QString::fromUtf8("timeDisplay"));
        timeDisplay->setGeometry(QRect(90, 51, 161, 20));
        timeDisplay->setFrameShape(QFrame::NoFrame);
        timeDisplay->setFrameShadow(QFrame::Raised);
        timeDisplay->setLineWidth(2);
        QuitButton = new QPushButton(dabframe);
        QuitButton->setObjectName(QString::fromUtf8("QuitButton"));
        QuitButton->setGeometry(QRect(370, 140, 71, 61));
        QPalette palette2;
        palette2.setBrush(QPalette::Active, QPalette::Button, brush1);
        palette2.setBrush(QPalette::Inactive, QPalette::Button, brush1);
        palette2.setBrush(QPalette::Disabled, QPalette::Button, brush1);
        QuitButton->setPalette(palette2);
        QuitButton->setFont(font1);
        QuitButton->setAutoFillBackground(true);
        coarseCorrectorDisplay = new QLCDNumber(dabframe);
        coarseCorrectorDisplay->setObjectName(QString::fromUtf8("coarseCorrectorDisplay"));
        coarseCorrectorDisplay->setGeometry(QRect(130, 70, 41, 31));
        QPalette palette3;
        palette3.setBrush(QPalette::Active, QPalette::Base, brush);
        QBrush brush2(QColor(255, 255, 0, 255));
        brush2.setStyle(Qt::SolidPattern);
        palette3.setBrush(QPalette::Active, QPalette::Window, brush2);
        palette3.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette3.setBrush(QPalette::Inactive, QPalette::Window, brush2);
        palette3.setBrush(QPalette::Disabled, QPalette::Base, brush2);
        palette3.setBrush(QPalette::Disabled, QPalette::Window, brush2);
        coarseCorrectorDisplay->setPalette(palette3);
        coarseCorrectorDisplay->setAutoFillBackground(false);
        coarseCorrectorDisplay->setFrameShape(QFrame::NoFrame);
        coarseCorrectorDisplay->setLineWidth(1);
        coarseCorrectorDisplay->setDigitCount(3);
        coarseCorrectorDisplay->setSegmentStyle(QLCDNumber::Flat);
        fineCorrectorDisplay = new QLCDNumber(dabframe);
        fineCorrectorDisplay->setObjectName(QString::fromUtf8("fineCorrectorDisplay"));
        fineCorrectorDisplay->setGeometry(QRect(30, 70, 71, 31));
        QPalette palette4;
        palette4.setBrush(QPalette::Active, QPalette::Base, brush);
        palette4.setBrush(QPalette::Active, QPalette::Window, brush2);
        palette4.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette4.setBrush(QPalette::Inactive, QPalette::Window, brush2);
        palette4.setBrush(QPalette::Disabled, QPalette::Base, brush2);
        palette4.setBrush(QPalette::Disabled, QPalette::Window, brush2);
        fineCorrectorDisplay->setPalette(palette4);
        fineCorrectorDisplay->setAutoFillBackground(false);
        fineCorrectorDisplay->setFrameShape(QFrame::NoFrame);
        fineCorrectorDisplay->setLineWidth(1);
        fineCorrectorDisplay->setDigitCount(4);
        fineCorrectorDisplay->setSegmentStyle(QLCDNumber::Flat);
        deviceSelector = new QComboBox(dabframe);
        deviceSelector->setObjectName(QString::fromUtf8("deviceSelector"));
        deviceSelector->setGeometry(QRect(20, 200, 101, 31));
        ensembleDisplay = new QListView(dabframe);
        ensembleDisplay->setObjectName(QString::fromUtf8("ensembleDisplay"));
        ensembleDisplay->setGeometry(QRect(190, 80, 171, 151));
        ensembleDisplay->setFrameShape(QFrame::Box);
        ensembleDisplay->setFrameShadow(QFrame::Raised);
        ensembleDisplay->setLineWidth(2);
        ensembleName = new QLabel(dabframe);
        ensembleName->setObjectName(QString::fromUtf8("ensembleName"));
        ensembleName->setGeometry(QRect(20, 10, 101, 21));
        QFont font2;
        font2.setPointSize(11);
        ensembleName->setFont(font2);
        ensembleName->setFrameShape(QFrame::NoFrame);
        ensembleId = new QLCDNumber(dabframe);
        ensembleId->setObjectName(QString::fromUtf8("ensembleId"));
        ensembleId->setGeometry(QRect(100, 10, 61, 21));
        ensembleId->setFrameShape(QFrame::NoFrame);
        ensembleId->setMode(QLCDNumber::Hex);
        ensembleId->setSegmentStyle(QLCDNumber::Flat);
        dumpButton = new QPushButton(dabframe);
        dumpButton->setObjectName(QString::fromUtf8("dumpButton"));
        dumpButton->setGeometry(QRect(100, 120, 81, 31));
        errorDisplay = new QLCDNumber(dabframe);
        errorDisplay->setObjectName(QString::fromUtf8("errorDisplay"));
        errorDisplay->setGeometry(QRect(290, 10, 51, 21));
        errorDisplay->setFrameShape(QFrame::NoFrame);
        errorDisplay->setDigitCount(3);
        errorDisplay->setSegmentStyle(QLCDNumber::Flat);
        ficRatioDisplay = new QLCDNumber(dabframe);
        ficRatioDisplay->setObjectName(QString::fromUtf8("ficRatioDisplay"));
        ficRatioDisplay->setGeometry(QRect(233, 11, 51, 20));
        ficRatioDisplay->setFrameShape(QFrame::NoFrame);
        ficRatioDisplay->setDigitCount(3);
        ficRatioDisplay->setSegmentStyle(QLCDNumber::Flat);
        snrDisplay = new QLCDNumber(dabframe);
        snrDisplay->setObjectName(QString::fromUtf8("snrDisplay"));
        snrDisplay->setGeometry(QRect(160, 10, 64, 21));
        snrDisplay->setFrameShape(QFrame::NoFrame);
        snrDisplay->setDigitCount(3);
        snrDisplay->setSegmentStyle(QLCDNumber::Flat);
        label_8 = new QLabel(dabframe);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(180, 30, 31, 21));
        label_9 = new QLabel(dabframe);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setGeometry(QRect(250, 30, 51, 21));
        label_10 = new QLabel(dabframe);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setGeometry(QRect(310, 30, 101, 21));
        streamOutSelector = new QComboBox(dabframe);
        streamOutSelector->setObjectName(QString::fromUtf8("streamOutSelector"));
        streamOutSelector->setGeometry(QRect(20, 150, 161, 21));
        correctorReset = new QPushButton(dabframe);
        correctorReset->setObjectName(QString::fromUtf8("correctorReset"));
        correctorReset->setGeometry(QRect(370, 200, 71, 41));
        QPalette palette5;
        QBrush brush3(QColor(85, 170, 255, 255));
        brush3.setStyle(Qt::SolidPattern);
        palette5.setBrush(QPalette::Active, QPalette::Button, brush3);
        palette5.setBrush(QPalette::Active, QPalette::Base, brush);
        QBrush brush4(QColor(0, 0, 0, 255));
        brush4.setStyle(Qt::SolidPattern);
        palette5.setBrush(QPalette::Active, QPalette::Window, brush4);
        palette5.setBrush(QPalette::Inactive, QPalette::Button, brush3);
        palette5.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette5.setBrush(QPalette::Inactive, QPalette::Window, brush4);
        palette5.setBrush(QPalette::Disabled, QPalette::Button, brush3);
        palette5.setBrush(QPalette::Disabled, QPalette::Base, brush4);
        palette5.setBrush(QPalette::Disabled, QPalette::Window, brush4);
        correctorReset->setPalette(palette5);
        correctorReset->setAutoFillBackground(true);
        versionName = new QLabel(dabframe);
        versionName->setObjectName(QString::fromUtf8("versionName"));
        versionName->setGeometry(QRect(270, 50, 171, 21));
        QFont font3;
        font3.setPointSize(12);
        font3.setBold(true);
        font3.setWeight(75);
        versionName->setFont(font3);
        versionName->setFrameShape(QFrame::NoFrame);
        audioDump = new QPushButton(dabframe);
        audioDump->setObjectName(QString::fromUtf8("audioDump"));
        audioDump->setGeometry(QRect(20, 120, 81, 31));
        QFont font4;
        font4.setPointSize(9);
        audioDump->setFont(font4);
        bandSelector = new QComboBox(dabframe);
        bandSelector->setObjectName(QString::fromUtf8("bandSelector"));
        bandSelector->setGeometry(QRect(20, 170, 91, 31));
        channelSelector = new QComboBox(dabframe);
        channelSelector->setObjectName(QString::fromUtf8("channelSelector"));
        channelSelector->setGeometry(QRect(120, 200, 61, 31));
        modeSelector = new QComboBox(dabframe);
        modeSelector->setObjectName(QString::fromUtf8("modeSelector"));
        modeSelector->setGeometry(QRect(110, 170, 71, 31));
        syncedLabel = new QLabel(dabframe);
        syncedLabel->setObjectName(QString::fromUtf8("syncedLabel"));
        syncedLabel->setGeometry(QRect(370, 10, 65, 21));
        QPalette palette6;
        palette6.setBrush(QPalette::Active, QPalette::Base, brush);
        palette6.setBrush(QPalette::Active, QPalette::Window, brush1);
        palette6.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette6.setBrush(QPalette::Inactive, QPalette::Window, brush1);
        palette6.setBrush(QPalette::Disabled, QPalette::Base, brush1);
        palette6.setBrush(QPalette::Disabled, QPalette::Window, brush1);
        syncedLabel->setPalette(palette6);
        syncedLabel->setAutoFillBackground(true);
        syncedLabel->setFrameShape(QFrame::Box);
        dynamicLabel = new QLabel(dabframe);
        dynamicLabel->setObjectName(QString::fromUtf8("dynamicLabel"));
        dynamicLabel->setGeometry(QRect(10, 250, 411, 31));

        retranslateUi(dabframe);

        QMetaObject::connectSlotsByName(dabframe);
    } // setupUi

    void retranslateUi(QDialog *dabframe)
    {
        dabframe->setWindowTitle(QApplication::translate("dabframe", "sdr-j DAB/DAB+ receiver ", 0, QApplication::UnicodeUTF8));
        dabframe->setWindowIconText(QApplication::translate("dabframe", "QUIT", 0, QApplication::UnicodeUTF8));
        startButton->setText(QApplication::translate("dabframe", "START", 0, QApplication::UnicodeUTF8));
        timeDisplay->setText(QApplication::translate("dabframe", "TextLabel", 0, QApplication::UnicodeUTF8));
        QuitButton->setText(QApplication::translate("dabframe", "QUIT", 0, QApplication::UnicodeUTF8));
        deviceSelector->clear();
        deviceSelector->insertItems(0, QStringList()
         << QApplication::translate("dabframe", "no device", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("dabframe", "file input (.raw)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("dabframe", "file input (.sdr)", 0, QApplication::UnicodeUTF8)
        );
        ensembleName->setText(QString());
        dumpButton->setText(QApplication::translate("dabframe", "dump", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("dabframe", "SNR", 0, QApplication::UnicodeUTF8));
        label_9->setText(QApplication::translate("dabframe", "fic ratio", 0, QApplication::UnicodeUTF8));
        label_10->setText(QApplication::translate("dabframe", "aac/mp2 ratio", 0, QApplication::UnicodeUTF8));
        streamOutSelector->clear();
        streamOutSelector->insertItems(0, QStringList()
         << QApplication::translate("dabframe", "select output", 0, QApplication::UnicodeUTF8)
        );
        correctorReset->setText(QApplication::translate("dabframe", "reset", 0, QApplication::UnicodeUTF8));
        versionName->setText(QString());
        audioDump->setText(QApplication::translate("dabframe", "audioDump", 0, QApplication::UnicodeUTF8));
        bandSelector->clear();
        bandSelector->insertItems(0, QStringList()
         << QApplication::translate("dabframe", "BAND III", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("dabframe", "L BAND", 0, QApplication::UnicodeUTF8)
        );
        modeSelector->clear();
        modeSelector->insertItems(0, QStringList()
         << QApplication::translate("dabframe", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("dabframe", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("dabframe", "4", 0, QApplication::UnicodeUTF8)
        );
        syncedLabel->setText(QString());
        dynamicLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class dabframe: public Ui_dabframe {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SDRGUI_H
