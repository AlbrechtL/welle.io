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

#ifndef _GUI
#define _GUI

#include "dab-constants.h"
#include "fic-handler.h"
#include "msc-handler.h"
#include <QComboBox>
#include <QLabel>
#include <sndfile.h>

#include <QQmlContext>
#include <QTimer>
#include <QtQml/QQmlApplicationEngine>

#include <QList>
#include <QtCharts>
using namespace QtCharts;

#include "motimageprovider.h"
#include "stationlist.h"

#include "ofdm-processor.h"
#include "ringbuffer.h"

class QSettings;
class virtualInput;
class CAudio;

class mscHandler;
class ficHandler;

class common_fft;

typedef enum {
    ScanStart,
    ScanTunetoChannel,
    ScanCheckSignal,
    ScanWaitForFIC,
    ScanWaitForChannelNames,
    ScanDone
} tScanChannelState;

/*
 *	GThe main gui object. It inherits from
 *	QDialog and the generated form
 */
class RadioInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant stationModel READ stationModel NOTIFY stationModelChanged)
    Q_PROPERTY(QVariant licenses READ licenses CONSTANT)

public:
    RadioInterface(QSettings*,
        QString,
        uint8_t,
        QString,
        QObject* parent = NULL);
    ~RadioInterface();
    Q_INVOKABLE void channelClick(QString, QString);
    Q_INVOKABLE void startChannelScanClick(void);
    Q_INVOKABLE void stopChannelScanClick(void);
    Q_INVOKABLE void saveSettings(void);
    Q_INVOKABLE void inputEnableAGCChange(bool checked);
    Q_INVOKABLE void inputGainChange(double gain);
    Q_INVOKABLE void terminateProcess(void);
    QVariant stationModel() const
    {
        return p_stationModel;
    }
    MOTImageProvider* MOTImage;

private:
    QSettings* dabSettings;
    bool autoStart;
    int16_t threshold;
    void setModeParameters(uint8_t);
    DabParams dabModeParameters;
    uint8_t isSynced;
    uint8_t dabBand;
    bool running;
    virtualInput* inputDevice;
    ofdmProcessor* my_ofdmProcessor;
    ficHandler* my_ficHandler;
    mscHandler* my_mscHandler;
    CAudio* Audio;
    RingBuffer<int16_t>* AudioBuffer;
    common_fft* spectrum_fft_handler;
    bool autoCorrector;
    const QVariantMap licenses();
    const char* get_programm_type_string(uint8_t);
    const char* get_programm_language_string(uint8_t);
    void dumpControlState(QSettings*);

    QTimer CheckFICTimer;
    QTimer ScanChannelTimer;
    QTimer StationTimer;
    QString currentChannel;
    QString CurrentStation;
    QString CurrentDevice;

    bool isFICCRC;
    bool isSignalPresent;
    bool scanMode;
    int BandIIIChannelIt;
    int LBandChannelIt;
    tScanChannelState ScanChannelState;
    StationList stationList;
    QVector<QPointF> spectrum_data;
    int coarseCorrector;
    int fineCorrector;
    bool setDevice(QString);
    QString nextChannel(QString currentChannel);
    QString input_device;
    int32_t tunedFrequency;
    int LastCurrentManualGain;
    int CurrentFrameErrors;
    QVariant p_stationModel;

public slots:
    void end_of_waiting_for_stations(void);
    void set_fineCorrectorDisplay(int);
    void set_coarseCorrectorDisplay(int);
    void clearEnsemble(void);
    void addtoEnsemble(const QString&);
    void nameofEnsemble(int, const QString&);
    void show_frameErrors(int);
    void show_rsErrors(int);
    void show_aacErrors(int);
    void show_ficSuccess(bool);
    void show_snr(int);
    void setSynced(char);
    void showLabel(QString);
    void showMOT(QByteArray, int, QString);
    void sendDatagram(char*, int);
    void changeinConfiguration(void);
    void newAudio(int);
    //
    void show_mscErrors(int);
    void show_ipErrors(int);
    void setStereo(bool isStereo);
    void setSignalPresent(bool isSignal);
    void displayDateTime(int* DateTime);
    void updateSpectrum(QAbstractSeries* series);
    void setErrorMessage(QString ErrorMessage);

private slots:
    //
    //	Somehow, these must be connected to the GUI
    //	We assume that any GUI will need these three:
    void setStart(void);
    void set_channelSelect(QString);
    void updateTimeDisplay(void);
    void autoCorrector_on(void);

    void CheckFICTimerTimeout(void);
    void StationTimerTimeout(void);
signals:
    void currentStation(QString text);
    void stationText(QString text);
    void syncFlag(bool active);
    void ficFlag(bool active);
    void dabType(QString text);
    void audioType(QString text);
    void bitrate(int bitrate);
    void stationType(QString text);
    void languageType(QString text);
    void signalPower(int power);
    void motChanged(void);
    void channelScanStopped(void);
    void channelScanProgress(int progress);
    void foundChannelCount(int channelCount);
    void newDateTime(int Year, int Month, int Day, int Hour, int Minute);
    void setYAxisMax(qreal max);
    void setXAxisMinMax(qreal min, qreal max);
    void displayFreqCorr(int Freq);
    void displayMSCErrors(int Errors);
    void displayCurrentChannel(QString Channel, int Frequency);
    void displayFrameErrors(int Errors);
    void displayRSErrors(int Errors);
    void displayAACErrors(int Errors);
    void showErrorMessage(QString Text);
    void stationModelChanged();
};

#endif
