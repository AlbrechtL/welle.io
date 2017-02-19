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

#ifndef	__RAW_FILES
#define	__RAW_FILES

#include	<QThread>
#include	<QString>
#include	<QSettings>


#include	"dab-constants.h"
#include	"CVirtualInput.h"
#include	"ringbuffer.h"

class	QLabel;
class	QSettings;
class	fileHulp;
/*
 */
class	CRAWFile: public virtualInput, QThread
{
public:
    CRAWFile	(QSettings *settings, bool *);
                ~CRAWFile	(void);
	int32_t		getSamples	(DSPCOMPLEX *, int32_t);
    int32_t     getSamplesFromShadowBuffer (DSPCOMPLEX *V, int32_t size);
	uint8_t		myIdentity	(void);
	int32_t		Samples		(void);
	bool		restartReader	(void);
	void		stopReader	(void);

private:
    QString		fileName;

virtual	void		run		(void);
	int32_t		readBuffer	(uint8_t *, int32_t);
    RingBuffer<uint8_t>	*SampleBuffer;
    RingBuffer<uint8_t>	*SpectrumSampleBuffer;
	int32_t		bufferSize;
	FILE		*filePointer;
	bool		readerOK;
	bool		readerPausing;
	bool		ExitCondition;
	bool		ThreadFinished;
	int64_t		currPos;
};

#endif

