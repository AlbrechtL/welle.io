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
#include <QQuickStyle>
#include <QQmlProperty>

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
    motImageProvider = new CMOTImageProvider;

    QSettings settings;
    connect(RadioController, &CRadioController::motChanged, this, &CGUIHelper::motUpdate);
    connect(RadioController, &CRadioController::motReseted, this, &CGUIHelper::motReset);
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

    trayIcon->setIcon(QIcon(":/icons/icon.png"));
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

    qDebug() << "GUI:" <<  "Deleting CGUIHelper";
}

void CGUIHelper::close()
{
    qDebug() << "GUI:" <<  "Close application";
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
    QByteArray InfoContent;

    // Set application version
    InfoContent.append(getInfoPage("Versions"));

    InfoContent.append("For legal information scroll down, please.\n");
    InfoContent.append("\n");

    // Read AUTHORS
    InfoContent.append("AUTHORS\n");
    InfoContent.append("-------\n");
    InfoContent.append(getInfoPage("Authors"));

    // Read THANKS
    InfoContent.append("THANKS\n");
    InfoContent.append("------\n");
    InfoContent.append(getInfoPage("Thanks"));

    // Read COPYING
    InfoContent.append("COPYING (GPLv2)\n");
    InfoContent.append("-------\n");
    InfoContent.append(getInfoPage("GPL-2"));

    InfoContent.append("COPYING (LGPL-2.1)\n");
    InfoContent.append("-------\n");
    InfoContent.append(getInfoPage("LGPL-2.1"));

    InfoContent.append("kiss_fft COPYING (BSD 3-clause)\n");
    InfoContent.append("-------\n");
    InfoContent.append(getInfoPage("BSD-3-Clause"));

    // Set graph license content
    ret.insert("FileContent", InfoContent);

    return ret;
}

const QByteArray CGUIHelper::getFileContent(QString filepath)
{
    QFile *File;
    QByteArray InfoContent;
    File = new QFile(filepath);
    File->open(QFile::ReadOnly);
    InfoContent.append(File->readAll());
    InfoContent.append("\n");
    delete File;
    return InfoContent;
}

const QByteArray CGUIHelper::getInfoPage(QString pageName)
{
    QByteArray InfoContent;

    if (pageName == "Versions") {
        // Set application version
        InfoContent.append(tr("welle.io version") + ": " + QString(CURRENT_VERSION) + "\n");
        InfoContent.append(tr("Git revision") + ": " + QString(GITHASH) + "\n");
        QString ts = QString(__TIMESTAMP__).replace("  "," ");
        QDateTime tsDT = QLocale(QLocale::C).toDateTime(ts, "ddd MMM d hh:mm:ss yyyy");
        InfoContent.append(tr("Built on") + ": " + tsDT.toString(Qt::ISODate) + "\n");
        InfoContent.append(tr("QT version") + ": " + qVersion() + "\n");
        InfoContent.append("\n");
    } else if (pageName == "Authors") {
        return getFileContent(":/AUTHORS");
    } else if (pageName == "Thanks") {
        return getFileContent(":/THANKS");
    } else if (pageName == "GPL-2") {
        return getFileContent(":/COPYING");
    } else if (pageName == "LGPL-2.1") {
        return getFileContent(":/libs/COPYING.LGPL-2.1");
    } else if (pageName == "BSD-3-Clause") {
       return getFileContent(":/libs/kiss_fft/COPYING");
    }

    return InfoContent;
}

void CGUIHelper::motUpdate(mot_file_t mot_file)
{
    std::clog  << "SLS ContentName: " << mot_file.content_name << std::endl;
    std::clog  << "catSLS Category: " << std::to_string(mot_file.category) << " SlideID: " << std::to_string(mot_file.slide_id) << std::endl;
    std::clog  << "catSLS CategoryTitle: " << mot_file.category_title << std::endl;
    std::clog  << "ClickThroughURL: " << mot_file.click_through_url << std::endl;

    QString pictureName =
            "/" + QString::number(mot_file.category) +
            "/" + QString::fromStdString(mot_file.category_title) +
            "/" + QString::number(mot_file.slide_id) +
            "/" + QString::fromStdString(mot_file.content_name);

    QByteArray qdata(reinterpret_cast<const char*>(mot_file.data.data()), static_cast<int>(mot_file.data.size()));
    QImage motImage;
    motImage.loadFromData(qdata, mot_file.content_sub_type == 0 ? "GIF" : mot_file.content_sub_type == 1 ? "JPEG" : mot_file.content_sub_type == 2 ? "BMP" : "PNG");
    motImageProvider->setPixmap(QPixmap::fromImage(motImage), pictureName);

    emit motChanged(pictureName, QString::fromStdString(mot_file.category_title), mot_file.category, mot_file.slide_id);
}

void CGUIHelper::motReset()
{
    motImageProvider->clear();
    emit motReseted();
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
    QSettings settings;
    int count = settings.value("hideWindowTrayMessageDisplayCount",0).toInt();

    // Hide only if system tray is available otherwise ignore it. Standard Gnome doesn't have a system tray so user would lose the control.
    if(trayIcon->isSystemTrayAvailable() && count < 4) {
        trayIcon->showMessage(QCoreApplication::applicationName(),
                              tr("The program will keep running in the "
                              "system tray. To terminate the program, "
                              "choose \"%1\" in the context menu "
                              "of the system tray entry.").arg(
                                //: "Quit" translation should be the same as the one of system tray
                                tr("Quit")
                              ),
                              QIcon(":/icon.png"), 5000);
        settings.setValue("hideWindowTrayMessageDisplayCount", count+1);
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

void CGUIHelper::saveMotImages()
{
    motImageProvider->saveAll();
}

void CGUIHelper::openAutoDevice()
{
    CDeviceID deviceId;
    deviceId = radioController->openDevice();
    emit newDeviceId(static_cast<int>(deviceId));
}

void CGUIHelper::openNull()
{
    radioController->openDevice(CDeviceID::NULLDEVICE);
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

void CGUIHelper::openSoapySdr()
{
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

void CGUIHelper::openRtlTcp(QString serverAddress, int IpPort, bool force)
{
    radioController->openDevice(CDeviceID::RTL_TCP, force, serverAddress, IpPort);
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
#else
    (void) fileFormat;
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

void CGUIHelper::setTranslator(QTranslator *translator)
{
    this->translator = translator;
}

QString CGUIHelper::mapToLanguage(QString Language)
{
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
    return Language;
}

bool CGUIHelper::loadTranslationFile(QTranslator *translator, QString Language)
{
    bool isLoaded = translator->load(QString(":/i18n/") + Language);

    if(!isLoaded)
    {
        qDebug() << "main:" <<  "Error while loading language" << Language << "use untranslated text (ie. English)";
    }

    return isLoaded;
}

void CGUIHelper::updateTranslator(QString Language, QObject *obj)
{
    QString lang = mapToLanguage(Language);

    // Set locale e.g. time formarts
    QLocale curLocale(QLocale((const QString&)lang));
    QLocale::setDefault(curLocale);

    loadTranslationFile(translator, lang);
    translateGUI(obj);
}

void CGUIHelper::translateGUI(QObject *obj)
{
    // Save previous width & height
    // (because they are reset by the call to retranslate())
    QVariant width = QQmlProperty::read(obj, "width");
    QVariant height = QQmlProperty::read(obj, "height");

    // Start translation of GUI
    QQmlContext *currentContext = QQmlEngine::contextForObject(obj);
    QQmlEngine *engine = currentContext->engine();
    engine->retranslate();

    // Restore previous width & height
    // (because they are reset by the call to retranslate())
    QQmlProperty::write(obj, "width", width);
    QQmlProperty::write(obj, "height", height);

    // Start translation of non-QML GUI
#ifndef QT_NO_SYSTEMTRAYICON
    minimizeAction->setText(tr("Mi&nimize"));
    maximizeAction->setText(tr("Ma&ximize"));
    restoreAction->setText(tr("&Restore"));
    quitAction->setText(tr("&Quit"));
#endif

    emit translationFinished();
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

QString CGUIHelper::getQQStyleToLoad(QString styleNameArg)  // Static
{
    QSettings settings;
    QString settingStyle = settings.value("qQStyle","").toString();

    // In case this is a first launch where the setting in the config file is not set
    if (settingStyle.isEmpty()) {
        if (styleNameArg.isEmpty()) {
            settings.setValue("qQStyle", "Default");
            return "Default";
        }
        else {
            settings.setValue("qQStyle", styleNameArg);
            return styleNameArg;
        }
    }

    QStringList availableStyle = QQuickStyle::availableStyles();

    for ( const QString& curStyle : availableStyle ) {
         if (settingStyle == curStyle)
             return settingStyle;
    }
    if (settingStyle == "System_Auto")
        return QString();
    else
        return "Default";
}

const QStringList CGUIHelper::qQStyleComboList()
{
    if ( !m_comboList.isEmpty() )
        return m_comboList;

    m_comboList = QQuickStyle::availableStyles();
    m_comboList.sort();
    int position = m_comboList.indexOf("Default");
    m_comboList.move(position, 0);
    m_comboList.insert(1, "System_Auto");

    QString settingStyle = settings.value("qQStyle","").toString();
    settingsStyleInAvailableStyles = false;

    for ( const auto& style : m_comboList ) {
         if (settingStyle == style)
             settingsStyleInAvailableStyles = true;
    }

    if ( settingsStyleInAvailableStyles == false ) {
        m_comboList.append(settingStyle);
        qDebug() << "Style from the settings " << settingStyle << " not available on system. Adding it to the list of styles and loading 'Default' instead.";
    }

    return m_comboList;
}

bool CGUIHelper::isThemableStyle(QString style)
{
    return (style == "Universal" || style == "Material");
}

int CGUIHelper::getIndexOfQQStyle(QString style)
{
    //qDebug() << "getIndexOfQQStyle: " << style;
    return m_comboList.indexOf(style);
}

QString CGUIHelper::getQQStyle()
{
    return settings.value("qQStyle","").toString();
}

void CGUIHelper::saveQQStyle(int index)
{
    //qDebug() << "saveQQStyle : " << index;
    settings.setValue("qQStyle",m_comboList.value(index));
    emit styleChanged();
}

StyleModel::StyleModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

StyleModel* CGUIHelper::qQStyleComboModel()
{
    if (m_styleModel != nullptr)
        m_styleModel = nullptr;

    QString settingStyle = settings.value("qQStyle","").toString();

    QStringList styleList = qQStyleComboList();

    m_styleModel = new StyleModel();
    for ( const auto& style : styleList  ) {
        if ( !settingsStyleInAvailableStyles && (settingStyle == style)) {
            m_styleModel->addStyle(Style(Style(style + tr(" (unavailable, fallback to Default)"), style)));
        }
        else {
            if (style == "System_Auto")
                m_styleModel->addStyle(Style(tr("Style of system"), style));
            else if (style == "Default")
                m_styleModel->addStyle(Style("Default" + tr(" (Recommended)"), style));
            else
                m_styleModel->addStyle(Style(style, style));
        }
    }
    return m_styleModel;
}

QHash<int, QByteArray> StyleModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[LabelRole] = "label";
    roles[StyleRole] = "style";
    return roles;
}

Style::Style(const QString &label, const QString &style)
    : m_label(label), m_style(style)
{
}

QString Style::label() const
{
    return m_label;
}

QString Style::style() const
{
    return m_style;
}

void StyleModel::addStyle(const Style &style)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_styles << style;
    endInsertRows();
}

int StyleModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return m_styles.count();
}

QVariantMap StyleModel::get(int row) const
{
    QHash<int,QByteArray> names = roleNames();
    QHashIterator<int, QByteArray> i(names);
    QVariantMap res;
    QModelIndex idx = index(row, 0);
    while (i.hasNext()) {
        i.next();
        QVariant data = idx.data(i.key());
        res[i.value()] = data;
    }
    return res;
}

QVariant StyleModel::data(const QModelIndex & index, int role) const
{
    if (index.row() < 0 || index.row() >= m_styles.count())
        return QVariant();

    const Style &style = m_styles[index.row()];
    if (role == LabelRole)
        return style.label();
    else if (role == StyleRole)
        return style.style();
    return QVariant();
}
