#
/*
 *    Copyright (C) 2010, 2011, 2012
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

#ifndef	__RTL_TCP_CLIENT
#define	__RTL_TCP_CLIENT
#include	<QtNetwork>
#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	<QLineEdit>
#include	<QHostAddress>
#include	<QByteArray>
#include	<QTcpSocket>
#include	<QTimer>
#include	<QComboBox>
#include	"dab-constants.h"
#include	"virtual-input.h"
#include	"ringbuffer.h"
#include	"ui_rtl_tcp-widget.h"

class	rtl_tcp_client: public virtualInput, Ui_rtl_tcp_widget {
Q_OBJECT
public:
			rtl_tcp_client (QSettings *, bool *, bool);
			~rtl_tcp_client	(void);
	int32_t		getRate		(void);
	bool		legalFrequency	(int32_t);
	int32_t		defaultFrequency	(void);
	void		setVFOFrequency	(int32_t);
	int32_t		getVFOFrequency	(void);
	bool		restartReader	(void);
	void		stopReader	(void);
	int32_t		getSamples	(DSPCOMPLEX *V, int32_t size);
#ifdef	GUI_3
	int32_t		getSamplesFromShadowBuffer (DSPCOMPLEX *V,
	                                            int32_t size);
#endif
	int32_t		Samples		(void);
	int16_t		bitDepth	(void);
	uint8_t		myIdentity	(void);
//
	void		setGain		(int32_t);
	void		setAgc		(bool);
//
private slots:
	void		sendGain	(int);
	void		set_Offset	(int);
	void		set_fCorrection	(int);
	void		readData	(void);
	void		setConnection	(void);
	void		wantConnect	(void);
	void		setDisconnect	(void);
private:
	void		sendVFO		(int32_t);
	void		sendRate	(int32_t);
    void        setGainMode (int32_t gainMode);
	void		sendCommand	(uint8_t, int32_t);
	QLineEdit	*hostLineEdit;
	bool		isvalidRate	(int32_t);
	QSettings	*remoteSettings;
	QFrame		*theFrame;
	int32_t		theRate;
	int32_t		vfoFrequency;
	RingBuffer<uint8_t>	*theBuffer;
    RingBuffer<uint8_t>	*theShadowBuffer;
	bool		connected;
	int16_t		theGain;
	int16_t		thePpm;
	QHostAddress	serverAddress;
	QTcpSocket	toServer;
	qint64		basePort;
};

#endif

