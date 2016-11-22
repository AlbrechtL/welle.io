#
/*
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
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

#ifndef __SDRPLAY__
#define	__SDRPLAY__

#include	<QObject>
#include	<QFrame>
#include	<QSettings>
#include	"dab-constants.h"
#include	"ringbuffer.h"
#include	"virtual-input.h"
#include	"ui_sdrplay-widget.h"

class	sdrplayWorker;
class	sdrplayLoader;

class	sdrplay: public virtualInput, public Ui_sdrplayWidget {
Q_OBJECT
public:
		sdrplay		(QSettings *, bool *, bool show = true);
		~sdrplay	(void);
	void	setVFOFrequency	(int32_t);
	int32_t	getVFOFrequency		(void);
	bool	legalFrequency		(int32_t);
	int32_t	defaultFrequency	(void);

	bool	restartReader		(void);
	void	stopReader		(void);
	int32_t	getSamples		(DSPCOMPLEX *, int32_t);
	int32_t	Samples			(void);
	uint8_t	myIdentity		(void);
	void	resetBuffer		(void);
	int16_t	maxGain			(void);
	int16_t	bitDepth		(void);
	void	setGain			(int32_t);
	void	setAgc			(bool);
private:
	QSettings	*sdrplaySettings;
	QFrame		*myFrame;
	sdrplayLoader	*theLoader;
	sdrplayWorker	*theWorker;
	RingBuffer<int16_t>	*_I_Buffer;
	int32_t		inputRate;
	int32_t		bandWidth;
	int32_t		vfoFrequency;
	int16_t		currentGain;
	int32_t		agcMode;
private slots:
	void		setGain_slider	(int);
	void		set_autoGain	(bool);
};
#endif

