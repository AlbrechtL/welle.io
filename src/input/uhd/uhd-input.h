#
/*
 *    Copyright (C) 2015
 *    Sebastian Held <sebastian.held@imst.de>
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
 */
#ifndef	__UHDINPUT
#define	__UHDINPUT

#include "virtual-input.h"

#include	<uhd/usrp/multi_usrp.hpp>
#include	"ringbuffer.h"
#include	<QThread>
#include	<QSettings>
#include	<QFrame>
#include	<QObject>
#include	"ui_uhd-widget.h"

class uhdInput;
//
//	the real worker:
class uhd_streamer : public QThread {
public:
	uhd_streamer (uhdInput *d);
	void stop();

private:
	uhdInput* m_theStick;
	virtual void run();
	volatile bool m_stop_signal_called;
};


class	uhdInput: public virtualInput, public Ui_uhdWidget {
Q_OBJECT
	friend class uhd_streamer;
public:
		uhdInput (QSettings *dabSettings, bool *success);
virtual	 	~uhdInput 	(void);
virtual	void	setVFOFrequency	(int32_t freq);
virtual	int32_t	getVFOFrequency	(void);
	bool	legalFrequency	(int32_t) {return true;}
	int32_t	defaultFrequency	(void) {return 100000000;}
virtual	bool	restartReader	(void);
virtual	void	stopReader	(void);
virtual	int32_t	getSamples	(DSPCOMPLEX *, int32_t size);
virtual	int32_t	Samples		(void);
	uint8_t	myIdentity	(void);
virtual	void	resetBuffer	(void);
virtual	int16_t	maxGain		(void);
	int16_t	bitDepth	(void);
//
private:
	QSettings	*uhdSettings;
	QFrame		*myFrame;
	uhd::usrp::multi_usrp::sptr m_usrp;
	uhd::rx_streamer::sptr m_rx_stream;
	RingBuffer<std::complex<float> > *theBuffer;
	uhd_streamer* m_workerHandle;
	int32_t		inputRate;
	int32_t		ringbufferSize;
private slots:
	void	setExternalGain	(int);
	void	set_fCorrection	(int);
	void	set_KhzOffset	(int);
};
#endif

