#
/*
 *    Copyright (C) 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the SDR-J suite of programs.
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
 *    along with JSDR; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __MIRICS_XXX
#define	__MIRICS_XXX

#include	<QObject>
#include	<QFrame>
#include	"dab-constants.h"
#include	"virtual-input.h"
#include	"ringbuffer.h"
#include	"mirisdr.h"

class		QSettings;
class		mirics_driver;
//

#include	"ui_xxx-widget.h"

class	mirics_xxx: public virtualInput, public Ui_xxxWidget {
Q_OBJECT
public:
		mirics_xxx	(QSettings *, bool *);
		~mirics_xxx	(void);
	void	setVFOFrequency		(int32_t);
	int32_t	getVFOFrequency		(void);
	int32_t	setExternalRate		(int32_t);
//	bool	legalFrequency		(int32_t);
	int32_t	defaultFrequency	(void);

	bool	restartReader		(void);
	void	stopReader		(void);
	int32_t	getSamples		(DSPCOMPLEX *, int32_t);
	int32_t	Samples			(void);
	uint8_t	myIdentity		(void);
	int16_t	maxGain			(void);
	void	resetBuffer		(void);
//	I_Buffer needs to be visible for use within the callback
	RingBuffer<int16_t>	*_I_Buffer;
	struct mirisdr_dev	*device;
	int32_t		bufferLength;
private slots:
	void		setExternalGain	(int);
	void		setKhzOffset	(int);
private:
	QSettings	*miriSettings;
	QFrame		*myFrame;
	int		*gains;
	int16_t		gainsCount;
	bool		setupDevice	(int32_t);
	int32_t		inputRate;
	int32_t		bandWidth;
	HINSTANCE	Handle;
	int32_t		deviceCount;
	mirics_driver	*workerHandle;
	int32_t		vfoFrequency;
	int32_t		vfoOffset;
	bool		open;
};
#endif

