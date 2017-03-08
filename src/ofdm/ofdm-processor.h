/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012, 2013
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

#ifndef	__OFDM_PROCESSOR__
#define	__OFDM_PROCESSOR__
/*
 *
 */
#include	"DabConstants.h"
#include	<QThread>
#include	<QObject>
#include	"stdint.h"
#include	"phasereference.h"
#include	"ofdm-decoder.h"
#include	"CVirtualInput.h"
#include	"ringbuffer.h"
//
//	Note:
//	It was found that enlarging the buffersize to e.g. 8192
//	cannot be handled properly by the underlying system.
#define	DUMPSIZE		4096
class	RadioInterface;
class	common_fft;
class	ofdmDecoder;
class	ficHandler;
class	mscHandler;

class ofdmProcessor: public QThread {
Q_OBJECT
public:
		ofdmProcessor  	(CVirtualInput *,
	                         CDABParams	*,
	                         RadioInterface *,
	                         mscHandler *,
	                         ficHandler *,
	                         int16_t,
	                         uint8_t);
		~ofdmProcessor	(void);
	void	reset			(void);
	void	stop		(void);
	void	setOffset	(int32_t);
	void	coarseCorrectorOn	(void);
	void	coarseCorrectorOff	(void);
	void	set_scanMode		(bool, QString);
private:
	CVirtualInput	*theRig;
	CDABParams	*params;
	RadioInterface	*myRadioInterface;
	ficHandler	*my_ficHandler;

	bool		running;
	int16_t		gain;
	int32_t		T_null;
	int32_t		T_u;
	int32_t		T_s;
	int32_t		T_g;
	int32_t		T_F;
	float		sLevel;
	DSPCOMPLEX	*dataBuffer;
	int32_t		FreqOffset;
	DSPCOMPLEX	*oscillatorTable;
	int32_t		localPhase;
	int16_t		fineCorrector;
	int32_t		coarseCorrector;

	uint8_t		freqsyncMethod;
	bool		f2Correction;
	int32_t		tokenCount;
	DSPCOMPLEX	*ofdmBuffer;
	uint32_t	ofdmBufferIndex;
	uint32_t	ofdmSymbolCount;
	phaseReference	phaseSynchronizer;
	ofdmDecoder	my_ofdmDecoder;
	DSPFLOAT	avgCorr;
	float		*correlationVector;
	float		*refArg;
	int32_t		sampleCnt;
	int32_t		inputSize;
	int32_t		inputPointer;
	DSPCOMPLEX	getSample	(int32_t);
	void		getSamples	(DSPCOMPLEX *, int16_t, int32_t);
	bool		scanMode;
	int32_t		NoReadCounter;

virtual	void		run		(void);
	int32_t		bufferContent;
	bool		isReset;
	int16_t		processBlock_0	(DSPCOMPLEX *);
	int16_t		getMiddle	(DSPCOMPLEX *);
	common_fft	*fft_handler;
	DSPCOMPLEX	*fft_buffer;
signals:
	void		show_fineCorrector	(int);
	void		show_coarseCorrector	(int);
	void		setSynced		(char);
    void		setSignalPresent	(bool);
    void        setErrorMessage (QString);
};
#endif

