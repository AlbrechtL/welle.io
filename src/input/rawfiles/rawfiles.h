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
 *	In a previous "dabstick-only" version of the software, we
 *	could be satisfied with ".raw" files that just contained the
 *	data, read fresh from the stick. Since the support of Mirics,
 *	with 16 bit values, we use the libsndfile
 *	To remain compatible with old files, there are
 *	two readers, one for "old" and one for new data
 */
#ifndef	__RAW_FILES
#define	__RAW_FILES

#include	<QThread>
#include	<QString>
#include	<QFrame>
#include	"dab-constants.h"
#include	"virtual-input.h"
#include	"ringbuffer.h"

#include	"ui_filereader-widget.h"

class	QLabel;
class	QSettings;
class	fileHulp;
/*
 */
class	rawFiles: public virtualInput, public Ui_filereaderWidget, QThread {
public:

			rawFiles	(QString, bool *);
	       		~rawFiles	(void);
	int32_t		getSamples	(DSPCOMPLEX *, int32_t);
	uint8_t		myIdentity	(void);
	int32_t		Samples		(void);
	bool		restartReader	(void);
	void		stopReader	(void);
private:
	QString		fileName;
virtual	void		run		(void);
	QFrame		*myFrame;
	int32_t		readBuffer	(uint8_t *, int32_t);
	RingBuffer<uint8_t>	*_I_Buffer;
	int32_t		bufferSize;
	FILE		*filePointer;
	bool		readerOK;
	bool		readerPausing;
	bool		ExitCondition;
	bool		ThreadFinished;
	int64_t		currPos;
};

#endif

