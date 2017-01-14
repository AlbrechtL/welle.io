#
/*
 *    Copyright (C)  2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Gui design and implementation in gui_3:
 *	(c) Albrecht Lohoefener
 */

#ifndef _GUI
#define _GUI

#include	"dab-constants.h"
#include	<sndfile.h>
#include	<QComboBox>
#include	<QLabel>

#include	<QTimer>
#include	<QtQml/QQmlApplicationEngine>
#include	<QQmlContext>

#include	<QtCharts>
using namespace QtCharts;

#include	"stationlist.h"
#include    "motimageprovider.h"

#include	"ofdm-processor.h"
#include	"ringbuffer.h"

class	QSettings;
class	virtualInput;
class	audioBase;

class	mscHandler;
class	ficHandler;

class	common_fft;

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
class RadioInterface: public QObject{
Q_OBJECT

public:
        	RadioInterface		(QSettings *,
	                                 QQmlApplicationEngine *,
	                                 QString,
	                                 uint8_t,
	                                 QString,
                                         QObject *parent = NULL);
		~RadioInterface		();

private:
	QSettings	*dabSettings;
	QQmlApplicationEngine *engine;
	bool		autoStart;
	int16_t		threshold;
	void		setModeParameters	(uint8_t);
	DabParams	dabModeParameters;
	uint8_t		isSynced;
	uint8_t		dabBand;
	bool		running;
	virtualInput	*inputDevice;
	ofdmProcessor	*my_ofdmProcessor;
	ficHandler	*my_ficHandler;
	mscHandler	*my_mscHandler;
	audioBase	*soundOut;
	RingBuffer<int16_t>	*audioBuffer;
    //DSPCOMPLEX	*spectrumBuffer;
    common_fft *spectrum_fft_handler;
	bool		autoCorrector;
const	char		*get_programm_type_string (uint8_t);
const	char		*get_programm_language_string (uint8_t);
	void		dumpControlState	(QSettings *);

	QTimer		CheckFICTimer;
	QTimer		ScanChannelTimer;
	QString		currentChannel;
	QString		CurrentStation;
	QString		CurrentDevice;

	bool		isFICCRC;
	bool		isSignalPresent;
	bool		scanMode;
	int		BandIIIChannelIt;
	int		LBandChannelIt;
	tScanChannelState ScanChannelState;
	StationList	stationList;
	QVector<QPointF> spectrum_data;
	int		coarseCorrector;
	int		fineCorrector;
	bool		setDevice		(QString);
	QString		nextChannel		(QString currentChannel);
    QString input_device;
    MOTImageProvider *MOTImage;
    int32_t	tunedFrequency;

public slots:
	void		end_of_waiting_for_stations	(void);
	void		set_fineCorrectorDisplay	(int);
	void		set_coarseCorrectorDisplay	(int);
	void		clearEnsemble		(void);
	void		addtoEnsemble		(const QString &);
	void		nameofEnsemble		(int, const QString &);
	void		show_successRate	(int);
	void		show_ficCRC		(bool);
	void		show_snr		(int);
	void		setSynced		(char);
	void		showLabel		(QString);
	void		showMOT			(QByteArray, int, QString);
	void		sendDatagram		(char *, int);
	void		changeinConfiguration	(void);
	void		newAudio		(int);
//
	void		show_mscErrors		(int);
	void		show_ipErrors		(int);
	void		setStereo		(bool isStereo);
    void		setSignalPresent	(bool isSignal);
	void		displayDateTime		(int *DateTime);
	void		updateSpectrum		(QAbstractSeries *series);

private slots:
//
//	Somehow, these must be connected to the GUI
//	We assume that any GUI will need these three:
	void		setStart		(void);
	void		TerminateProcess	(void);
	void		set_channelSelect	(QString);
	void		updateTimeDisplay	(void);
	void		autoCorrector_on	(void);

	void		CheckFICTimerTimeout    (void);
	void		channelClick		(QString, QString);
	void		startChannelScanClick	(void);
	void		stopChannelScanClick	(void);
	void		saveSettings		(void);
    void		inputEnableAGCChange    (bool checked);
    void		inputGainChange (double gain);
signals:
	void		currentStation 		(QString text);
	void		stationText		(QString text);
    void		syncFlag		(bool active);
    void		ficFlag			(bool active);
	void		dabType			(QString text);
	void		audioType		(QString text);
	void		bitrate			(int bitrate);
	void		stationType		(QString text);
	void		languageType		(QString text);
	void		signalPower		(int power);
    void		motChanged		(void);
	void		channelScanStopped	(void);
	void		channelScanProgress	(int progress);
	void		foundChannelCount	(int channelCount);
	void		newDateTime		(int Year, int Month,
	                                         int Day, int Hour, int Minute);
    void		setYAxisMax         (qreal max);
    void		setXAxisMinMax		(qreal min, qreal max);
	void		displayFreqCorr		(int Freq);
	void		displayMSCErrors	(int Errors);
	void		displayCurrentChannel	(QString Channel,
	                                             int Frequency);
	void		displaySuccessRate	(int Rate);
};

#endif

