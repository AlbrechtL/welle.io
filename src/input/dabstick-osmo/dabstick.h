#
/*
 *    Copyright (C) 2010, 2011, 2012, 2013
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
 */

#ifndef _DABSTICK
#define	_DABSTICK

#include	<QObject>
#include	<QSettings>
#include	"dab-constants.h"
#include	"ringbuffer.h"
#include	"fir-filters.h"
#include	"virtual-input.h"
#include	"dongleselect.h"
#include	"ui_dabstick-widget-osmo.h"
class	dll_driver;
//
//	create typedefs for the library functions
typedef	struct rtlsdr_dev rtlsdr_dev_t;
extern "C"  {
typedef	void (*rtlsdr_read_async_cb_t) (uint8_t *buf, uint32_t len, void *ctx);
typedef	 int (*  pfnrtlsdr_open )(rtlsdr_dev_t **, uint32_t);
typedef	int (*  pfnrtlsdr_close) (rtlsdr_dev_t *);
typedef	int (*  pfnrtlsdr_set_center_freq) (rtlsdr_dev_t *, uint32_t);
typedef uint32_t (*  pfnrtlsdr_get_center_freq) (rtlsdr_dev_t *);
typedef	int (*  pfnrtlsdr_get_tuner_gains) (rtlsdr_dev_t *, int *);
typedef	int (*  pfnrtlsdr_set_tuner_gain_mode) (rtlsdr_dev_t *, int);
typedef	int (*  pfnrtlsdr_set_sample_rate) (rtlsdr_dev_t *, uint32_t);
typedef	int (*  pfnrtlsdr_get_sample_rate) (rtlsdr_dev_t *);
typedef	int (*  pfnrtlsdr_set_tuner_gain) (rtlsdr_dev_t *, int);
typedef	int (*  pfnrtlsdr_get_tuner_gain) (rtlsdr_dev_t *);
typedef int (*  pfnrtlsdr_reset_buffer) (rtlsdr_dev_t *);
typedef	int (*  pfnrtlsdr_read_async) (rtlsdr_dev_t *,
	                               rtlsdr_read_async_cb_t,
	                               void *,
	                               uint32_t,
	                               uint32_t);
typedef int (*  pfnrtlsdr_cancel_async) (rtlsdr_dev_t *);
typedef int (*  pfnrtlsdr_set_direct_sampling) (rtlsdr_dev_t *, int);
typedef uint32_t (*  pfnrtlsdr_get_device_count) (void);
typedef	int (* pfnrtlsdr_set_freq_correction)(rtlsdr_dev_t *, int);
typedef	char *(* pfnrtlsdr_get_device_name)(int);
}
//	This class is a simple wrapper around the
//	rtlsdr library that is read is as dll
//	It does not do any processing
class	dabStick: public virtualInput, public Ui_dabstickWidget_osmo {
Q_OBJECT
public:
			dabStick	(QSettings *, bool *, bool show = true);
			~dabStick	(void);
	void		setVFOFrequency	(int32_t);
	void		getVFOFrequency	(int32_t *);
	uint8_t		myIdentity	(void);
//	interface to the reader
	bool		restartReader	(void);
	void		stopReader	(void);
	int32_t		getSamples	(DSPCOMPLEX *, int32_t);
	int32_t		getSamples	(DSPCOMPLEX *, int32_t, int32_t);
	int32_t		Samples		(void);
	int32_t		getSamplesMissed	(void);
	void		resetBuffer	(void);
	int16_t		maxGain		(void);
	int16_t		bitDepth	(void);
//
	void		setGain		(int32_t);
	void		setAgc		(bool);
//
//	These need to be visible for the separate usb handling thread
	RingBuffer<uint8_t>	*_I_Buffer;
	pfnrtlsdr_read_async	rtlsdr_read_async;
	struct rtlsdr_dev	*device;
	int32_t		sampleCounter;
private:
	QFrame		*myFrame;
	QSettings	*dabstickSettings;
	dongleSelect	*dongleSelector;
	int32_t		inputRate;
	int32_t		deviceCount;
	HINSTANCE	Handle;
	dll_driver	*workerHandle;
	int32_t		lastFrequency;
	bool		libraryLoaded;
	bool		open;
	int		*gains;
	int16_t		gainsCount;
//	here we need to load functions from the dll
	bool		load_rtlFunctions	(void);
	pfnrtlsdr_open	rtlsdr_open;
	pfnrtlsdr_close	rtlsdr_close;

	pfnrtlsdr_set_center_freq rtlsdr_set_center_freq;
	pfnrtlsdr_get_center_freq rtlsdr_get_center_freq;
	pfnrtlsdr_get_tuner_gains rtlsdr_get_tuner_gains;
	pfnrtlsdr_set_tuner_gain_mode rtlsdr_set_tuner_gain_mode;
	pfnrtlsdr_set_sample_rate rtlsdr_set_sample_rate;
	pfnrtlsdr_get_sample_rate rtlsdr_get_sample_rate;
	pfnrtlsdr_set_tuner_gain rtlsdr_set_tuner_gain;
	pfnrtlsdr_get_tuner_gain rtlsdr_get_tuner_gain;
	pfnrtlsdr_reset_buffer rtlsdr_reset_buffer;
	pfnrtlsdr_cancel_async rtlsdr_cancel_async;
	pfnrtlsdr_set_direct_sampling	rtlsdr_set_direct_sampling;
	pfnrtlsdr_get_device_count rtlsdr_get_device_count;
	pfnrtlsdr_set_freq_correction rtlsdr_set_freq_correction;
	pfnrtlsdr_get_device_name rtlsdr_get_device_name;
private slots:
	void		setGain			(const QString &);
	void		set_autogain		(const QString &);
	void		set_fCorrection		(int);
	void		set_KhzOffset		(int);

};
#endif

