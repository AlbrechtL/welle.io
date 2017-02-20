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
	QString	name;
} motElement;

class	MOT_directory {
public:
	uint16_t	transportId;
	uint8_t		*dir_segments;
	int16_t		dir_segmentSize;
	int16_t		num_dirSegments;
	int16_t		dirSize;
	int16_t		numObjects;
	motElement	*dir_proper;
	bool		marked [512];
	MOT_directory (uint16_t transportId,
	               int16_t	segmentSize,
	               int32_t dirSize, int16_t objects) {
int16_t	i;
	   for (i = 0; i < 512; i ++)
	      marked [i] = false;
	   num_dirSegments	= -1;
	   this	-> transportId	= transportId;
	   this	-> dirSize	= dirSize;
	   this	-> numObjects	= objects;
	   this	-> dir_segmentSize	= segmentSize;
	   dir_segments = new uint8_t [dirSize];
	   dir_proper  = new motElement [objects];
	}

	~MOT_directory (void) {
	   delete [] dir_segments;
	   delete [] dir_proper;
	}
};

class	motHandler: public QObject {
Q_OBJECT
public:
		motHandler	(RadioInterface *);
		~motHandler	(void);
void		process_mscGroup	(uint8_t *,
	                                 uint8_t,
	                                 bool,
	                                 int16_t,
	                                 uint16_t);
void		processHeader (int16_t	transportId,
	                       uint8_t	*segment,
	                       int16_t	segmentSize,
	                       int16_t	headerSize,
	                       int32_t	bodySize,
	                       bool	lastFlag);
void		processDirectory (int16_t	transportId,
	                       uint8_t	*segment,
	                       int16_t	segmentSize,
	                       bool	lastFlag);
void		directorySegment (uint16_t	transportId,
	                          uint8_t	*segment,
	                          int16_t	segmentNumber,
	                          int16_t	segmentSize,
	                          bool		lastFlag);
void		analyse_theDirectory	(void);
int16_t		get_dirEntry	(int16_t	number,
	                         uint8_t *data,
	                         uint16_t currentBase);

void		processSegment	(int16_t	transportId,
	                         uint8_t	*segment,
	                         int16_t	segmentNumber,
	                         int16_t	segmentSize,
	                         bool		lastFlag);
	void	my_help		(void);
private:
	motElement	table [16];
	motElement	*old_slide;
	int16_t		ordernumber;
	MOT_directory	*theDirectory;
	
	motElement	*getHandle	(uint16_t transportId);
	void		newEntry	(uint16_t	transportId,
	                                 int16_t	size,
	                                 int16_t	contentType,
	                                 int16_t	contentsubType,
	                                 QString	name);
	void		newEntry	(int16_t	index,
	                                 uint16_t	transportId,
	                                 int16_t	size,
	                                 int16_t	contentType,
	                                 int16_t	contentsubType,
	                                 QString	name);
	bool		isComplete	(motElement *);
	void	handleComplete	(motElement *);
	void	checkDir	(QString &);
signals:
	void	the_picture	(QByteArray, int, QString);
};
#endif
