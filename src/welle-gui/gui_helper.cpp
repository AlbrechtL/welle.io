/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <QDebug>
#include <QSettings>

#include "gui_helper.h"
#include "debug_output.h"
#include "input_factory.h"
#include "audio_output.h"
#include "dab-constants.h"
#include "msc-handler.h"
#include "version.h"
#include "waterfallitem.h"

/**
  *	We use the creation function merely to set up the
  *	user interface and make the connections between the
  *	gui elements and the handling agents. All real action
  *	is embedded in actions, initiated by gui buttons
  */
CGUIHelper::CGUIHelper(CRadioController *RadioController, QObject *parent)
    : QObject(parent)
    , radioController(RadioController)
    , spectrumSeries(nullptr)
    , impulseResponseSeries(nullptr)
{
    // Add image provider for the MOT slide show
    motImage = new CMOTImageProvider;

    connect(RadioController, &CRadioController::motChanged, this, &CGUIHelper::motUpdate);
    connect(RadioController, &CRadioController::showErrorMessage, this, &CGUIHelper::showErrorMessage);
    connect(RadioController, &CRadioController::showInfoMessage, this, &CGUIHelper::showInfoMessage);

#ifndef QT_NO_SYSTEMTRAYICON
    minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SIGNAL(minimizeWindow()));

    maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(maximizeAction, SIGNAL(triggered()), this, SIGNAL(maximizeWindow()));

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SIGNAL(restoreWindow()));

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    trayIconMenu = new QMenu();
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon();

    trayIcon->setContextMenu(trayIconMenu);

    trayIcon->setIcon(QIcon(":/icon/icon.png"));
    trayIcon->show();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(showWindow(QSystemTrayIcon::ActivationReason)));

#endif

    CDebugOutput::setCGUI(this);
}

CGUIHelper::~CGUIHelper()
{
    // Avoid segmentation fault if a debug message should be displayed after deleting
    CDebugOutput::setCGUI(nullptr);

    qDebug() << "GUI:" <<  "deleting radioInterface";
}

void CGUIHelper::close()
{
    qDebug() << "GUI:" <<  "close application";
    QApplication::quit();
}

void CGUIHelper::deviceClosed()
{
}

/**
 * \brief returns the licenses for all the relative libraries plus application version information
 */
const QVariantMap CGUIHelper::licenses()
{
    QVariantMap ret;
    QFile *File;
    QByteArray InfoContent;

    // Set application version
    InfoContent.append("welle.io " + tr("version") + ": " + QString(CURRENT_VERSION) + "\n");
    InfoContent.append(tr("Git revision") + ": " + QString(GITHASH) + "\n");
    InfoContent.append(tr("Build on") + ": " + QString(__TIMESTAMP__) + "\n");
    InfoContent.append(tr("QT version") + ": " + qVersion() + "\n");
    InfoContent.append("\n");

    InfoContent.append("For legal information scroll down, please.\n");
    InfoContent.append("\n");

    // Read AUTHORS
    InfoContent.append("AUTHORS\n");
    InfoContent.append("-------\n");
    File = new QFile(":/AUTHORS");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Read THANKS
    InfoContent.append("THANKS\n");
    InfoContent.append("------\n");
    File = new QFile(":/THANKS");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Read COPYING
    InfoContent.append("COPYING (GPLv2)\n");
    InfoContent.append("-------\n");
    File = new QFile(":/COPYING");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Read COPYING
    InfoContent.append("COPYING (LGPL-2.1)\n");
    InfoContent.append("-------\n");
    File = new QFile(":/libs/COPYING.LGPL-2.1");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Read COPYING
    InfoContent.append("kiss_fft COPYING (BSD 3-clause)\n");
    InfoContent.append("-------\n");
    File = new QFile(":/libs/kiss_fft/COPYING");
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;

    // Set graph license content
    ret.insert("FileContent", InfoContent);

    return ret;
}

void CGUIHelper::motUpdate(QImage MOTImage)
{
    if (MOTImage.isNull()) {
        MOTImage = QImage(320, 240, QImage::Format_Alpha8);
        MOTImage.fill(Qt::transparent);
    }
    this->motImage->setPixmap(QPixmap::fromImage(MOTImage));
    emit motChanged();
}

void CGUIHelper::showErrorMessage(QString Text)
{
#ifndef QT_NO_SYSTEMTRAYICON
    trayIcon->showMessage(QCoreApplication::applicationName(), Text, QIcon(":/icon.png"), 5000);
#else
    (void)Text;
#endif
}

void CGUIHelper::showInfoMessage(QString Text)
{
#ifndef QT_NO_SYSTEMTRAYICON
    trayIcon->showMessage(QCoreApplication::applicationName(), Text, QIcon(":/icon.png"), 5000);
#else
    (void)Text;
#endif
}

void CGUIHelper::showWindow(QSystemTrayIcon::ActivationReason r)
{
#ifndef QT_NO_SYSTEMTRAYICON
    if (r == QSystemTrayIcon::Trigger)
        emit restoreWindow();
#endif
}

void CGUIHelper::registerSpectrumSeries(QAbstractSeries* series)
{
    spectrumSeries = static_cast<QXYSeries*>(series);
}

void CGUIHelper::registerSpectrumWaterfall(QObject *obj)
{
    WaterfallItem *item = qobject_cast<WaterfallItem*>(obj);
    spectrumSeries = static_cast<QXYSeries*>(item->getDataSeries());
}

void CGUIHelper::registerImpulseResonseSeries(QAbstractSeries* series)
{
    impulseResponseSeries = static_cast<QXYSeries*>(series);
}

void CGUIHelper::registerImpulseResonseWaterfall(QObject *obj)
{
    WaterfallItem *item = qobject_cast<WaterfallItem*>(obj);
    impulseResponseSeries = static_cast<QXYSeries*>(item->getDataSeries());
}

void CGUIHelper::registerNullSymbolSeries(QAbstractSeries *series)
{
    nullSymbolSeries = static_cast<QXYSeries*>(series);
}

void CGUIHelper::registerNullSymbolWaterfall(QObject *obj)
{
    WaterfallItem *item = qobject_cast<WaterfallItem*>(obj);
    nullSymbolSeries = static_cast<QXYSeries*>(item->getDataSeries());
}

void CGUIHelper::registerConstellationSeries(QAbstractSeries *series)
{
    constellationSeries = static_cast<QXYSeries*>(series);
}

void CGUIHelper::tryHideWindow()
{
#ifndef QT_NO_SYSTEMTRAYICON
    // Hide only if system tray is available otherwise ignore it. Standard Gnome doesn't have a system tray so user would lost the control.
    if(trayIcon->isSystemTrayAvailable()) {
        trayIcon->showMessage(QCoreApplication::applicationName(), tr("The program will keep running in the "
                                           "system tray. To terminate the program, "
                                           "choose <b>Quit</b> in the context menu "
                                           "of the system tray entry."), QIcon(":/icon.png"), 5000);
        emit minimizeWindow();
    }
#endif
}

// This function is called by the QML GUI
void CGUIHelper::updateSpectrum()
{
    std::vector<DSPCOMPLEX> signalProbeBuffer;
    int T_u = radioController->getParams().T_u;

    qreal y = 0;
    qreal x = 0;
    qreal y_max = 0;
    qreal x_min = 0;
    qreal x_max = 0;

    qreal tunedFrequency_MHz = 0;
    qreal CurrentFrequency = radioController->getCurrentFrequency();
    qreal sampleFrequency_MHz = INPUT_RATE / 1e6;
    qreal dip_MHz = sampleFrequency_MHz / T_u;

    signalProbeBuffer = radioController->getSignalProbe();

    if (signalProbeBuffer.size() == (size_t)T_u) {
        spectrumSeriesData.resize(T_u);

        fft::Forward FFT(T_u);
        DSPCOMPLEX* spectrumBuffer = FFT.getVector();

        std::copy(signalProbeBuffer.begin(), signalProbeBuffer.begin() + T_u,
                spectrumBuffer);

        // Do FFT to get the spectrum
        FFT.do_FFT();

        tunedFrequency_MHz = CurrentFrequency / 1e6;

        // Process samples one by one
        for (int i = 0; i < T_u; i++) {
            int half_Tu = T_u / 2;

            // Shift FFT samples
            if (i < half_Tu)
                y = abs(spectrumBuffer[i + half_Tu]);
            else
                y = abs(spectrumBuffer[i - half_Tu]);

            // Apply a cumulative moving average filter
            int avg = 4; // Number of y values to average
            qreal CMA = spectrumSeriesData[i].y();
            y = (CMA * avg + y) / (avg + 1);

            // Find maximum value to scale the plotter
            if (y > y_max)
                y_max = y;

            // Calc x frequency
            x = (i * dip_MHz) + (tunedFrequency_MHz - (sampleFrequency_MHz / 2));

            spectrumSeriesData[i]= QPointF(x, y);
        }

        x_min = tunedFrequency_MHz - (sampleFrequency_MHz / 2);
        x_max = tunedFrequency_MHz + (sampleFrequency_MHz / 2);

        emit setSpectrumAxis(y_max, x_min, x_max);

        if(spectrumSeries)
            spectrumSeries->replace(spectrumSeriesData);
    }
}

void CGUIHelper::updateImpulseResponse()
{
    std::vector<float> impulseResponseBuffer;
    int T_u = radioController->getParams().T_u;

    qreal y_max = 0;
    qreal x_min = 0;
    qreal x_max = 0;

    impulseResponseBuffer = radioController->getImpulseResponse();

    if (impulseResponseBuffer.size() == (size_t)T_u) {
        impulseResponseSeriesData.resize(T_u);
        for (int i = 0; i < T_u; i++) {
            qreal y = 10.0f * std::log10(impulseResponseBuffer[i]);
            qreal x = i;

            // Find maximum value to scale the plotter
            if (y > y_max)
                y_max = y;
            impulseResponseSeriesData[i] = QPointF(x, y);
        }

        x_min = 0;
        x_max = T_u;

        emit setImpulseResponseAxis(y_max, x_min, x_max);

        if(impulseResponseSeries)
            impulseResponseSeries->replace(impulseResponseSeriesData);
    }
}

void CGUIHelper::updateNullSymbol()
{
    std::vector<DSPCOMPLEX> nullSymbolBuffer;
    int T_u = radioController->getParams().T_u;
    int T_null = radioController->getParams().T_null;

    qreal y = 0;
    qreal x = 0;
    qreal y_max = 0;
    qreal x_min = 0;
    qreal x_max = 0;

    qreal tunedFrequency_MHz = 0;
    qreal CurrentFrequency = radioController->getCurrentFrequency();
    qreal sampleFrequency_MHz = INPUT_RATE / 1e6;
    qreal dip_MHz = sampleFrequency_MHz / T_u;

    nullSymbolBuffer = radioController->getNullSymbol();

    if (nullSymbolBuffer.size() == (size_t)T_null) {
        nullSymbolSeriesData.resize(T_u);

        fft::Forward FFT(T_u);
        DSPCOMPLEX* spectrumBuffer = FFT.getVector();

        std::copy(nullSymbolBuffer.begin(), nullSymbolBuffer.begin() + T_u,
                spectrumBuffer);

        // Do FFT to get the spectrum
        FFT.do_FFT();

        tunedFrequency_MHz = CurrentFrequency / 1e6;

        // Process samples one by one
        for (int i = 0; i < T_u; i++) {
            int half_Tu = T_u / 2;

            // Shift FFT samples
            if (i < half_Tu)
                y = abs(spectrumBuffer[i + half_Tu]);
            else
                y = abs(spectrumBuffer[i - half_Tu]);

            // Apply a cumulative moving average filter
            int avg = 4; // Number of y values to average
            qreal CMA = nullSymbolSeriesData[i].y();
            y = (CMA * avg + y) / (avg + 1);

            // Find maximum value to scale the plotter
            if (y > y_max)
                y_max = y;

            // Calc x frequency
            x = (i * dip_MHz) + (tunedFrequency_MHz - (sampleFrequency_MHz / 2));

            nullSymbolSeriesData[i]= QPointF(x, y);
        }

        x_min = tunedFrequency_MHz - (sampleFrequency_MHz / 2);
        x_max = tunedFrequency_MHz + (sampleFrequency_MHz / 2);

        emit setNullSymbolAxis(y_max, x_min, x_max);

        if(nullSymbolSeries)
            nullSymbolSeries->replace(nullSymbolSeriesData);
    }
}

void CGUIHelper::updateConstellation()
{
    std::vector<DSPCOMPLEX> constellationPointBuffer;

    qreal x_min = 0;
    qreal x_max = 0;

    constellationPointBuffer = radioController->getConstellationPoint();

    const size_t decim = OfdmDecoder::constellationDecimation;
    const auto& params = radioController->getParams();
    const size_t num_iqpoints = (params.L-1) * params.K / decim;
    if (constellationPointBuffer.size() == num_iqpoints) {
        constellationSeriesData.resize(num_iqpoints);

        size_t i = 0;
        for (int l = 1; l < params.L; l++) {
            size_t ix = (l-1) * params.K/decim;
            for (int k = 0; k < params.K; k += decim) {
                qreal y = 180.0f / (float)M_PI *
                    std::arg(constellationPointBuffer.at(ix++));

                qreal x = k - params.K/2.0 + (l-1)/((qreal)params.L/decim);
                constellationSeriesData[i++] = QPointF(x, y);
            }
        }

        // TM I:
        // k from -768 to 768, but not counting 0 gives a total of 1536 carriers
        x_min = -params.K / 2;
        x_max = params.K / 2;

        emit setConstellationAxis(x_min, x_max);

        if(constellationSeries)
            constellationSeries->replace(constellationSeriesData);
    }
    /*
    else {
        qDebug() << "IQ" << constellationPointBuffer.size() << num_iqpoints;
    }
    */
}

void CGUIHelper::openAutoDevice()
{
    CDeviceID deviceId;
    deviceId = radioController->openDevice();
    emit newDeviceId(static_cast<int>(deviceId));
}

void CGUIHelper::openAirspy()
{
    radioController->openDevice(CDeviceID::AIRSPY);
}

void CGUIHelper::setBiasTeeAirspy(bool isOn)
{
    radioController->setDeviceParam("biastee", isOn ? 1 : 0);
}

void CGUIHelper::openRtlSdr()
{
#ifdef __ANDROID__
    radioController->openDevice(CDeviceID::ANDROID_RTL_SDR);
#else
    radioController->openDevice(CDeviceID::RTL_SDR);
#endif
}

void CGUIHelper::setBiasTeeRtlSdr(bool isOn)
{
    radioController->setDeviceParam("biastee", isOn ? 1 : 0);
}

void CGUIHelper::openSoapySdr(QString driverArgs)
{
    radioController->deviceInitArgs[CDeviceID::SOAPYSDR] = driverArgs.toStdString();
    radioController->openDevice(CDeviceID::SOAPYSDR);
}

void CGUIHelper::setAntennaSoapySdr(QString text)
{
    radioController->setDeviceParam("SoapySDRAntenna", text);
}

void CGUIHelper::setDriverArgsSoapySdr(QString text)
{
    radioController->setDeviceParam("SoapySDRDriverArgs", text);
}

void CGUIHelper::setClockSourceSoapySdr(QString text)
{
    radioController->setDeviceParam("SoapySDRClockSource", text);
}

void CGUIHelper::openRtlTcp(QString IpAddress, int IpPort, bool force)
{
    radioController->openDevice(CDeviceID::RTL_TCP, force, IpAddress, IpPort);
}

void CGUIHelper::openRawFile(QString fileFormat)
{
#ifdef __ANDROID__
    // Open file selection dialog
    const auto ACTION_OPEN_DOCUMENT = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent", "ACTION_OPEN_DOCUMENT");
    QAndroidJniObject intent("android/content/Intent", "(Ljava/lang/String;)V", ACTION_OPEN_DOCUMENT.object());
    const auto CATEGORY_OPENABLE = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent", "CATEGORY_OPENABLE");
    intent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", CATEGORY_OPENABLE.object());
    intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString(QStringLiteral("*/*")).object());

    // Open file dialog
    activityResultReceiver = new FileActivityResultReceiver(this, fileFormat);
    QtAndroid::startActivity( intent.object<jobject>(), 12, activityResultReceiver);
#endif
}

void CGUIHelper::openRawFile(QString filename, QString fileFormat)
{   
    radioController->openDevice(CDeviceID::RAWFILE, true, filename, fileFormat);
}

void CGUIHelper::setNewDebugOutput(QString text)
{
    text = text.remove('\n');
    emit newDebugOutput(text);
}

void CGUIHelper::addTranslator(QString Language, QObject *obj)
{
    if(translator) {
        QCoreApplication::removeTranslator(translator);
        delete(translator);
    }

    translator = new QTranslator;

    if(Language == "auto")
    {
        QString locale = QLocale::system().name();
        Language = locale;
    }

    // Special handling for German
    if(Language == "de_AT" || Language ==  "de_CH" || Language ==  "de_BE" || Language ==  "de_IT" || Language ==  "de_LU")
    {
        qDebug() << "main:" <<  "Use de_DE instead of" << Language;
        Language = "de_DE";
    }

    // Special handling for French
    if(Language == "fr_BE" || Language == "fr_CA" || Language == "fr_CH" || Language == "fr_LU")
    {
        qDebug() << "main:" <<  "Use fr_FR instead of" << Language;
        Language = "fr_FR";
    }

    // Set new language
    qDebug() << "main:" <<  "Set language" << Language;
    bool isTranslation = translator->load(QString(":/i18n/") + Language);
    QCoreApplication::installTranslator(translator);

    if(!isTranslation && Language != "en_GB")
    {
        qDebug() << "main:" <<  "Error while loading language" << Language << "use English \"en_GB\" instead";
        Language = "en_GB";
    }

    // Set locale e.g. time formarts
    QLocale curLocale(QLocale((const QString&)Language));
    QLocale::setDefault(curLocale);

    // Start translation of GUI
    QQmlContext *currentContext = QQmlEngine::contextForObject(obj);
    QQmlEngine *engine = currentContext->engine();
    engine->retranslate();
}

#ifdef __ANDROID__
void FileActivityResultReceiver::handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &intent) {
    if (!intent.isValid()) {
        return;
    }

    const auto uri = intent.callObjectMethod("getData", "()Landroid/net/Uri;");
    if (!uri.isValid()) {
        return;
    }

    const auto scheme = uri.callObjectMethod("getScheme", "()Ljava/lang/String;");
    if (scheme.toString() == QLatin1String("content")) {
        const auto tmpFile = uri.callObjectMethod("toString", "()Ljava/lang/String;");
        guiHelper->openRawFile(QString(tmpFile.toString()), fileFormat);
    }
}
#endif
