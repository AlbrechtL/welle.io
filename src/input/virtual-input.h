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
 *	We have to create a simple virtual class here, since we
 *	want the interface with different devices (including  filehandling)
 *	to be transparent
 */
#ifndef	__VIRTUAL_INPUT
#define	__VIRTUAL_INPUT

#include	<stdint.h>
#include	"dab-constants.h"
#include	<QObject>
#include	<QDialog>

#define	NIX		0100
#define	FILEREADER	0200
#define	DAB_STICK	0101
#define	AIRSPY		0102
#define	ELAD		0104
#define	SDRPLAY		0110

#define	someStick(x)	(x & 017)
class	virtualInput: public QObject {
public:
			virtualInput 	(void);
virtual			~virtualInput 	(void);
virtual		void	setVFOFrequency	(int32_t);
virtual		int32_t	getVFOFrequency	(void);
virtual		uint8_t	myIdentity	(void);
virtual		bool	legalFrequency	(int32_t);
virtual		int32_t	defaultFrequency	(void);
virtual		bool	restartReader	(void);
virtual		void	stopReader	(void);
virtual		int32_t	getSamples	(DSPCOMPLEX *, int32_t);
virtual		int32_t	Samples		(void);
virtual		void	resetBuffer	(void);
virtual		int16_t	bitDepth	(void) { return 10;}
//
//	To accomodate gui_3 without a separate control for the device
virtual		void	setGain		(int32_t);
virtual		void	setAgc		(bool);
//
protected:
		int32_t	lastFrequency;
	        int32_t	vfoOffset;
};
#endif

