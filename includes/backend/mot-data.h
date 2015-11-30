#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the SDR-J (JSDR).
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

#ifndef	__MOT_HANDLER__
#define	__MOT_HANDLER__
#include	"dab-constants.h"
#include	<QObject>
#include	<QImage>
#include	<QLabel>

class	RadioInterface;

typedef struct  {
	int32_t ordernumber;
	uint16_t transportId;
	QByteArray body;
	int32_t	bodySize;
	uint16_t contentType;
	uint16_t contentsubType;
	int16_t  segmentSize;
	int16_t  numofSegments;
	bool	marked	[100];
} motElement;

class	motHandler: public QObject {
Q_OBJECT
public:
		motHandler	(RadioInterface *);
		~motHandler	(void);
void		processHeader (int16_t	transportId,
	                       uint8_t	*segment,
	                       int16_t	segmentSize,
	                       int16_t	headerSize,
	                       int32_t	bodySize,
	                       bool	lastFlag);

void		processSegment	(int16_t	transportId,
	                         uint8_t	*segment,
	                         int16_t	segmentNumber,
	                         int16_t	segmentSize,
	                         bool		lastFlag);
private:
	motElement table [16];
	int16_t	ordernumber;

	motElement	*getHandle	(uint16_t transportId);
	void	newEntry	(uint16_t	transportId,
	                         int16_t	size,
	                         int16_t	contentType,
	                         int16_t	contentsubType);
	bool	isComplete	(motElement *);
	void	handleComplete	(motElement *);
signals:
	void	pictureReady	(QByteArray);
};
#endif
